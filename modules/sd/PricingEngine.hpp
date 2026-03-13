#pragma once

#include <QString>
#include <QDate>

/**
 * @brief SAP-style condition technique pricing engine.
 *
 * Evaluates a stack of pricing conditions against a material/customer/quantity
 * combination and returns the net unit price after all applicable adjustments.
 *
 * Condition type execution sequence:
 *   PR00  — base price (FIXED)
 *   K004  — customer-specific discount (PCT)
 *   KA00  — surcharge (PCT or FIXED)
 *   MWST  — tax (calculated separately in SD posting)
 *
 * All conditions are fetched from the pricing_conditions table at call time;
 * results are NOT cached (call from worker threads via QtConcurrent if needed).
 */
class PricingEngine {
public:
    static PricingEngine& instance() {
        static PricingEngine inst;
        return inst;
    }
    PricingEngine(const PricingEngine&) = delete;
    PricingEngine& operator=(const PricingEngine&) = delete;

    /**
     * @brief Calculate the net unit price for a material/customer/quantity.
     *
     * Reads active pricing_conditions in order:
     *   1. PR00 base price (material-specific or fallback to materials.base_price)
     *   2. K004 customer discount (percentage, subtracted)
     *   3. KA00 surcharges (percentage or fixed, added)
     *
     * @return Net unit price in base currency (EUR).
     */
    [[nodiscard]] double calculatePrice(int materialId, int customerId, double quantity = 1.0) const;

    /**
     * @brief Calculate price with full breakdown for display/audit purposes.
     */
    struct PriceBreakdown {
        double basePrice{0.0};
        double discountAmount{0.0};
        double surchargeAmount{0.0};
        double netUnitPrice{0.0};
        double netTotalPrice{0.0};
    };

    [[nodiscard]] PriceBreakdown calculateBreakdown(
        int materialId, int customerId, double quantity = 1.0) const;

private:
    PricingEngine() = default;
};
