#include "PredictiveReorder.hpp"
#include "core/database/DatabaseManager.hpp"

#include <QSqlQuery>
#include <QDate>
#include <cmath>
#include <numeric>

double PredictiveReorder::linearRegressionSlope(const QVector<double>& y)
{
    const int n = y.size();
    if (n < 2) return y.isEmpty() ? 0.0 : y[0];

    // x = 0,1,2,...,n-1
    double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    for (int i = 0; i < n; ++i) {
        sumX  += i;
        sumY  += y[i];
        sumXY += i * y[i];
        sumX2 += i * i;
    }
    double denom = n * sumX2 - sumX * sumX;
    if (std::abs(denom) < 1e-9) return sumY / n;
    return (n * sumXY - sumX * sumY) / denom;
}

QVector<PredictiveReorder::ReorderSuggestion> PredictiveReorder::analyse(
    int lookbackDays, int leadTimeDays, double safetyFactor) const
{
    QVector<ReorderSuggestion> suggestions;

    auto token = DatabaseManager::instance().acquire();
    if (!token) return suggestions;
    QSqlDatabase db = token->connection();

    // Fetch current stock levels
    QSqlQuery stockQ(db);
    stockQ.exec(
        "SELECT sl.material_id, sl.warehouse_id, sl.quantity_on_hand, "
        "       m.material_number, m.description, m.reorder_point, m.min_stock_level, "
        "       w.code "
        "FROM stock_levels sl "
        "JOIN materials  m ON m.id=sl.material_id "
        "JOIN warehouses w ON w.id=sl.warehouse_id "
        "WHERE m.is_active=true AND m.material_type != 'DIEN'"
    );

    const QDate cutoff = QDate::currentDate().addDays(-lookbackDays);

    while (stockQ.next()) {
        int    matId    = stockQ.value(0).toInt();
        int    whId     = stockQ.value(1).toInt();
        double stock    = stockQ.value(2).toDouble();
        QString matNum  = stockQ.value(3).toString();
        QString matDesc = stockQ.value(4).toString();
        double reorderPt= stockQ.value(5).toDouble();
        QString whCode  = stockQ.value(7).toString();

        // Daily outbound consumption over lookback window
        QSqlQuery consQ(db);
        consQ.prepare(
            "SELECT movement_date::DATE as d, ABS(SUM(quantity)) "
            "FROM inventory_ledger "
            "WHERE material_id=:mid AND warehouse_id=:wid "
            "  AND quantity < 0 "                   // outbound only
            "  AND movement_date >= :cutoff "
            "GROUP BY d ORDER BY d"
        );
        consQ.bindValue(":mid",    matId);
        consQ.bindValue(":wid",    whId);
        consQ.bindValue(":cutoff", cutoff);

        QVector<double> dailyConsumption;
        while (consQ.exec() && consQ.next())
            dailyConsumption << consQ.value(1).toDouble();

        if (dailyConsumption.isEmpty()) continue; // no consumption history

        // Average and trend
        double sum = std::accumulate(dailyConsumption.begin(), dailyConsumption.end(), 0.0);
        double avg = sum / std::max(lookbackDays, 1);
        double slope = linearRegressionSlope(dailyConsumption);

        // Project demand over lead time (use slope-adjusted forecast)
        double projectedDaily = std::max(0.0, avg + slope * leadTimeDays);
        double projectedDemand = projectedDaily * leadTimeDays;
        double safetyStock = avg * safetyFactor * leadTimeDays;
        double neededQty = projectedDemand + safetyStock - stock;

        if (neededQty <= 0 && stock > reorderPt) continue; // no action needed

        ReorderSuggestion s;
        s.materialId              = matId;
        s.warehouseId             = whId;
        s.materialNumber          = matNum;
        s.materialDescription     = matDesc;
        s.warehouseCode           = whCode;
        s.currentStock            = stock;
        s.avgDailyConsumption     = avg;
        s.projectedDemand         = projectedDemand;
        s.safetyStock             = safetyStock;
        s.suggestedOrderQty       = std::max(0.0, neededQty);
        s.urgent                  = (stock <= reorderPt && reorderPt > 0);
        suggestions.append(s);
    }

    // Sort: urgent items first, then by suggested qty descending
    std::sort(suggestions.begin(), suggestions.end(),
        [](const ReorderSuggestion& a, const ReorderSuggestion& b) {
            if (a.urgent != b.urgent) return a.urgent > b.urgent;
            return a.suggestedOrderQty > b.suggestedOrderQty;
        });

    return suggestions;
}
