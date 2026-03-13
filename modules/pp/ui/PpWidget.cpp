#include "PpWidget.hpp"
#include "ui_PpWidget.h"
#include "core/database/DatabaseManager.hpp"

#include <QSqlQuery>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDateEdit>

PpWidget::PpWidget(const Session& session, QWidget *parent)
    : QWidget(parent), ui(new Ui::PpWidget), m_session(session)
{
    ui->setupUi(this);
    ui->ordersTable->horizontalHeader()->setStretchLastSection(true);

    connect(ui->refreshBtn,  &QPushButton::clicked, this, &PpWidget::onRefresh);
    connect(ui->newOrderBtn, &QPushButton::clicked, this, &PpWidget::onNewOrder);
    connect(ui->releaseBtn,  &QPushButton::clicked, this, &PpWidget::onRelease);
    connect(ui->ordersTable, &QTableWidget::currentCellChanged,
            this, [this](int row, int, int, int){ onOrderSelected(row); });

    if (!session.hasRole(edv::core::security::Role::Manager)) {
        ui->newOrderBtn->hide();
        ui->releaseBtn->hide();
    }
    loadOrders();
}

PpWidget::~PpWidget() { delete ui; }
void PpWidget::onRefresh() { loadOrders(); }

void PpWidget::loadOrders()
{
    auto* tbl = ui->ordersTable;
    tbl->setRowCount(0);

    auto token = DatabaseManager::instance().acquire();
    if (!token) return;

    QString sql =
        "SELECT po.id, po.order_number, m.description, "
        "       po.planned_qty, po.produced_qty, po.start_date, po.end_date, po.status "
        "FROM production_orders po "
        "JOIN materials m ON m.id=po.material_id ";

    const QString status = ui->statusFilter->currentText();
    const QString search = ui->searchEdit->text().trimmed();
    QStringList where;
    if (status != "All") where << "po.status='" + status + "'";
    if (!search.isEmpty()) where << "(po.order_number ILIKE '%" + search + "%' OR m.description ILIKE '%" + search + "%')";
    if (!where.isEmpty()) sql += "WHERE " + where.join(" AND ") + " ";
    sql += "ORDER BY po.id DESC";

    QSqlQuery q(token->connection());
    q.exec(sql);

    int row = 0;
    while (q.next()) {
        tbl->insertRow(row);
        tbl->setItem(row, 0, new QTableWidgetItem(q.value(1).toString())); // order_number
        tbl->setItem(row, 1, new QTableWidgetItem(q.value(2).toString())); // material
        tbl->setItem(row, 2, new QTableWidgetItem(q.value(3).toString())); // planned
        tbl->setItem(row, 3, new QTableWidgetItem(q.value(4).toString())); // produced
        tbl->setItem(row, 4, new QTableWidgetItem(q.value(5).toDate().toString("yyyy-MM-dd")));
        tbl->setItem(row, 5, new QTableWidgetItem(q.value(6).toDate().toString("yyyy-MM-dd")));
        tbl->setItem(row, 6, new QTableWidgetItem(q.value(7).toString()));
        tbl->setItem(row, 7, new QTableWidgetItem(q.value(0).toString())); // hidden id

        const QString st = q.value(7).toString();
        if (st == "completed") tbl->item(row, 6)->setForeground(QColor("#2EA043"));
        else if (st == "in_progress") tbl->item(row, 6)->setForeground(QColor("#1F6FEB"));
        ++row;
    }
}

void PpWidget::onOrderSelected(int row)
{
    if (row < 0 || row >= ui->ordersTable->rowCount()) return;
    // We need to get the material_id via order number
    auto* numItem = ui->ordersTable->item(row, 0);
    if (!numItem) return;

    auto token = DatabaseManager::instance().acquire();
    if (!token) return;
    QSqlQuery q(token->connection());
    q.prepare("SELECT material_id FROM production_orders WHERE order_number=:n");
    q.bindValue(":n", numItem->text());
    if (q.exec() && q.next()) loadBom(q.value(0).toInt());
}

void PpWidget::loadBom(int materialId)
{
    auto* tbl = ui->bomTable;
    tbl->setRowCount(0);
    tbl->horizontalHeader()->setStretchLastSection(true);

    auto token = DatabaseManager::instance().acquire();
    if (!token) return;
    QSqlQuery q(token->connection());
    q.prepare(
        "SELECT m.material_number, m.description, b.quantity, b.unit_of_measure "
        "FROM bill_of_materials b "
        "JOIN materials m ON m.id=b.component_material_id "
        "WHERE b.parent_material_id=:id ORDER BY m.material_number"
    );
    q.bindValue(":id", materialId);
    if (!q.exec()) return;

    int row = 0;
    while (q.next()) {
        tbl->insertRow(row);
        tbl->setItem(row, 0, new QTableWidgetItem(q.value(0).toString()));
        tbl->setItem(row, 1, new QTableWidgetItem(q.value(1).toString()));
        tbl->setItem(row, 2, new QTableWidgetItem(q.value(2).toString()));
        tbl->setItem(row, 3, new QTableWidgetItem(q.value(3).toString()));
        ++row;
    }
    if (row == 0) {
        tbl->insertRow(0);
        tbl->setItem(0, 1, new QTableWidgetItem(tr("No BOM defined for this material")));
    }
}

void PpWidget::onNewOrder()
{
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle(tr("Create Production Order"));
    auto* form = new QFormLayout(dlg);
    form->setSpacing(12); form->setContentsMargins(24,24,24,24);

    auto* matCombo = new QComboBox(dlg);
    auto* qtyEdit  = new QDoubleSpinBox(dlg); qtyEdit->setRange(1, 999999); qtyEdit->setValue(1);
    auto* startDate= new QDateEdit(QDate::currentDate(), dlg); startDate->setCalendarPopup(true);
    auto* endDate  = new QDateEdit(QDate::currentDate().addDays(7), dlg); endDate->setCalendarPopup(true);

    {
        auto token = DatabaseManager::instance().acquire();
        if (token) {
            QSqlQuery q(token->connection());
            q.exec("SELECT id, material_number, description FROM materials WHERE is_active=true AND material_type IN ('FERT','HALB') ORDER BY material_number");
            while (q.next())
                matCombo->addItem(q.value(1).toString() + " - " + q.value(2).toString(), q.value(0).toInt());
        }
    }

    form->addRow(tr("Material:"),    matCombo);
    form->addRow(tr("Planned Qty:"), qtyEdit);
    form->addRow(tr("Start Date:"),  startDate);
    form->addRow(tr("End Date:"),    endDate);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dlg);
    form->addRow(btns);
    connect(btns, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

    if (dlg->exec() != QDialog::Accepted) return;

    auto token = DatabaseManager::instance().acquire();
    if (!token) return;

    QSqlQuery seqQ(token->connection());
    seqQ.exec("SELECT NEXTVAL('seq_sales_order')"); // reuse sequence
    const QString orderNum = seqQ.next()
        ? QString("PP-%1").arg(seqQ.value(0).toLongLong(), 6, 10, QChar('0'))
        : "PP-000001";

    QSqlQuery ins(token->connection());
    ins.prepare(
        "INSERT INTO production_orders "
        "(order_number,material_id,planned_qty,start_date,end_date,status,created_by_user) "
        "VALUES (:on,:mid,:qty,:sd,:ed,'planned',:uid)"
    );
    ins.bindValue(":on",  orderNum);
    ins.bindValue(":mid", matCombo->currentData().toInt());
    ins.bindValue(":qty", qtyEdit->value());
    ins.bindValue(":sd",  startDate->date());
    ins.bindValue(":ed",  endDate->date());
    ins.bindValue(":uid", m_session.userId);
    if (ins.exec())
        QMessageBox::information(this, tr("Created"), tr("Production order %1 created.").arg(orderNum));
    loadOrders();
}

void PpWidget::onRelease()
{
    const int row = ui->ordersTable->currentRow();
    if (row < 0) return;
    const QString orderNum = ui->ordersTable->item(row, 0)->text();

    auto token = DatabaseManager::instance().acquire();
    if (!token) return;
    QSqlQuery q(token->connection());
    q.prepare("UPDATE production_orders SET status='released' WHERE order_number=:n AND status='planned'");
    q.bindValue(":n", orderNum);
    q.exec();
    loadOrders();
}
