#include "PostManager.hpp"
#include "core/database/DatabaseManager.hpp"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QMutexLocker>
#include <QDebug>

namespace edv::core::journal {

// ── Internal helpers ──────────────────────────────────────────────────────────

static bool bindOptionalInt(QSqlQuery& q, const QString& placeholder,
                            const std::optional<int>& v)
{
    if (v.has_value())
        q.bindValue(placeholder, *v);
    else
        q.bindValue(placeholder, QVariant(QMetaType(QMetaType::Int)));
    return true;
}

// ── PostManager ───────────────────────────────────────────────────────────────

QString PostManager::generateDocumentNumber(
    DocumentType dt,
    int fiscalYear,
    QSqlDatabase& db)
{
    QMutexLocker lk(&m_seqMutex);

    QSqlQuery q(db);
    q.exec("SELECT NEXTVAL('seq_journal_document')");
    if (!q.next()) {
        m_lastError = q.lastError().text();
        return {};
    }

    const long long seq = q.value(0).toLongLong();
    return QString("%1-%2-%3")
        .arg(docTypeToString(dt))
        .arg(fiscalYear)
        .arg(seq, 6, 10, QChar('0'));
}

PostingResult PostManager::post(const PostingRequest& req)
{
    // ── Pre-validation ─────────────────────────────────────────────────────
    if (!req.isBalanced()) {
        return {false, {}, QStringLiteral("Journal entry is not balanced (debit ≠ credit)")};
    }
    if (!req.postingDate.isValid()) {
        return {false, {}, QStringLiteral("Invalid posting date")};
    }

    // ── Acquire DB connection ──────────────────────────────────────────────
    auto token = DatabaseManager::instance().acquire();
    if (!token) {
        return {false, {}, QStringLiteral("Database unavailable")};
    }

    QSqlDatabase db = token->connection();

    // ── Generate document number (needs sequence in DB) ────────────────────
    const int fiscalYear = req.postingDate.year();
    const QString docNum = generateDocumentNumber(req.documentType, fiscalYear, db);
    if (docNum.isEmpty()) {
        return {false, {}, m_lastError};
    }

    // ── ACID transaction ───────────────────────────────────────────────────
    if (!db.transaction()) {
        return {false, {}, db.lastError().text()};
    }

    // 1. Insert document header
    {
        QSqlQuery q(db);
        q.prepare(
            "INSERT INTO journal_document_header "
            "  (document_number, document_type, posting_date, document_date, "
            "   reference, header_text, currency, posted_by_user_id, created_at) "
            "VALUES "
            "  (:dn, :dt, :pd, :dd, :ref, :ht, :cur, :uid, NOW())"
        );
        q.bindValue(":dn",  docNum);
        q.bindValue(":dt",  docTypeToString(req.documentType));
        q.bindValue(":pd",  req.postingDate);
        q.bindValue(":dd",  req.documentDate.isValid() ? req.documentDate : req.postingDate);
        q.bindValue(":ref", req.reference);
        q.bindValue(":ht",  req.headerText);
        q.bindValue(":cur", req.currency);
        q.bindValue(":uid", req.postedByUserId);

        if (!q.exec()) {
            db.rollback();
            return {false, {}, q.lastError().text()};
        }
    }

    // 2. Insert Universal Journal lines
    for (const auto& line : req.lines) {
        QSqlQuery q(db);
        q.prepare(
            "INSERT INTO fact_universal_journal "
            "  (document_number, gl_account, amount, currency, cost_center, profit_center, "
            "   line_text, customer_id, vendor_id, material_id, employee_id, "
            "   sales_order_id, purchase_order_id, posting_date, created_at) "
            "VALUES "
            "  (:dn, :gl, :amt, :cur, :cc, :pc, :lt, :cid, :vid, :mid, :eid, :soid, :poid, :pd, NOW())"
        );
        q.bindValue(":dn",  docNum);
        q.bindValue(":gl",  line.glAccount);
        q.bindValue(":amt", line.amount);
        q.bindValue(":cur", line.currency.isEmpty() ? req.currency : line.currency);
        q.bindValue(":cc",  line.costCenter);
        q.bindValue(":pc",  line.profitCenter);
        q.bindValue(":lt",  line.text);
        q.bindValue(":pd",  req.postingDate);
        bindOptionalInt(q, ":cid",  line.customerId);
        bindOptionalInt(q, ":vid",  line.vendorId);
        bindOptionalInt(q, ":mid",  line.materialId);
        bindOptionalInt(q, ":eid",  line.employeeId);
        bindOptionalInt(q, ":soid", line.salesOrderId);
        bindOptionalInt(q, ":poid", line.purchaseOrderId);

        if (!q.exec()) {
            db.rollback();
            return {false, {}, q.lastError().text()};
        }
    }

    if (!db.commit()) {
        db.rollback();
        return {false, {}, db.lastError().text()};
    }

    qDebug() << "[PostManager] Posted document:" << docNum
             << "lines:" << req.lines.size();

    return {true, docNum, {}};
}

PostingResult PostManager::reverse(
    const QString& documentNumber,
    const QDate&   reversalDate,
    int            postedByUserId)
{
    if (!reversalDate.isValid())
        return {false, {}, "Invalid reversal date"};

    auto token = DatabaseManager::instance().acquire();
    if (!token)
        return {false, {}, "Database unavailable"};

    QSqlDatabase db = token->connection();

    // Fetch original header
    QSqlQuery hdr(db);
    hdr.prepare(
        "SELECT document_type, currency, reference, header_text "
        "FROM journal_document_header WHERE document_number = :dn"
    );
    hdr.bindValue(":dn", documentNumber);
    if (!hdr.exec() || !hdr.next())
        return {false, {}, "Document not found: " + documentNumber};

    const QString docType   = hdr.value(0).toString();
    const QString currency  = hdr.value(1).toString();
    const QString reference = hdr.value(2).toString();
    const QString origText  = hdr.value(3).toString();

    // Fetch original lines
    QSqlQuery lines(db);
    lines.prepare(
        "SELECT gl_account, amount, cost_center, profit_center, line_text, "
        "       customer_id, vendor_id, material_id, employee_id, "
        "       sales_order_id, purchase_order_id "
        "FROM fact_universal_journal WHERE document_number = :dn"
    );
    lines.bindValue(":dn", documentNumber);
    if (!lines.exec())
        return {false, {}, lines.lastError().text()};

    // Build reversal request (all amounts negated)
    PostingRequest rev;
    // Map string docType back to enum — default SA for reversal
    rev.documentType    = DocumentType::SA;
    rev.postingDate     = reversalDate;
    rev.documentDate    = reversalDate;
    rev.reference       = "REV-" + documentNumber;
    rev.headerText      = "Reversal of: " + origText;
    rev.currency        = currency;
    rev.postedByUserId  = postedByUserId;

    while (lines.next()) {
        JournalLine l;
        l.glAccount   = lines.value(0).toInt();
        l.amount      = -lines.value(1).toDouble(); // NEGATED
        l.costCenter  = lines.value(2).toString();
        l.profitCenter= lines.value(3).toString();
        l.text        = lines.value(4).toString();
        if (!lines.isNull(5))  l.customerId      = lines.value(5).toInt();
        if (!lines.isNull(6))  l.vendorId        = lines.value(6).toInt();
        if (!lines.isNull(7))  l.materialId      = lines.value(7).toInt();
        if (!lines.isNull(8))  l.employeeId      = lines.value(8).toInt();
        if (!lines.isNull(9))  l.salesOrderId    = lines.value(9).toInt();
        if (!lines.isNull(10)) l.purchaseOrderId = lines.value(10).toInt();
        rev.lines.append(l);
    }

    return post(rev);
}

} // namespace edv::core::journal
