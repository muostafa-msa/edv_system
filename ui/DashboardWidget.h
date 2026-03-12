#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>

class QLabel;
class QChartView;

class DashboardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget *parent = nullptr);

public slots:
    void refresh();

private:
    void setupUi();
    QWidget *createKpiCard(const QString &title, const QString &value,
                           const QString &subtitle, const QString &color);
    void loadKpiData();

    QLabel *m_customersVal;
    QLabel *m_ordersVal;
    QLabel *m_lowStockVal;
    QLabel *m_revenueVal;
    QChartView *m_revenueChart;
    QChartView *m_ordersChart;
};

#endif // DASHBOARDWIDGET_H
