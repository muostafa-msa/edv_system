#include "DashboardWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QScrollArea>
#include <QSqlQuery>
#include <QDateTime>

#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>

#include "core/DatabaseManager.h"

DashboardWidget::DashboardWidget(QWidget *parent) : QWidget(parent)
{
    setupUi();
    refresh();
}

void DashboardWidget::setupUi()
{
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *content = new QWidget(scroll);
    scroll->setWidget(content);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(scroll);

    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(20);

    // ---- Page header ----
    auto *hdr = new QLabel(tr("Dashboard"), content);
    QFont hf;
    hf.setPointSize(20);
    hf.setBold(true);
    hdr->setFont(hf);
    layout->addWidget(hdr);

    auto *dateLbl = new QLabel(QDateTime::currentDateTime().toString("dddd, d MMMM yyyy"), content);
    dateLbl->setObjectName("secondaryText");
    layout->addWidget(dateLbl);

    // ---- KPI cards ----
    auto *kpiRow = new QHBoxLayout;
    kpiRow->setSpacing(16);

    auto *custCard  = createKpiCard(tr("Total Customers"),   "—", tr("Active accounts"), "#1F6FEB");
    auto *orderCard = createKpiCard(tr("Open Orders"),       "—", tr("Pending fulfillment"), "#2EA043");
    auto *stockCard = createKpiCard(tr("Low Stock Items"),   "—", tr("Below minimum level"), "#D29922");
    auto *revCard   = createKpiCard(tr("Monthly Revenue"),   "— EUR", tr("Current month"), "#8957E5");

    // Grab the value labels for refresh()
    m_customersVal = custCard->findChild<QLabel*>("kpiValue");
    m_ordersVal    = orderCard->findChild<QLabel*>("kpiValue");
    m_lowStockVal  = stockCard->findChild<QLabel*>("kpiValue");
    m_revenueVal   = revCard->findChild<QLabel*>("kpiValue");

    kpiRow->addWidget(custCard);
    kpiRow->addWidget(orderCard);
    kpiRow->addWidget(stockCard);
    kpiRow->addWidget(revCard);
    layout->addLayout(kpiRow);

    // ---- Charts ----
    auto *chartsRow = new QHBoxLayout;
    chartsRow->setSpacing(16);

    // Revenue bar chart
    auto *revenueGroup = new QFrame(content);
    revenueGroup->setObjectName("card");
    auto *rgLayout = new QVBoxLayout(revenueGroup);
    rgLayout->setContentsMargins(16, 16, 16, 16);
    auto *rvTitle = new QLabel(tr("Monthly Revenue (EUR)"), revenueGroup);
    rvTitle->setObjectName("cardTitle");
    rgLayout->addWidget(rvTitle);

    m_revenueChart = new QChartView(revenueGroup);
    m_revenueChart->setMinimumHeight(240);
    m_revenueChart->setRenderHint(QPainter::Antialiasing);
    rgLayout->addWidget(m_revenueChart);
    chartsRow->addWidget(revenueGroup, 2);

    // Orders pie chart
    auto *ordersGroup = new QFrame(content);
    ordersGroup->setObjectName("card");
    auto *ogLayout = new QVBoxLayout(ordersGroup);
    ogLayout->setContentsMargins(16, 16, 16, 16);
    auto *orTitle = new QLabel(tr("Orders by Status"), ordersGroup);
    orTitle->setObjectName("cardTitle");
    ogLayout->addWidget(orTitle);

    m_ordersChart = new QChartView(ordersGroup);
    m_ordersChart->setMinimumHeight(240);
    m_ordersChart->setRenderHint(QPainter::Antialiasing);
    ogLayout->addWidget(m_ordersChart);
    chartsRow->addWidget(ordersGroup, 1);

    layout->addLayout(chartsRow);
    layout->addStretch();
}

QWidget *DashboardWidget::createKpiCard(const QString &title, const QString &value,
                                         const QString &subtitle, const QString &color)
{
    auto *card = new QFrame(this);
    card->setObjectName("card");

    auto *cl = new QVBoxLayout(card);
    cl->setContentsMargins(20, 20, 20, 20);
    cl->setSpacing(4);

    // Color bar at top
    auto *bar = new QFrame(card);
    bar->setFixedHeight(4);
    bar->setStyleSheet(QString("background: %1; border-radius: 2px;").arg(color));
    cl->addWidget(bar);
    cl->addSpacing(8);

    auto *titleLbl = new QLabel(title, card);
    titleLbl->setObjectName("kpiTitle");

    auto *valLbl = new QLabel(value, card);
    valLbl->setObjectName("kpiValue");
    QFont vf;
    vf.setPointSize(28);
    vf.setBold(true);
    valLbl->setFont(vf);
    valLbl->setStyleSheet(QString("color: %1;").arg(color));

    auto *subLbl = new QLabel(subtitle, card);
    subLbl->setObjectName("kpiSubtitle");

    cl->addWidget(titleLbl);
    cl->addWidget(valLbl);
    cl->addWidget(subLbl);

    return card;
}

void DashboardWidget::refresh()
{
    loadKpiData();

    // ---- Revenue bar chart ----
    auto *barSet = new QBarSet(tr("Revenue"));
    barSet->setColor(QColor("#1F6FEB"));

    QStringList months;
    QSqlQuery q(DatabaseManager::instance().database());
    q.exec(
        "SELECT TO_CHAR(order_date,'Mon') AS month, COALESCE(SUM(total_gross),0) "
        "FROM sales_orders "
        "WHERE order_date >= DATE_TRUNC('year', CURRENT_DATE) "
        "  AND status NOT IN ('cancelled','draft') "
        "GROUP BY 1, DATE_PART('month', order_date) "
        "ORDER BY DATE_PART('month', order_date)"
    );
    while (q.next()) {
        months << q.value(0).toString();
        *barSet << q.value(1).toDouble();
    }

    // If no data, show sample
    if (months.isEmpty()) {
        for (auto &m : {"Jan","Feb","Mar","Apr","May","Jun"}) {
            months << m;
            *barSet << 0;
        }
    }

    auto *series = new QBarSeries;
    series->append(barSet);

    auto *chart = new QChart;
    chart->addSeries(series);
    chart->setTitle("");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->hide();
    chart->setBackgroundVisible(false);

    auto *axisX = new QBarCategoryAxis;
    axisX->append(months);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto *axisY = new QValueAxis;
    axisY->setLabelFormat("%.0f");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    m_revenueChart->setChart(chart);

    // ---- Orders pie chart ----
    auto *pie = new QPieSeries;
    QSqlQuery pq(DatabaseManager::instance().database());
    pq.exec(
        "SELECT status, COUNT(*) FROM sales_orders "
        "WHERE status NOT IN ('cancelled') "
        "GROUP BY status ORDER BY status"
    );

    bool hasPieData = false;
    QStringList colors = {"#1F6FEB","#2EA043","#D29922","#8957E5","#F85149"};
    int ci = 0;
    while (pq.next()) {
        hasPieData = true;
        auto *sl = pie->append(pq.value(0).toString(), pq.value(1).toDouble());
        sl->setColor(QColor(colors.value(ci++ % colors.size())));
        sl->setLabelVisible(true);
    }
    if (!hasPieData)
        pie->append(tr("No Data"), 1.0);

    auto *pieChart = new QChart;
    pieChart->addSeries(pie);
    pieChart->setTitle("");
    pieChart->setAnimationOptions(QChart::SeriesAnimations);
    pieChart->legend()->setAlignment(Qt::AlignBottom);
    pieChart->setBackgroundVisible(false);

    m_ordersChart->setChart(pieChart);
}

void DashboardWidget::loadKpiData()
{
    auto db = DatabaseManager::instance().database();

    // Total active customers
    {
        QSqlQuery q(db);
        q.exec("SELECT COUNT(*) FROM customers WHERE is_active=true");
        if (q.next() && m_customersVal)
            m_customersVal->setText(q.value(0).toString());
    }

    // Open orders
    {
        QSqlQuery q(db);
        q.exec("SELECT COUNT(*) FROM sales_orders WHERE status IN ('draft','confirmed','shipped')");
        if (q.next() && m_ordersVal)
            m_ordersVal->setText(q.value(0).toString());
    }

    // Low stock
    {
        QSqlQuery q(db);
        q.exec(
            "SELECT COUNT(*) FROM stock_levels "
            "WHERE quantity_on_hand < min_stock AND min_stock > 0"
        );
        if (q.next() && m_lowStockVal)
            m_lowStockVal->setText(q.value(0).toString());
    }

    // Monthly revenue
    {
        QSqlQuery q(db);
        q.exec(
            "SELECT COALESCE(SUM(total_gross),0) FROM sales_orders "
            "WHERE DATE_TRUNC('month', order_date) = DATE_TRUNC('month', CURRENT_DATE) "
            "  AND status NOT IN ('cancelled','draft')"
        );
        if (q.next() && m_revenueVal)
            m_revenueVal->setText(QString("%1 EUR").arg(q.value(0).toDouble(), 0, 'f', 0));
    }
}
