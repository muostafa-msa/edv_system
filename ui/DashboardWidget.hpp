#pragma once

#include <QWidget>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui { class DashboardWidget; }
QT_END_NAMESPACE

namespace QtCharts { class QChartView; }
class QChartView;

class DashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget *parent = nullptr);
    ~DashboardWidget() override;

public slots:
    void refresh();

private:
    void buildKpiRow();
    void buildChartsRow();
    void loadKpiData();
    void loadRevenueChart();
    void loadOrdersChart();
    void loadRecentPostings();

    QWidget* createKpiCard(const QString& title, const QString& value,
                           const QString& subtitle, const QString& color);

    Ui::DashboardWidget *ui;

    // KPI value labels (updated by refresh)
    QLabel *m_customersVal{nullptr};
    QLabel *m_ordersVal{nullptr};
    QLabel *m_lowStockVal{nullptr};
    QLabel *m_revenueVal{nullptr};

    QChartView *m_revenueChart{nullptr};
    QChartView *m_ordersChart{nullptr};
};
