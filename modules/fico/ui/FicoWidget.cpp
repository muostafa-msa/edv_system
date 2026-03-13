#include "FicoWidget.hpp"
#include "ui_FicoWidget.h"
#include "core/database/DatabaseManager.hpp"
#include "core/journal/PostManager.hpp"

#include <QSqlQuery>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QInputDialog>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QDateEdit>
#include <QComboBox>
#include <QTimer>

// ── JournalHeaderModel ────────────────────────────────────────────────────────

JournalHeaderModel::JournalHeaderModel(QObject* parent)
    : QAbstractTableModel(parent) {}

QVariant JournalHeaderModel::data(const QModelIndex& idx, int role) const
{
    if (!idx.isValid() || idx.row() >= m_data.size()) return {};
    const auto& r = m_data[idx.row()];

    if (role == Qt::DisplayRole) {
        switch (idx.column()) {
            case 0: return r.documentNumber;
            case 1: return r.documentType;
            case 2: return r.postingDate.toString("yyyy-MM-dd");
            case 3: return r.reference;
            case 4: return r.headerText;
            case 5: return QString("%1 EUR").arg(r.totalDebit, 0, 'f', 2);
            case 6: return r.isReversed ? tr("Reversed") : tr("Posted");
        }
    }
    if (role == Qt::TextAlignmentRole && idx.column() == 5)
        return int(Qt::AlignRight | Qt::AlignVCenter);
    if (role == Qt::ForegroundRole && r.isReversed)
        return QColor("#8B949E");
    return {};
}

QVariant JournalHeaderModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static const QStringList h = {
            "Document No.", "Type", "Posting Date", "Reference", "Description", "Debit", "Status"
        };
        return h.value(section);
    }
    return {};
}

void JournalHeaderModel::load(const QString& typeFilter, const QString& search)
{
    beginResetModel();
    m_data.clear();

    auto token = DatabaseManager::instance().acquire();
    if (!token) { endResetModel(); return; }

    QString sql =
        "SELECT h.document_number, h.document_type, h.posting_date, h.reference, "
        "       h.header_text, COALESCE(SUM(CASE WHEN f.amount>0 THEN f.amount ELSE 0 END),0), "
        "       h.is_reversed "
        "FROM journal_document_header h "
        "LEFT JOIN fact_universal_journal f ON f.document_number=h.document_number ";

    QStringList where;
    if (!typeFilter.isEmpty() && typeFilter != "All Types")
        where << "h.document_type=:dtype";
    if (!search.isEmpty())
        where << "(h.document_number ILIKE :s OR h.header_text ILIKE :s OR h.reference ILIKE :s)";
    if (!where.isEmpty()) sql += "WHERE " + where.join(" AND ") + " ";

    sql += "GROUP BY h.document_number, h.document_type, h.posting_date, "
           "         h.reference, h.header_text, h.is_reversed "
           "ORDER BY h.posting_date DESC, h.document_number DESC LIMIT 500";

    QSqlQuery q(token->connection());
    q.prepare(sql);
    if (!typeFilter.isEmpty() && typeFilter != "All Types")
        q.bindValue(":dtype", typeFilter.left(2));
    if (!search.isEmpty())
        q.bindValue(":s", "%" + search + "%");

    if (q.exec()) {
        while (q.next()) {
            m_data.append({
                q.value(0).toString(), q.value(1).toString(),
                q.value(2).toDate(),   q.value(3).toString(),
                q.value(4).toString(), q.value(5).toDouble(),
                q.value(6).toBool()
            });
        }
    }
    endResetModel();
}

QString JournalHeaderModel::documentNumberAt(int row) const
{
    if (row < 0 || row >= m_data.size()) return {};
    return m_data[row].documentNumber;
}

// ── FicoWidget ────────────────────────────────────────────────────────────────

FicoWidget::FicoWidget(const Session& session, QWidget *parent)
    : QWidget(parent), ui(new Ui::FicoWidget)
    , m_model(new JournalHeaderModel(this))
    , m_session(session)
{
    ui->setupUi(this);
    ui->journalTable->setModel(m_model);
    ui->journalTable->horizontalHeader()->setStretchLastSection(true);
    ui->journalTable->verticalHeader()->hide();
    ui->journalTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

    // Date defaults for trial balance
    ui->tbFromDate->setDate(QDate(QDate::currentDate().year(), 1, 1));
    ui->tbToDate->setDate(QDate::currentDate());

    connect(ui->refreshBtn,    &QPushButton::clicked, this, &FicoWidget::onRefresh);
    connect(ui->newEntryBtn,   &QPushButton::clicked, this, &FicoWidget::onNewEntry);
    connect(ui->reverseBtn,    &QPushButton::clicked, this, &FicoWidget::onReverse);
    connect(ui->runTbBtn,      &QPushButton::clicked, this, &FicoWidget::onRunTrialBalance);
    connect(ui->searchEdit,    &QLineEdit::textChanged, this, [this](){ onFilterChanged(); });
    connect(ui->docTypeFilter, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &FicoWidget::onFilterChanged);

    // Hide buttons for staff
    if (!session.hasRole(edv::core::security::Role::Manager)) {
        ui->newEntryBtn->hide();
        ui->reverseBtn->hide();
    }

    loadJournalTable();
}

FicoWidget::~FicoWidget() { delete ui; }

void FicoWidget::onRefresh()   { loadJournalTable(); }
void FicoWidget::onFilterChanged() { loadJournalTable(); }

void FicoWidget::loadJournalTable()
{
    const QString typeFilter = ui->docTypeFilter->currentText();
    const QString search     = ui->searchEdit->text().trimmed();
    m_model->load(typeFilter, search);
}

void FicoWidget::onNewEntry()
{
    // Simple GL entry dialog
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle(tr("New GL Journal Entry"));
    dlg->setMinimumWidth(480);

    auto* form = new QFormLayout(dlg);
    form->setSpacing(12); form->setContentsMargins(24,24,24,24);

    auto* headerText = new QLineEdit(dlg);
    auto* ref        = new QLineEdit(dlg);
    auto* postDate   = new QDateEdit(QDate::currentDate(), dlg);
    postDate->setCalendarPopup(true);

    auto* drAccount  = new QSpinBox(dlg);
    drAccount->setRange(1000, 9999); drAccount->setValue(1000);
    auto* crAccount  = new QSpinBox(dlg);
    crAccount->setRange(1000, 9999); crAccount->setValue(4000);
    auto* amount     = new QDoubleSpinBox(dlg);
    amount->setRange(0.01, 9999999.99); amount->setDecimals(2);
    auto* description= new QLineEdit(dlg);

    form->addRow(tr("Posting Date:"),    postDate);
    form->addRow(tr("Header Text:"),     headerText);
    form->addRow(tr("Reference:"),       ref);
    form->addRow(tr("Debit Account:"),   drAccount);
    form->addRow(tr("Credit Account:"),  crAccount);
    form->addRow(tr("Amount (EUR):"),    amount);
    form->addRow(tr("Line Text:"),       description);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dlg);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

    if (dlg->exec() != QDialog::Accepted) return;

    if (headerText->text().trimmed().isEmpty() || amount->value() < 0.01) {
        QMessageBox::warning(this, tr("Validation"), tr("Header text and amount are required."));
        return;
    }

    PostingRequest req;
    req.documentType   = DocumentType::SA;
    req.postingDate    = postDate->date();
    req.documentDate   = postDate->date();
    req.reference      = ref->text().trimmed();
    req.headerText     = headerText->text().trimmed();
    req.postedByUserId = m_session.userId;

    JournalLine debit;
    debit.glAccount = drAccount->value();
    debit.amount    = amount->value();
    debit.text      = description->text().trimmed();
    req.lines.append(debit);

    JournalLine credit;
    credit.glAccount = crAccount->value();
    credit.amount    = -amount->value();
    credit.text      = description->text().trimmed();
    req.lines.append(credit);

    auto result = PostManager::instance().post(req);
    if (result.success) {
        QMessageBox::information(this, tr("Posted"),
            tr("Document posted: %1").arg(result.documentNumber));
        loadJournalTable();
    } else {
        QMessageBox::critical(this, tr("Posting Failed"), result.errorMessage);
    }
}

void FicoWidget::onReverse()
{
    const auto idx = ui->journalTable->currentIndex();
    if (!idx.isValid()) {
        QMessageBox::information(this, tr("Selection"), tr("Please select a document to reverse."));
        return;
    }

    const QString docNum = m_model->documentNumberAt(idx.row());
    if (QMessageBox::question(this, tr("Reverse Document"),
            tr("Reverse document %1?").arg(docNum),
            QMessageBox::Yes | QMessageBox::Cancel) != QMessageBox::Yes)
        return;

    auto result = PostManager::instance().reverse(docNum, QDate::currentDate(), m_session.userId);
    if (result.success) {
        QMessageBox::information(this, tr("Reversed"),
            tr("Reversal posted: %1").arg(result.documentNumber));
        loadJournalTable();
    } else {
        QMessageBox::critical(this, tr("Reversal Failed"), result.errorMessage);
    }
}

void FicoWidget::onRunTrialBalance()
{
    loadTrialBalance(ui->tbFromDate->date(), ui->tbToDate->date());
}

void FicoWidget::loadTrialBalance(const QDate& from, const QDate& to)
{
    auto token = DatabaseManager::instance().acquire();
    if (!token) return;

    QSqlQuery q(token->connection());
    q.prepare(
        "SELECT c.gl_code, c.description, "
        "       COALESCE(SUM(CASE WHEN f.amount>0 THEN f.amount ELSE 0 END),0) AS debit, "
        "       COALESCE(ABS(SUM(CASE WHEN f.amount<0 THEN f.amount ELSE 0 END)),0) AS credit "
        "FROM chart_of_accounts c "
        "LEFT JOIN fact_universal_journal f "
        "       ON f.gl_account=c.gl_code "
        "      AND f.posting_date BETWEEN :from AND :to "
        "GROUP BY c.gl_code, c.description "
        "ORDER BY c.gl_code"
    );
    q.bindValue(":from", from);
    q.bindValue(":to",   to);

    auto* tbl = ui->trialBalanceTable;
    tbl->setRowCount(0);
    tbl->horizontalHeader()->setStretchLastSection(true);

    if (!q.exec()) return;

    double totalDr = 0, totalCr = 0;
    int row = 0;
    while (q.next()) {
        double dr = q.value(2).toDouble();
        double cr = q.value(3).toDouble();
        double bal = dr - cr;
        totalDr += dr; totalCr += cr;

        tbl->insertRow(row);
        tbl->setItem(row, 0, new QTableWidgetItem(q.value(0).toString()));
        tbl->setItem(row, 1, new QTableWidgetItem(q.value(1).toString()));
        tbl->setItem(row, 2, new QTableWidgetItem(QString("%1").arg(dr, 0,'f',2)));
        tbl->setItem(row, 3, new QTableWidgetItem(QString("%1").arg(cr, 0,'f',2)));
        auto* balItem = new QTableWidgetItem(QString("%1").arg(bal, 0,'f',2));
        balItem->setForeground(bal >= 0 ? QColor("#2EA043") : QColor("#F85149"));
        tbl->setItem(row, 4, balItem);
        ++row;
    }

    // Totals row
    tbl->insertRow(row);
    auto bold = QFont(); bold.setBold(true);
    for (int c = 0; c < 5; ++c) {
        QString txt;
        switch(c) {
            case 1: txt = tr("TOTALS"); break;
            case 2: txt = QString("%1").arg(totalDr,0,'f',2); break;
            case 3: txt = QString("%1").arg(totalCr,0,'f',2); break;
            case 4: txt = QString("%1").arg(totalDr-totalCr,0,'f',2); break;
        }
        auto* it = new QTableWidgetItem(txt);
        it->setFont(bold);
        tbl->setItem(row, c, it);
    }
}
