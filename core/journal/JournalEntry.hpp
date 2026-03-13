#pragma once

#include <QString>
#include <QDate>
#include <QVector>
#include <QUuid>
#include <optional>

namespace edv::core::journal {

/**
 * @brief SAP-style document types for the Universal Journal.
 */
enum class DocumentType {
    SA,  ///< GL journal entry (Sachkonten)
    DR,  ///< Customer invoice (Debitor)
    KR,  ///< Vendor invoice (Kreditor)
    WE,  ///< Goods receipt (Wareneingang)
    WA,  ///< Goods issue (Warenausgang)
    PR,  ///< Production order posting
    HR,  ///< HR / payroll posting
};

inline QString docTypeToString(DocumentType dt) {
    switch (dt) {
        case DocumentType::SA: return "SA";
        case DocumentType::DR: return "DR";
        case DocumentType::KR: return "KR";
        case DocumentType::WE: return "WE";
        case DocumentType::WA: return "WA";
        case DocumentType::PR: return "PR";
        case DocumentType::HR: return "HR";
    }
    return "SA";
}

/**
 * @brief A single debit or credit line within a journal entry.
 *
 * Debit lines: amount > 0
 * Credit lines: amount < 0
 *
 * Constraint: SUM(amount) across all lines == 0 (double-entry invariant).
 */
struct JournalLine {
    int      glAccount{0};         ///< Chart of accounts GL code
    double   amount{0.0};          ///< Positive = debit, negative = credit
    QString  currency{"EUR"};
    QString  costCenter;           ///< Optional controlling object
    QString  profitCenter;
    QString  text;                 ///< Line item text / description

    // Reference dimensions
    std::optional<int> customerId;
    std::optional<int> vendorId;
    std::optional<int> materialId;
    std::optional<int> employeeId;
    std::optional<int> salesOrderId;
    std::optional<int> purchaseOrderId;
};

/**
 * @brief A complete double-entry journal entry header + lines.
 *
 * Build one of these and pass it to PostManager::post() for
 * ACID atomic persistence into fact_universal_journal.
 */
struct PostingRequest {
    DocumentType     documentType{DocumentType::SA};
    QDate            postingDate;
    QDate            documentDate;
    QString          reference;          ///< External document number
    QString          headerText;         ///< Posting description
    QString          currency{"EUR"};
    int              postedByUserId{0};  ///< Session user id
    QVector<JournalLine> lines;

    /// Validate debit = credit and minimum 2 lines.
    [[nodiscard]] bool isBalanced() const noexcept {
        if (lines.size() < 2) return false;
        double sum = 0.0;
        for (const auto& l : lines) sum += l.amount;
        return std::abs(sum) < 1e-6;
    }
};

/**
 * @brief Result returned by PostManager::post().
 */
struct PostingResult {
    bool    success{false};
    QString documentNumber;   ///< e.g. "SA-2025-000042"
    QString errorMessage;

    explicit operator bool() const noexcept { return success; }
};

} // namespace edv::core::journal

using DocumentType   = edv::core::journal::DocumentType;
using JournalLine    = edv::core::journal::JournalLine;
using PostingRequest = edv::core::journal::PostingRequest;
using PostingResult  = edv::core::journal::PostingResult;
