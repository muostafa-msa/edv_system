#include "PricingEngine.hpp"
#include "core/database/DatabaseManager.hpp"

#include <QSqlQuery>
#include <QDate>

PricingEngine::PriceBreakdown PricingEngine::calculateBreakdown(
    int materialId, int customerId, double quantity) const
{
    PriceBreakdown result;

    auto token = DatabaseManager::instance().acquire();
    if (!token) return result;
    QSqlDatabase db = token->connection();

    // ── Step 1: Base price (PR00) ──────────────────────────────────────────
    // Check condition table first, then fall back to materials.base_price
    {
        QSqlQuery q(db);
        q.prepare(
            "SELECT amount, calc_type FROM pricing_conditions "
            "WHERE condition_type='PR00' AND material_id=:mid "
            "  AND (customer_id IS NULL OR customer_id=:cid) "
            "  AND valid_from <= CURRENT_DATE "
            "  AND (valid_to IS NULL OR valid_to >= CURRENT_DATE) "
            "  AND is_active=true "
            "ORDER BY customer_id DESC NULLS LAST LIMIT 1"
        );
        q.bindValue(":mid", materialId);
        q.bindValue(":cid", customerId);
        if (q.exec() && q.next()) {
            result.basePrice = q.value(0).toDouble();
        } else {
            // Fallback to materials table
            QSqlQuery fb(db);
            fb.prepare("SELECT base_price FROM materials WHERE id=:id");
            fb.bindValue(":id", materialId);
            if (fb.exec() && fb.next())
                result.basePrice = fb.value(0).toDouble();
        }
    }

    double runningPrice = result.basePrice;

    // ── Step 2: Customer discount (K004 — percentage) ─────────────────────
    {
        QSqlQuery q(db);
        q.prepare(
            "SELECT amount, calc_type FROM pricing_conditions "
            "WHERE condition_type='K004' AND customer_id=:cid "
            "  AND (material_id IS NULL OR material_id=:mid) "
            "  AND valid_from <= CURRENT_DATE "
            "  AND (valid_to IS NULL OR valid_to >= CURRENT_DATE) "
            "  AND is_active=true "
            "ORDER BY material_id DESC NULLS LAST LIMIT 1"
        );
        q.bindValue(":cid", customerId);
        q.bindValue(":mid", materialId);
        if (q.exec() && q.next()) {
            double pct = q.value(0).toDouble();   // stored as negative e.g. -5.0
            double disc = runningPrice * (std::abs(pct) / 100.0);
            result.discountAmount = disc;
            runningPrice -= disc;
        }
    }

    // ── Step 3: Surcharges (KA00) ─────────────────────────────────────────
    {
        QSqlQuery q(db);
        q.prepare(
            "SELECT amount, calc_type FROM pricing_conditions "
            "WHERE condition_type='KA00' "
            "  AND (material_id IS NULL OR material_id=:mid) "
            "  AND (customer_id IS NULL OR customer_id=:cid) "
            "  AND valid_from <= CURRENT_DATE "
            "  AND (valid_to IS NULL OR valid_to >= CURRENT_DATE) "
            "  AND is_active=true"
        );
        q.bindValue(":mid", materialId);
        q.bindValue(":cid", customerId);
        if (q.exec()) {
            while (q.next()) {
                const QString calcType = q.value(1).toString();
                double val = q.value(0).toDouble();
                double surcharge = (calcType == "PCT") ? runningPrice * (val / 100.0) : val;
                result.surchargeAmount += surcharge;
                runningPrice += surcharge;
            }
        }
    }

    result.netUnitPrice  = std::max(0.0, runningPrice);
    result.netTotalPrice = result.netUnitPrice * quantity;
    return result;
}

double PricingEngine::calculatePrice(int materialId, int customerId, double quantity) const
{
    return calculateBreakdown(materialId, customerId, quantity).netUnitPrice;
}
