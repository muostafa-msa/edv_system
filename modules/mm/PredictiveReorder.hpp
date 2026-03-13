#pragma once

#include <QString>
#include <QVector>
#include <QDate>

/**
 * @brief AI-based predictive reorder engine using linear regression.
 *
 * Analyses historical inventory_ledger consumption data to forecast
 * future demand and calculates optimal reorder quantities and timing.
 *
 * Algorithm:
 *   1. Fetch last N days of outbound movements (GI_SO, WA) per material/warehouse
 *   2. Fit a simple linear regression on daily consumption
 *   3. Project consumption for the next lead_time_days
 *   4. Reorder recommendation = projected_demand + safety_stock - current_stock
 *
 * All computation is synchronous; call from QtConcurrent::run() for async UI.
 */
class PredictiveReorder {
public:
    static PredictiveReorder& instance() {
        static PredictiveReorder inst;
        return inst;
    }
    PredictiveReorder(const PredictiveReorder&) = delete;
    PredictiveReorder& operator=(const PredictiveReorder&) = delete;

    struct ReorderSuggestion {
        int     materialId;
        int     warehouseId;
        QString materialNumber;
        QString materialDescription;
        QString warehouseCode;
        double  currentStock{0.0};
        double  avgDailyConsumption{0.0};
        double  projectedDemand{0.0};     // over lead_time_days
        double  safetyStock{0.0};
        double  suggestedOrderQty{0.0};
        bool    urgent{false};            // current stock < reorder_point
    };

    /**
     * @brief Analyse all materials and return reorder suggestions.
     * @param lookbackDays  Historical window for regression (default: 90 days)
     * @param leadTimeDays  Supplier lead time for projection (default: 14 days)
     * @param safetyFactor  Multiplier on avg daily consumption for safety stock (default: 1.5×lead)
     */
    [[nodiscard]] QVector<ReorderSuggestion> analyse(
        int lookbackDays = 90,
        int leadTimeDays = 14,
        double safetyFactor = 1.5
    ) const;

private:
    PredictiveReorder() = default;

    /// Simple ordinary-least-squares slope for y = a + b*x (x=day index, y=quantity)
    [[nodiscard]] static double linearRegressionSlope(const QVector<double>& y);
};
