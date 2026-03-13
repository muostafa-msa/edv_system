#include "SdWidget.hpp"
#include "ui_SdWidget.h"
#include "core/database/DatabaseManager.hpp"
#include "core/journal/PostManager.hpp"
#include "modules/sd/PricingEngine.hpp"

#include <QSqlQuery>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QDateEdit>
#include <QComboBox>
#include <QLabel>

// ── SalesOrderModel ───────────────────────────────────────────────────────────

SalesOrderModel::SalesOrderModel(QObject* parent) : QAbstractTableModel(parent) {}

QVariant SalesOrderModel::data(const QModelIndex& idx, int role) const
{
    if (!idx.isValid() || idx.row() >= m_data.size()) return {};
    const auto& r = m_data[idx.row()];

    if (role == Qt::DisplayRole) {
        switch (idx.column()) {
            case 0: return r.orderNumber;
            case 1: return r.customerName;
            case 2: return r.orderDate.toString("yyyy-MM-dd");
            case 3: return r.status.toUpper();
            case 4: return QString("%1 %2").arg(r.totalGross, 0,'f',2).arg(r.currency);
            case 5: return r.id;
        }
    }
    if (role == Qt::ForegroundRole) {
        if (r.status == "invoiced") return QColor("#2EA043");
        if (r.status == "cancelled") return QColor("#8B949E");
    }
    if (role == Qt::TextAlignmentRole && idx.column() == 4)
        return int(Qt::AlignRight | Qt::AlignVCenter);
    return {};
}

QVariant SalesOrderModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static const QStringList h = {"Order No.","Customer","Date","Status","Total","ID"};
        return h.value(section);
    }
    return {};
}

void SalesOrderModel::load(const QString& statusFilter, const QString& search)
{
    beginResetModel();
    m_data.clear();

    auto token = DatabaseManager::instance().acquire();
    if (!token) { endResetModel(); return; }

    QString sql =
        "SELECT so.id, so.order_number, c.company_name, so.order_date, "
        "       so.status, so.total_gross, so.currency "
        "FROM sales_orders so "
        "JOIN customers c ON c.id=so.customer_id ";

    QStringList where;
    if (!statusFilter.isEmpty() && statusFilter != "All Statuses")
        where << "so.status=:status";
    if (!search.isEmpty())
        where << "(so.order_number ILIKE :s OR c.company_name ILIKE :s)";
    if (!where.isEmpty()) sql += "WHERE " + where.join(" AND ") + " ";
    sql += "ORDER BY so.order_date DESC, so.id DESC LIMIT 500";

    QSqlQuery q(token->connection());
    q.prepare(sql);
    if (!statusFilter.isEmpty() && statusFilter != "All Statuses")
        q.bindValue(":status", statusFilter);
    if (!search.isEmpty())
        q.bindValue(":s", "%" + search + "%");

    if (q.exec()) {
        while (q.next())
            m_data.append({q.value(0).toInt(), q.value(1).toString(),
                           q.value(2).toString(), q.value(3).toDate(),
                           q.value(4).toString(), q.value(5).toDouble(),
                           q.value(6).toString()});
    }
    endResetModel();
}

int SalesOrderModel::orderIdAt(int row) const
{
    return (row >= 0 && row < m_data.size()) ? m_data[row].id : -1;
}
QString SalesOrderModel::orderNumberAt(int row) const
{
    return (row >= 0 && row < m_data.size()) ? m_data[row].orderNumber : QString{};
}
QString SalesOrderModel::orderStatusAt(int row) const
{
    return (row >= 0 && row < m_data.size()) ? m_data[row].status : QString{};
}

// ── SdWidget ──────────────────────────────────────────────────────────────────

SdWidget::SdWidget(const Session& session, QWidget *parent)
    : QWidget(parent), ui(new Ui::SdWidget)
    , m_model(new SalesOrderModel(this))
    , m_session(session)
{
    ui->setupUi(this);
    ui->ordersTable->setModel(m_model);
    ui->ordersTable->horizontalHeader()->setStretchLastSection(false);
    ui->ordersTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->ordersTable->verticalHeader()->hide();
    ui->ordersTable->setColumnHidden(5, true); // hide internal ID col

    connect(ui->refreshBtn,   &QPushButton::clicked,  this, &SdWidget::onRefresh);
    connect(ui->newOrderBtn,  &QPushButton::clicked,  this, &SdWidget::onNewOrder);
    connect(ui->confirmBtn,   &QPushButton::clicked,  this, &SdWidget::onConfirmOrder);
    connect(ui->invoiceBtn,   &QPushButton::clicked,  this, &SdWidget::onPostInvoice);
    connect(ui->ordersTable->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this, [this](const QModelIndex& cur, const QModelIndex&){
                onOrderSelected(cur);
            });
    connect(ui->searchEdit,   &QLineEdit::textChanged, this, &SdWidget::onFilterChanged);
    connect(ui->statusFilter, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &SdWidget::onFilterChanged);

    if (!session.hasRole(edv::core::security::Role::Manager)) {
        ui->newOrderBtn->hide();
        ui->confirmBtn->hide();
        ui->invoiceBtn->hide();
    }

    loadOrders();
}

SdWidget::~SdWidget() { delete ui; }

void SdWidget::onRefresh()      { loadOrders(); }
void SdWidget::onFilterChanged(){ loadOrders(); }

void SdWidget::loadOrders()
{
    m_model->load(ui->statusFilter->currentText(), ui->searchEdit->text().trimmed());
}

void SdWidget::onOrderSelected(const QModelIndex& idx)
{
    if (!idx.isValid()) return;
    loadLineItems(m_model->orderIdAt(idx.row()));
}

void SdWidget::loadLineItems(int orderId)
{
    auto* tbl = ui->lineItemsTable;
    tbl->setRowCount(0);
    tbl->horizontalHeader()->setStretchLastSection(true);

    auto token = DatabaseManager::instance().acquire();
    if (!token) return;

    QSqlQuery q(token->connection());
    q.prepare(
        "SELECT soi.line_number, m.description, soi.quantity, "
        "       soi.unit_of_measure, soi.unit_price, soi.discount_pct, soi.net_amount "
        "FROM sales_order_items soi "
        "JOIN materials m ON m.id=soi.material_id "
        "WHERE soi.sales_order_id=:id ORDER BY soi.line_number"
    );
    q.bindValue(":id", orderId);
    if (!q.exec()) return;

    int row = 0;
    while (q.next()) {
        tbl->insertRow(row);
        for (int c = 0; c < 7; ++c) {
            QString val;
            switch(c) {
                case 0: val = q.value(0).toString(); break;
                case 1: val = q.value(1).toString(); break;
                case 2: val = q.value(2).toString(); break;
                case 3: val = q.value(3).toString(); break;
                case 4: val = QString("%1").arg(q.value(4).toDouble(),0,'f',2); break;
                case 5: val = QString("%1%").arg(q.value(5).toDouble(),0,'f',1); break;
                case 6: val = QString("%1 EUR").arg(q.value(6).toDouble(),0,'f',2); break;
            }
            auto* item = new QTableWidgetItem(val);
            if (c >= 2) item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            tbl->setItem(row, c, item);
        }
        ++row;
    }
}

void SdWidget::onNewOrder()
{
    // Dialog: pick customer, add one line item
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle(tr("Create Sales Order"));
    dlg->setMinimumWidth(520);
    auto* form = new QFormLayout(dlg);
    form->setSpacing(12); form->setContentsMargins(24,24,24,24);

    // Customer picker
    auto* custCombo = new QComboBox(dlg);
    {
        auto token = DatabaseManager::instance().acquire();
        if (token) {
            QSqlQuery q(token->connection());
            q.exec("SELECT id, company_name FROM customers WHERE is_active=true ORDER BY company_name");
            while (q.next())
                custCombo->addItem(q.value(1).toString(), q.value(0).toInt());
        }
    }

    // Material picker
    auto* matCombo = new QComboBox(dlg);
    {
        auto token = DatabaseManager::instance().acquire();
        if (token) {
            QSqlQuery q(token->connection());
            q.exec("SELECT id, material_number, description, base_price FROM materials WHERE is_active=true ORDER BY description");
            while (q.next())
                matCombo->addItem(
                    QString("%1 - %2").arg(q.value(1).toString(), q.value(2).toString()),
                    QVariantList{q.value(0).toInt(), q.value(3).toDouble()});
        }
    }

    auto* qtyEdit   = new QDoubleSpinBox(dlg);
    qtyEdit->setRange(0.001, 999999); qtyEdit->setValue(1.0);
    auto* priceLbl  = new QLabel("—", dlg);

    // Update price preview using PricingEngine
    auto updatePrice = [&]() {
        if (matCombo->count() == 0 || custCombo->count() == 0) return;
        int matId  = matCombo->currentData().toList().value(0).toInt();
        int custId = custCombo->currentData().toInt();
        double price = PricingEngine::instance().calculatePrice(matId, custId, qtyEdit->value());
        priceLbl->setText(QString("%1 EUR").arg(price, 0,'f',2));
    };
    connect(matCombo,  qOverload<int>(&QComboBox::currentIndexChanged), dlg, updatePrice);
    connect(custCombo, qOverload<int>(&QComboBox::currentIndexChanged), dlg, updatePrice);
    connect(qtyEdit, &QDoubleSpinBox::valueChanged, dlg, updatePrice);
    updatePrice();

    form->addRow(tr("Customer:"),     custCombo);
    form->addRow(tr("Material:"),     matCombo);
    form->addRow(tr("Quantity:"),     qtyEdit);
    form->addRow(tr("Net Price:"),    priceLbl);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dlg);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

    if (dlg->exec() != QDialog::Accepted) return;

    int custId  = custCombo->currentData().toInt();
    auto matData= matCombo->currentData().toList();
    int matId   = matData.value(0).toInt();
    double qty  = qtyEdit->value();
    double price= PricingEngine::instance().calculatePrice(matId, custId, qty);
    double net  = price * qty;

    auto token = DatabaseManager::instance().acquire();
    if (!token) return;

    QSqlQuery seqQ(token->connection());
    seqQ.exec("SELECT NEXTVAL('seq_sales_order')");
    if (!seqQ.next()) return;
    const QString orderNum = QString("SO-%1").arg(seqQ.value(0).toLongLong(), 6, 10, QChar('0'));

    QSqlDatabase db = token->connection();
    db.transaction();

    QSqlQuery ins(db);
    ins.prepare(
        "INSERT INTO sales_orders "
        "(order_number,customer_id,order_date,status,currency,net_amount,tax_amount,total_gross,created_by_user) "
        "VALUES (:on,:cid,CURRENT_DATE,'draft',:cur,:net,:tax,:gross,:uid) RETURNING id"
    );
    double tax = net * 0.19;
    ins.bindValue(":on",   orderNum);
    ins.bindValue(":cid",  custId);
    ins.bindValue(":cur",  "EUR");
    ins.bindValue(":net",  net);
    ins.bindValue(":tax",  tax);
    ins.bindValue(":gross",net + tax);
    ins.bindValue(":uid",  m_session.userId);

    if (!ins.exec() || !ins.next()) { db.rollback(); return; }
    int newId = ins.value(0).toInt();

    QSqlQuery lineIns(db);
    lineIns.prepare(
        "INSERT INTO sales_order_items "
        "(sales_order_id,line_number,material_id,quantity,unit_of_measure,unit_price,discount_pct,net_amount,tax_rate) "
        "VALUES (:sid,1,:mid,:qty,'EA',:price,0,:net,19)"
    );
    lineIns.bindValue(":sid",   newId);
    lineIns.bindValue(":mid",   matId);
    lineIns.bindValue(":qty",   qty);
    lineIns.bindValue(":price", price);
    lineIns.bindValue(":net",   net);
    if (!lineIns.exec()) { db.rollback(); return; }

    db.commit();
    QMessageBox::information(this, tr("Order Created"),
        tr("Sales order %1 created successfully.").arg(orderNum));
    loadOrders();
}

void SdWidget::onConfirmOrder()
{
    const auto idx = ui->ordersTable->currentIndex();
    if (!idx.isValid()) return;
    if (m_model->orderStatusAt(idx.row()) != "draft") {
        QMessageBox::information(this, tr("Status"), tr("Only draft orders can be confirmed."));
        return;
    }
    auto token = DatabaseManager::instance().acquire();
    if (!token) return;
    QSqlQuery q(token->connection());
    q.prepare("UPDATE sales_orders SET status='confirmed' WHERE id=:id");
    q.bindValue(":id", m_model->orderIdAt(idx.row()));
    q.exec();
    loadOrders();
}

void SdWidget::onPostInvoice()
{
    const auto idx = ui->ordersTable->currentIndex();
    if (!idx.isValid()) return;
    const QString status = m_model->orderStatusAt(idx.row());
    if (status != "confirmed" && status != "shipped") {
        QMessageBox::information(this, tr("Status"),
            tr("Order must be confirmed or shipped before invoicing."));
        return;
    }
    const int orderId      = m_model->orderIdAt(idx.row());
    const QString orderNum = m_model->orderNumberAt(idx.row());

    // Fetch net amount
    auto token = DatabaseManager::instance().acquire();
    if (!token) return;
    QSqlQuery q(token->connection());
    q.prepare("SELECT net_amount, customer_id FROM sales_orders WHERE id=:id");
    q.bindValue(":id", orderId);
    if (!q.exec() || !q.next()) return;
    double net = q.value(0).toDouble();
    int custId = q.value(1).toInt();

    postInvoiceJournal(orderId, orderNum, net);
    loadOrders();
}

void SdWidget::postInvoiceJournal(int orderId, const QString& orderNumber, double netAmount)
{
    double vat   = netAmount * 0.19;
    double gross = netAmount + vat;

    PostingRequest req;
    req.documentType   = DocumentType::DR;
    req.postingDate    = QDate::currentDate();
    req.documentDate   = QDate::currentDate();
    req.reference      = orderNumber;
    req.headerText     = "Customer Invoice: " + orderNumber;
    req.postedByUserId = m_session.userId;

    // Debit: AR 1100
    JournalLine ar; ar.glAccount = 1100; ar.amount = gross;
    ar.text = "Customer Invoice " + orderNumber;
    ar.salesOrderId = orderId;
    req.lines.append(ar);

    // Credit: Revenue 4000
    JournalLine rev; rev.glAccount = 4000; rev.amount = -netAmount;
    rev.text = "Revenue: " + orderNumber;
    rev.salesOrderId = orderId;
    req.lines.append(rev);

    // Credit: VAT 2100
    JournalLine vatLine; vatLine.glAccount = 2100; vatLine.amount = -vat;
    vatLine.text = "VAT 19%: " + orderNumber;
    req.lines.append(vatLine);

    auto result = PostManager::instance().post(req);
    if (!result.success) {
        QMessageBox::critical(this, tr("Invoice Failed"), result.errorMessage);
        return;
    }

    // Update SO status + link document
    auto token = DatabaseManager::instance().acquire();
    if (!token) return;
    QSqlQuery upd(token->connection());
    upd.prepare("UPDATE sales_orders SET status='invoiced', journal_doc_num=:dn WHERE id=:id");
    upd.bindValue(":dn", result.documentNumber);
    upd.bindValue(":id", orderId);
    upd.exec();

    QMessageBox::information(this, tr("Invoice Posted"),
        tr("Invoice posted as document %1").arg(result.documentNumber));
}
