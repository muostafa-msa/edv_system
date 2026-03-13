#pragma once

#include "JournalEntry.hpp"
#include <QString>
#include <QMutex>
#include <QSqlDatabase>

namespace edv::core::journal {

/**
 * @brief ACID atomic journal posting engine.
 *
 * Every financial event (sales invoice, goods receipt, payroll run, etc.)
 * is persisted through PostManager::post(). It guarantees:
 *
 *  - Balanced posting (debit == credit)
 *  - Atomic DB transaction (rolled back on any failure)
 *  - Sequential document numbering per document type / fiscal year
 *  - Audit trail: created_at, posted_by_user_id
 *
 * PostManager is thread-safe. Multiple threads may call post() concurrently;
 * document number generation is serialized via an internal mutex + DB sequence.
 */
class PostManager {
public:
    static PostManager& instance() {
        static PostManager inst;
        return inst;
    }
    PostManager(const PostManager&) = delete;
    PostManager& operator=(const PostManager&) = delete;

    /**
     * @brief Post a double-entry journal entry atomically.
     *
     * Validates:
     *  - lines.size() >= 2
     *  - SUM(amount) == 0 (balanced)
     *  - postingDate is valid
     *  - glAccount exists in chart_of_accounts
     *
     * On success: inserts one row into journal_document_header and
     * N rows into fact_universal_journal (one per line).
     *
     * @return PostingResult with success flag and document number.
     */
    [[nodiscard]] PostingResult post(const PostingRequest& req);

    /**
     * @brief Reverse an existing document (creates a mirror posting).
     * @param documentNumber e.g. "SA-2025-000042"
     * @param reversalDate Date for the reversal posting.
     * @param postedByUserId
     * @return PostingResult of the reversal document.
     */
    [[nodiscard]] PostingResult reverse(
        const QString& documentNumber,
        const QDate&   reversalDate,
        int            postedByUserId
    );

    [[nodiscard]] QString lastError() const { return m_lastError; }

private:
    PostManager() = default;

    [[nodiscard]] QString generateDocumentNumber(
        DocumentType dt,
        int fiscalYear,
        QSqlDatabase& db
    );

    mutable QString m_lastError;
    QMutex          m_seqMutex;  // Serialises document number generation
};

} // namespace edv::core::journal

using PostManager = edv::core::journal::PostManager;
