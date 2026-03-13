#include "DashboardWidget.hpp"
#include "ui_DashboardWidget.h"
#include "core/database/DatabaseManager.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QFont>
#include <QDate>
#include <QSqlQuery>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QPainter>

#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>

DashboardWidget::DashboardWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::DashboardWidget)
{
    ui->setupUi(this);
    buildKpiRow();
    buildChartsRow();
    refresh();
}

DashboardWidget::~DashboardWidget() { delete ui; }

// ── KPI cards ─────────────────────────────────────────────────────────────────

QWidget* DashboardWidget::createKpiCard(const QString& title, const QString& value,
                                         const QString& subtitle, const QString& color)
{
    auto* card = new QFrame(this);
    card->setObjectName("card");

    auto* cl = new QVBoxLayout(card);
    cl->setContentsMargins(20, 20, 20, 20);
    cl->setSpacing(6);

    auto* bar = new QFrame(card);
    bar->setFixedHeight(4);
    bar->setStyleSheet(QString("background:%1; border-radius:2px;").arg(color));
    cl->addWidget(bar);
    cl->addSpacing(6);

    auto* titleLbl = new QLabel(title, card);
    titleLbl->setObjectName("kpiTitle");

    auto* valLbl = new QLabel(value, card);
    valLbl->setObjectName("kpiValue");
    QFont vf; vf.setPointSize(26); vf.setBold(true);
    valLbl->setFont(vf);
    valLbl->setStyleSheet(QString("color:%1;").arg(color));

    auto* subLbl = new QLabel(subtitle, card);
    subLbl->setObjectName("kpiSubtitle");

    cl->addWidget(titleLbl);
    cl->addWidget(valLbl);
    cl->addWidget(subLbl);
    return card;
}

void DashboardWidget::buildKpiRow()
{
    auto* row = new QHBoxLayout(ui->kpiRow);
    row->setSpacing(16);

    auto* c1 = createKpiCard(tr("Total Customers"),  "—",      tr("Active accounts"),      "#1F6FEB");
    auto* c2 = createKpiCard(tr("Open Orders"),      "—",      tr("Pending fulfillment"),   "#2EA043");
    auto* c3 = createKpiCard(tr("Low Stock Items"),  "—",      tr("Below reorder point"),   "#D29922");
    auto* c4 = createKpiCard(tr("Monthly Revenue"),  "— EUR",  tr("Current month"),         "#8957E5");

    m_customersVal = c1->findChild<QLabel*>("kpiValue");
    m_ordersVal    = c2->findChild<QLabel*>("kpiValue");
    m_lowStockVal  = c3->findChild<QLabel*>("kpiValue");
    m_revenueVal   = c4->findChild<QLabel*>("kpiValue");

    row->addWidget(c1); row->addWidget(c2);
    row->addWidget(c3); row->addWidget(c4);
}

void DashboardWidget::buildChartsRow()
{
    auto* row = new QHBoxLayout(ui->chartsRow);
    row->setSpacing(16);

    auto* revCard = new QFrame(this);
    revCard->setObjectName("card");
    auto* revLayout = new QVBoxLayout(revCard);
    revLayout->setContentsMargins(16,16,16,16);
    auto* revTitle = new QLabel(tr("Monthly Revenue (EUR)"), revCard);
    revTitle->setObjectName("cardTitle");
    m_revenueChart = new QChartView(revCard);
    m_revenueChart->setMinimumHeight(260);
    m_revenueChart->setRenderHint(QPainter::Antialiasing);
    revLayout->addWidget(revTitle);
    revLayout->addWidget(m_revenueChart);
    row->addWidget(revCard, 3);

    auto* ordCard = new QFrame(this);
    ordCard->setObjectName("card");
    auto* ordLayout = new QVBoxLayout(ordCard);
    ordLayout->setContentsMargins(16,16,16,16);
    auto* ordTitle = new QLabel(tr("Orders by Status"), ordCard);
    ordTitle->setObjectName("cardTitle");
    m_ordersChart = new QChartView(ordCard);
    m_ordersChart->setMinimumHeight(260);
    m_ordersChart->setRenderHint(QPainter::Antialiasing);
    ordLayout->addWidget(ordTitle);
    ordLayout->addWidget(m_ordersChart);
    row->addWidget(ordCard, 2);
}

// ── Refresh ───────────────────────────────────────────────────────────────────

void DashboardWidget::refresh()
{
    loadKpiData();
    loadRevenueChart();
    loadOrdersChart();
    loadRecentPostings();
}

void DashboardWidget::loadKpiData()
{
    auto token = DatabaseManager::instance().acquire();
    if (!token) return;
    QSqlDatabase db = token->connection();

    {
        QSqlQuery q(db);
        q.exec("SELECT COUNT(*) FROM customers WHERE is_active=true");
        if (q.next() && m_customersVal) m_customersVal->setText(q.value(0).toString());
    }
    {
        QSqlQuery q(db);
        q.exec("SELECT COUNT(*) FROM sales_orders WHERE status IN ('confirmed','picking','shipped')");
        if (q.next() && m_ordersVal) m_ordersVal->setText(q.value(0).toString());
    }
    {
        QSqlQuery q(db);
        q.exec("SELECT COUNT(*) FROM stock_levels WHERE quantity_on_hand < reorder_point AND reorder_point > 0");
        if (q.next() && m_lowStockVal) m_lowStockVal->setText(q.value(0).toString());
    }
    {
        QSqlQuery q(db);
        q.exec(
            "SELECT COALESCE(ABS(SUM(f.amount)),0) "
            "FROM fact_universal_journal f "
            "JOIN chart_of_accounts c ON c.gl_code=f.gl_account "
            "WHERE c.account_type='Revenue' "
            "  AND DATE_TRUNC('month',f.posting_date)=DATE_TRUNC('month',CURRENT_DATE)"
        );
        if (q.next() && m_revenueVal)
            m_revenueVal->setText(QString("%1 EUR").arg(q.value(0).toDouble(), 0, 'f', 0));
    }
}

void DashboardWidget::loadRevenueChart()
{
    auto token = DatabaseManager::instance().acquire();
    if (!token) return;

    auto* barSet = new QBarSet(tr("Revenue"));
    barSet->setColor(QColor("#1F6FEB"));

    QStringList months;
    QSqlQuery q(token->connection());
    q.exec(
        "SELECT TO_CHAR(f.posting_date,'Mon') AS month, "
        "       COALESCE(ABS(SUM(f.amount)),0) "
        "FROM fact_universal_journal f "
        "JOIN chart_of_accounts c ON c.gl_code=f.gl_account "
        "WHERE c.account_type='Revenue' "
        "  AND DATE_PART('year',f.posting_date)=DATE_PART('year',CURRENT_DATE) "
        "GROUP BY 1, DATE_PART('month',f.posting_date) "
        "ORDER BY DATE_PART('month',f.posting_date)"
    );
    while (q.next()) {
        months << q.value(0).toString();
        *barSet << q.value(1).toDouble();
    }
    if (months.isEmpty()) {
        for (const char* m : {"Jan","Feb","Mar","Apr","May","Jun"}) { months << m; *barSet << 0; }
    }

    auto* series = new QBarSeries; series->append(barSet);
    auto* chart  = new QChart;
    chart->addSeries(series);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->hide();
    chart->setBackgroundVisible(false);
    auto* axX = new QBarCategoryAxis; axX->append(months);
    chart->addAxis(axX, Qt::AlignBottom); series->attachAxis(axX);
    auto* axY = new QValueAxis; axY->setLabelFormat("%.0f");
    chart->addAxis(axY, Qt::AlignLeft); series->attachAxis(axY);
    m_revenueChart->setChart(chart);
}

void DashboardWidget::loadOrdersChart()
{
    auto token = DatabaseManager::instance().acquire();
    if (!token) return;

    auto* pie = new QPieSeries;
    QSqlQuery q(token->connection());
    q.exec("SELECT status, COUNT(*) FROM sales_orders WHERE status!='cancelled' GROUP BY status");

    static const QStringList colors = {"#1F6FEB","#2EA043","#D29922","#8957E5","#F85149"};
    int ci = 0; bool hasData = false;
    while (q.next()) {
        hasData = true;
        auto* sl = pie->append(q.value(0).toString(), q.value(1).toDouble());
        sl->setColor(QColor(colors.value(ci++ % colors.size())));
        sl->setLabelVisible(true);
    }
    if (!hasData) pie->append(tr("No Data"), 1.0);

    auto* chart = new QChart;
    chart->addSeries(pie);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->setBackgroundVisible(false);
    m_ordersChart->setChart(chart);
}

void DashboardWidget::loadRecentPostings()
{
    auto token = DatabaseManager::instance().acquire();
    if (!token) return;

    auto* tbl = ui->activityTable;
    tbl->setRowCount(0);
    tbl->horizontalHeader()->setStretchLastSection(true);

    QSqlQuery q(token->connection());
    q.exec(
        "SELECT h.document_number, h.document_type, h.posting_date, "
        "       SUM(CASE WHEN f.amount>0 THEN f.amount ELSE 0 END), h.header_text "
        "FROM journal_document_header h "
        "JOIN fact_universal_journal f ON f.document_number=h.document_number "
        "GROUP BY h.document_number, h.document_type, h.posting_date, h.header_text "
        "ORDER BY h.created_at DESC LIMIT 20"
    );

    int row = 0;
    while (q.next()) {
        tbl->insertRow(row);
        tbl->setItem(row, 0, new QTableWidgetItem(q.value(0).toString()));
        tbl->setItem(row, 1, new QTableWidgetItem(q.value(1).toString()));
        tbl->setItem(row, 2, new QTableWidgetItem(q.value(2).toDate().toString("yyyy-MM-dd")));
        tbl->setItem(row, 3, new QTableWidgetItem(
            QString("%1 EUR").arg(q.value(3).toDouble(), 0, 'f', 2)));
        tbl->setItem(row, 4, new QTableWidgetItem(q.value(4).toString()));
        ++row;
    }
}
