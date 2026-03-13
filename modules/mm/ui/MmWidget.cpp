#include "MmWidget.hpp"
#include "ui_MmWidget.h"
#include "core/database/DatabaseManager.hpp"
#include "core/journal/PostManager.hpp"

#include <QSqlQuery>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDateTime>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>

MmWidget::MmWidget(const Session& session, QWidget *parent)
    : QWidget(parent), ui(new Ui::MmWidget), m_session(session)
{
    ui->setupUi(this);

    // Populate port combo
    const auto ports = WmsScanner::availablePorts();
    for (const auto& p : ports) ui->portCombo->addItem(p);

    // Populate warehouse filter
    {
        auto token = DatabaseManager::instance().acquire();
        if (token) {
            QSqlQuery q(token->connection());
            q.exec("SELECT code FROM warehouses WHERE is_active=true ORDER BY code");
            while (q.next()) ui->warehouseFilter->addItem(q.value(0).toString());
        }
    }

    connect(ui->stockRefreshBtn, &QPushButton::clicked, this, &MmWidget::loadStockLevels);
    connect(ui->goodsReceiptBtn, &QPushButton::clicked, this, &MmWidget::onGoodsReceipt);
    connect(ui->goodsIssueBtn,   &QPushButton::clicked, this, &MmWidget::onGoodsIssue);
    connect(ui->connectPortBtn,  &QPushButton::clicked, this, &MmWidget::onConnectPort);
    connect(ui->disconnectPortBtn,&QPushButton::clicked,this, &MmWidget::onDisconnectPort);
    connect(ui->testScanBtn,     &QPushButton::clicked, this, &MmWidget::onTestScan);
    connect(ui->runReorderBtn,   &QPushButton::clicked, this, &MmWidget::onRunReorderAnalysis);
    connect(&m_scanner, &WmsScanner::barcodeScanned, this, &MmWidget::onBarcodeScanned);

    if (!session.hasRole(edv::core::security::Role::Manager)) {
        ui->goodsReceiptBtn->hide();
        ui->goodsIssueBtn->hide();
    }

    loadStockLevels();
}

MmWidget::~MmWidget() { delete ui; }

void MmWidget::loadStockLevels()
{
    auto* tbl = ui->stockTable;
    tbl->setRowCount(0);
    tbl->horizontalHeader()->setStretchLastSection(true);
    tbl->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    auto token = DatabaseManager::instance().acquire();
    if (!token) return;

    QString sql =
        "SELECT m.material_number, m.description, w.code, "
        "       sl.quantity_on_hand, m.unit_of_measure, sl.reorder_point "
        "FROM stock_levels sl "
        "JOIN materials  m ON m.id=sl.material_id "
        "JOIN warehouses w ON w.id=sl.warehouse_id "
        "WHERE m.is_active=true ";

    const QString wh = ui->warehouseFilter->currentText();
    if (wh != "All Warehouses") sql += "AND w.code='" + wh + "' ";

    const QString search = ui->stockSearchEdit->text().trimmed();
    if (!search.isEmpty())
        sql += "AND (m.material_number ILIKE '%" + search + "%' OR m.description ILIKE '%" + search + "%') ";

    sql += "ORDER BY m.material_number";

    QSqlQuery q(token->connection());
    q.exec(sql);

    int row = 0;
    while (q.next()) {
        tbl->insertRow(row);
        double stock   = q.value(3).toDouble();
        double reorder = q.value(5).toDouble();
        bool   low     = (reorder > 0 && stock <= reorder);

        for (int c = 0; c < 7; ++c) {
            QString val;
            switch(c) {
                case 0: val = q.value(0).toString(); break;
                case 1: val = q.value(1).toString(); break;
                case 2: val = q.value(2).toString(); break;
                case 3: val = QString("%1").arg(stock, 0,'f',2); break;
                case 4: val = q.value(4).toString(); break;
                case 5: val = QString("%1").arg(reorder, 0,'f',0); break;
                case 6: val = low ? tr("⚠ Low Stock") : tr("OK"); break;
            }
            auto* it = new QTableWidgetItem(val);
            if (c >= 3) it->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            if (c == 6) it->setForeground(low ? QColor("#D29922") : QColor("#2EA043"));
            tbl->setItem(row, c, it);
        }
        ++row;
    }
}

void MmWidget::onGoodsReceipt()
{
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle(tr("Post Goods Receipt"));
    auto* form = new QFormLayout(dlg);
    form->setSpacing(12); form->setContentsMargins(24,24,24,24);

    auto* matCombo = new QComboBox(dlg);
    auto* whCombo  = new QComboBox(dlg);
    auto* qtyEdit  = new QDoubleSpinBox(dlg);
    qtyEdit->setRange(0.001, 999999); qtyEdit->setValue(1.0);
    auto* refEdit  = new QLineEdit(dlg);

    {
        auto token = DatabaseManager::instance().acquire();
        if (token) {
            QSqlQuery qm(token->connection());
            qm.exec("SELECT id, material_number, description FROM materials WHERE is_active=true ORDER BY material_number");
            while (qm.next())
                matCombo->addItem(QString("%1 - %2").arg(qm.value(1).toString(), qm.value(2).toString()), qm.value(0).toInt());
            QSqlQuery qw(token->connection());
            qw.exec("SELECT id, code FROM warehouses WHERE is_active=true ORDER BY code");
            while (qw.next())
                whCombo->addItem(qw.value(1).toString(), qw.value(0).toInt());
        }
    }

    form->addRow(tr("Material:"),   matCombo);
    form->addRow(tr("Warehouse:"),  whCombo);
    form->addRow(tr("Quantity:"),   qtyEdit);
    form->addRow(tr("Reference:"),  refEdit);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dlg);
    form->addRow(btns);
    connect(btns, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

    if (dlg->exec() != QDialog::Accepted) return;

    postStockMovement(matCombo->currentData().toInt(),
                      whCombo->currentData().toInt(),
                      qtyEdit->value(), "GR_PO",
                      refEdit->text().trimmed());
    loadStockLevels();
}

void MmWidget::onGoodsIssue()
{
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle(tr("Post Goods Issue"));
    auto* form = new QFormLayout(dlg);
    form->setSpacing(12); form->setContentsMargins(24,24,24,24);

    auto* matCombo = new QComboBox(dlg);
    auto* whCombo  = new QComboBox(dlg);
    auto* qtyEdit  = new QDoubleSpinBox(dlg);
    qtyEdit->setRange(0.001, 999999); qtyEdit->setValue(1.0);
    auto* refEdit  = new QLineEdit(dlg);

    {
        auto token = DatabaseManager::instance().acquire();
        if (token) {
            QSqlQuery qm(token->connection());
            qm.exec("SELECT id, material_number, description FROM materials WHERE is_active=true ORDER BY material_number");
            while (qm.next())
                matCombo->addItem(QString("%1 - %2").arg(qm.value(1).toString(), qm.value(2).toString()), qm.value(0).toInt());
            QSqlQuery qw(token->connection());
            qw.exec("SELECT id, code FROM warehouses WHERE is_active=true ORDER BY code");
            while (qw.next())
                whCombo->addItem(qw.value(1).toString(), qw.value(0).toInt());
        }
    }

    form->addRow(tr("Material:"),   matCombo);
    form->addRow(tr("Warehouse:"),  whCombo);
    form->addRow(tr("Quantity:"),   qtyEdit);
    form->addRow(tr("Reference:"),  refEdit);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dlg);
    form->addRow(btns);
    connect(btns, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

    if (dlg->exec() != QDialog::Accepted) return;

    postStockMovement(matCombo->currentData().toInt(),
                      whCombo->currentData().toInt(),
                      -qtyEdit->value(), "GI_SO",  // negative = outbound
                      refEdit->text().trimmed());
    loadStockLevels();
}

void MmWidget::postStockMovement(int materialId, int warehouseId,
                                  double quantity, const QString& movementType,
                                  const QString& reference)
{
    auto token = DatabaseManager::instance().acquire();
    if (!token) return;

    // Fetch GL accounts
    QSqlQuery q(token->connection());
    q.prepare("SELECT gl_inventory_account, base_price FROM materials WHERE id=:id");
    q.bindValue(":id", materialId);
    if (!q.exec() || !q.next()) return;
    int    glInv    = q.value(0).toInt();
    double unitCost = q.value(1).toDouble();
    double totalCost= std::abs(quantity) * unitCost;

    // Insert ledger record
    {
        QSqlQuery ins(token->connection());
        ins.prepare(
            "INSERT INTO inventory_ledger "
            "(material_id, warehouse_id, movement_type, quantity, unit_cost, reference_doc, created_by_user) "
            "VALUES (:mid, :wid, :mt, :qty, :uc, :ref, :uid)"
        );
        ins.bindValue(":mid",  materialId);
        ins.bindValue(":wid",  warehouseId);
        ins.bindValue(":mt",   movementType);
        ins.bindValue(":qty",  quantity);
        ins.bindValue(":uc",   unitCost);
        ins.bindValue(":ref",  reference);
        ins.bindValue(":uid",  m_session.userId);
        ins.exec();
    }

    // Post journal (WE = goods receipt, WA = goods issue)
    if (glInv > 0 && totalCost > 0) {
        PostingRequest req;
        req.documentType   = (quantity > 0) ? DocumentType::WE : DocumentType::WA;
        req.postingDate    = QDate::currentDate();
        req.documentDate   = QDate::currentDate();
        req.reference      = reference;
        req.headerText     = movementType + " — " + reference;
        req.postedByUserId = m_session.userId;

        if (quantity > 0) {
            // GR: Debit Inventory, Credit GR/IR (use COGS as clearing)
            JournalLine dr; dr.glAccount = glInv; dr.amount = totalCost;
            dr.materialId = materialId; req.lines.append(dr);
            JournalLine cr; cr.glAccount = 5000; cr.amount = -totalCost;
            cr.materialId = materialId; req.lines.append(cr);
        } else {
            // GI: Debit COGS, Credit Inventory
            JournalLine dr; dr.glAccount = 5000; dr.amount = totalCost;
            dr.materialId = materialId; req.lines.append(dr);
            JournalLine cr; cr.glAccount = glInv; cr.amount = -totalCost;
            cr.materialId = materialId; req.lines.append(cr);
        }
        (void)PostManager::instance().post(req);
    }
}

void MmWidget::onConnectPort()
{
    const QString port = ui->portCombo->currentText();
    if (port.isEmpty()) {
        QMessageBox::warning(this, tr("No Port"), tr("No serial port selected."));
        return;
    }
    if (m_scanner.openPort(port)) {
        ui->portStatusLabel->setText(tr("● Connected: ") + port);
        ui->portStatusLabel->setStyleSheet("color:#2EA043;");
    } else {
        ui->portStatusLabel->setText(tr("● Error: ") + m_scanner.lastError());
        ui->portStatusLabel->setStyleSheet("color:#F85149;");
    }
}

void MmWidget::onDisconnectPort()
{
    m_scanner.closePort();
    ui->portStatusLabel->setText(tr("● Not connected"));
    ui->portStatusLabel->setStyleSheet("");
}

void MmWidget::onTestScan()
{
    m_scanner.emitTestBarcode("MAT000001");
}

void MmWidget::onBarcodeScanned(const QString& barcode)
{
    ui->lastBarcodeLabel->setText(tr("Last scan: %1").arg(barcode));

    // Look up material by number
    auto token = DatabaseManager::instance().acquire();
    if (!token) return;
    QSqlQuery q(token->connection());
    q.prepare("SELECT id, description FROM materials WHERE material_number=:mn AND is_active=true");
    q.bindValue(":mn", barcode.trimmed());
    const QString matDesc = (q.exec() && q.next())
        ? q.value(1).toString() : tr("Unknown material");

    // Log to scan table
    auto* tbl = ui->scanLogTable;
    int row = 0;
    tbl->insertRow(row);
    tbl->setItem(row, 0, new QTableWidgetItem(QDateTime::currentDateTime().toString("hh:mm:ss")));
    tbl->setItem(row, 1, new QTableWidgetItem(barcode));
    tbl->setItem(row, 2, new QTableWidgetItem(matDesc));
    tbl->setItem(row, 3, new QTableWidgetItem(tr("Scanned")));
    tbl->scrollToTop();
}

void MmWidget::onRunReorderAnalysis()
{
    ui->runReorderBtn->setEnabled(false);
    ui->runReorderBtn->setText(tr("Analysing..."));

    int lookback  = ui->lookbackSpin->value();
    int leadTime  = ui->leadTimeSpin->value();

    // Run analysis on background thread via QtConcurrent
    auto* watcher = new QFutureWatcher<QVector<PredictiveReorder::ReorderSuggestion>>(this);
    connect(watcher, &QFutureWatcher<QVector<PredictiveReorder::ReorderSuggestion>>::finished,
            this, [this, watcher]() {
        auto suggestions = watcher->result();
        watcher->deleteLater();

        auto* tbl = ui->reorderTable;
        tbl->setRowCount(0);
        tbl->horizontalHeader()->setStretchLastSection(true);

        int row = 0;
        for (const auto& s : suggestions) {
            tbl->insertRow(row);
            tbl->setItem(row, 0, new QTableWidgetItem(s.urgent ? "🔴" : "🟡"));
            tbl->setItem(row, 1, new QTableWidgetItem(
                s.materialNumber + " - " + s.materialDescription));
            tbl->setItem(row, 2, new QTableWidgetItem(s.warehouseCode));
            tbl->setItem(row, 3, new QTableWidgetItem(QString("%1").arg(s.currentStock,0,'f',1)));
            tbl->setItem(row, 4, new QTableWidgetItem(QString("%1").arg(s.avgDailyConsumption,0,'f',2)));
            tbl->setItem(row, 5, new QTableWidgetItem(QString("%1").arg(s.projectedDemand,0,'f',1)));
            tbl->setItem(row, 6, new QTableWidgetItem(QString("%1").arg(s.safetyStock,0,'f',1)));
            auto* qtyItem = new QTableWidgetItem(QString("%1").arg(s.suggestedOrderQty,0,'f',1));
            qtyItem->setForeground(s.urgent ? QColor("#F85149") : QColor("#D29922"));
            tbl->setItem(row, 7, qtyItem);
            ++row;
        }

        ui->runReorderBtn->setEnabled(true);
        ui->runReorderBtn->setText(tr("Run Analysis"));
    });

    watcher->setFuture(QtConcurrent::run([lookback, leadTime]() {
        return PredictiveReorder::instance().analyse(lookback, leadTime);
    }));
}
