#include "InventoryWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QColor>
#include <QFont>
#include <QDialog>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QSqlQuery>
#include <QMessageBox>

#include "core/DatabaseManager.h"

InventoryWidget::InventoryWidget(QWidget *parent) : QWidget(parent)
{
    setupUi();
    refresh();
}

void InventoryWidget::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    // ---- Header ----
    auto *hdrRow = new QHBoxLayout;
    auto *title = new QLabel(tr("Inventory & Materials"), this);
    QFont hf;
    hf.setPointSize(20);
    hf.setBold(true);
    title->setFont(hf);

    auto *addBtn = new QPushButton(tr("+ Add Material"), this);
    addBtn->setObjectName("primaryButton");
    addBtn->setCursor(Qt::PointingHandCursor);

    hdrRow->addWidget(title);
    hdrRow->addStretch();
    hdrRow->addWidget(addBtn);
    root->addLayout(hdrRow);

    // ---- Search ----
    auto *actRow = new QHBoxLayout;
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search by SKU, name or barcode…"));
    m_searchEdit->setObjectName("searchField");
    m_searchEdit->setMinimumWidth(280);
    actRow->addWidget(m_searchEdit);
    actRow->addStretch();
    root->addLayout(actRow);

    // ---- Table ----
    m_table = new QTableWidget(this);
    m_table->setObjectName("dataTable");
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({
        tr("SKU"), tr("Name"), tr("Category"), tr("Unit"),
        tr("In Stock"), tr("Min Stock"), tr("Sell Price"), tr("Status")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->setSortingEnabled(true);

    m_table->setColumnWidth(0, 100);
    m_table->setColumnWidth(1, 220);
    m_table->setColumnWidth(2, 120);
    m_table->setColumnWidth(3, 60);
    m_table->setColumnWidth(4, 90);
    m_table->setColumnWidth(5, 90);
    m_table->setColumnWidth(6, 100);

    root->addWidget(m_table);

    connect(addBtn, &QPushButton::clicked, this, &InventoryWidget::onAddMaterial);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &InventoryWidget::onSearch);
}

void InventoryWidget::refresh()
{
    loadData(m_searchEdit ? m_searchEdit->text() : QString());
}

void InventoryWidget::onSearch(const QString &text)
{
    loadData(text.trimmed());
}

void InventoryWidget::loadData(const QString &filter)
{
    m_table->setSortingEnabled(false);
    m_table->setRowCount(0);

    QString sql =
        "SELECT m.sku, m.name, mc.name AS category, m.base_unit, "
        "       COALESCE(SUM(il.quantity_change),0) AS stock, "
        "       m.min_stock, m.selling_price, m.is_active "
        "FROM materials m "
        "LEFT JOIN material_categories mc ON mc.id = m.category_id "
        "LEFT JOIN inventory_ledger il ON il.material_id = m.id "
        "WHERE m.is_active = true ";

    if (!filter.isEmpty())
        sql += " AND (LOWER(m.name) LIKE :f OR LOWER(m.sku) LIKE :f)";

    sql += " GROUP BY m.id, m.sku, m.name, mc.name, m.base_unit, m.min_stock, m.selling_price, m.is_active"
           " ORDER BY m.name";

    QSqlQuery q(DatabaseManager::instance().database());
    q.prepare(sql);
    if (!filter.isEmpty())
        q.bindValue(":f", "%" + filter.toLower() + "%");

    if (!q.exec()) return;

    while (q.next()) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        double stock    = q.value(4).toDouble();
        double minStock = q.value(5).toDouble();
        bool   lowStock = (minStock > 0 && stock < minStock);

        auto makeItem = [](const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            return item;
        };

        m_table->setItem(row, 0, makeItem(q.value(0).toString()));
        m_table->setItem(row, 1, makeItem(q.value(1).toString()));
        m_table->setItem(row, 2, makeItem(q.value(2).toString()));
        m_table->setItem(row, 3, makeItem(q.value(3).toString()));

        auto *stockItem = makeItem(QString::number(stock, 'f', 2));
        stockItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (lowStock)
            stockItem->setForeground(QColor("#F85149"));
        m_table->setItem(row, 4, stockItem);

        auto *minItem = makeItem(QString::number(minStock, 'f', 2));
        minItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 5, minItem);

        auto *priceItem = makeItem(QString::number(q.value(6).toDouble(), 'f', 2));
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 6, priceItem);

        auto *statusItem = makeItem(lowStock ? tr("Low Stock") : tr("OK"));
        statusItem->setForeground(lowStock ? QColor("#D29922") : QColor("#2EA043"));
        QFont sf;
        sf.setBold(true);
        statusItem->setFont(sf);
        m_table->setItem(row, 7, statusItem);
    }

    m_table->setSortingEnabled(true);
}

void InventoryWidget::onAddMaterial()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Add Material"));
    dlg.setMinimumWidth(440);

    auto *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(24, 24, 24, 24);

    auto *title = new QLabel(tr("New Material"), &dlg);
    QFont tf;
    tf.setPointSize(15);
    tf.setBold(true);
    title->setFont(tf);
    layout->addWidget(title);

    auto *form = new QFormLayout;
    form->setSpacing(10);

    auto *skuEdit   = new QLineEdit(&dlg);
    auto *nameEdit  = new QLineEdit(&dlg);
    auto *unitEdit  = new QLineEdit("pcs", &dlg);
    auto *costSpin  = new QDoubleSpinBox(&dlg);
    auto *priceSpin = new QDoubleSpinBox(&dlg);
    auto *minSpin   = new QDoubleSpinBox(&dlg);

    costSpin->setRange(0, 999999.99);
    costSpin->setDecimals(2);
    priceSpin->setRange(0, 999999.99);
    priceSpin->setDecimals(2);
    minSpin->setRange(0, 999999);
    minSpin->setDecimals(0);

    skuEdit->setPlaceholderText("MAT-001");
    nameEdit->setPlaceholderText(tr("Material description"));

    form->addRow(tr("SKU *:"),        skuEdit);
    form->addRow(tr("Name *:"),       nameEdit);
    form->addRow(tr("Base Unit:"),    unitEdit);
    form->addRow(tr("Cost Price:"),   costSpin);
    form->addRow(tr("Sell Price:"),   priceSpin);
    form->addRow(tr("Min Stock:"),    minSpin);

    layout->addLayout(form);

    auto *errorLbl = new QLabel(&dlg);
    errorLbl->setStyleSheet("color: #F85149;");
    errorLbl->hide();
    layout->addWidget(errorLbl);

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    btns->button(QDialogButtonBox::Ok)->setText(tr("Add"));
    btns->button(QDialogButtonBox::Ok)->setObjectName("primaryButton");
    layout->addWidget(btns);

    connect(btns, &QDialogButtonBox::accepted, [&]() {
        if (skuEdit->text().trimmed().isEmpty() || nameEdit->text().trimmed().isEmpty()) {
            errorLbl->setText(tr("SKU and Name are required."));
            errorLbl->show();
            return;
        }
        QSqlQuery q(DatabaseManager::instance().database());
        q.prepare(
            "INSERT INTO materials (sku, name, base_unit, cost_price, selling_price, min_stock) "
            "VALUES (:sku, :name, :unit, :cost, :price, :min)"
        );
        q.bindValue(":sku",   skuEdit->text().trimmed());
        q.bindValue(":name",  nameEdit->text().trimmed());
        q.bindValue(":unit",  unitEdit->text().trimmed());
        q.bindValue(":cost",  costSpin->value());
        q.bindValue(":price", priceSpin->value());
        q.bindValue(":min",   minSpin->value());

        if (q.exec()) {
            dlg.accept();
            refresh();
        } else {
            errorLbl->setText(tr("Failed to save. SKU may already exist."));
            errorLbl->show();
        }
    });
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    dlg.exec();
}
