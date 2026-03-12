#include "CrmWidget.h"
#include "models/CustomerModel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QDialog>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QFont>

CrmWidget::CrmWidget(QWidget *parent) : QWidget(parent)
{
    m_model = new CustomerModel(this);
    setupUi();
    refresh();
}

void CrmWidget::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    // ---- Header ----
    auto *headerRow = new QHBoxLayout;
    auto *title = new QLabel(tr("Customers"), this);
    QFont hf;
    hf.setPointSize(20);
    hf.setBold(true);
    title->setFont(hf);

    auto *addBtn = new QPushButton(tr("+ Add Customer"), this);
    addBtn->setObjectName("primaryButton");
    addBtn->setCursor(Qt::PointingHandCursor);

    headerRow->addWidget(title);
    headerRow->addStretch();
    headerRow->addWidget(addBtn);
    root->addLayout(headerRow);

    // ---- Search + actions ----
    auto *actRow = new QHBoxLayout;

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search by name, number or email…"));
    m_searchEdit->setObjectName("searchField");
    m_searchEdit->setMinimumWidth(280);

    m_editBtn   = new QPushButton(tr("Edit"), this);
    m_deleteBtn = new QPushButton(tr("Deactivate"), this);
    m_editBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);

    actRow->addWidget(m_searchEdit);
    actRow->addStretch();
    actRow->addWidget(m_editBtn);
    actRow->addWidget(m_deleteBtn);
    root->addLayout(actRow);

    // ---- Table ----
    m_table = new QTableView(this);
    m_table->setModel(m_model);
    m_table->setObjectName("dataTable");
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);

    // Column widths
    m_table->setColumnWidth(0, 100);
    m_table->setColumnWidth(1, 220);
    m_table->setColumnWidth(2, 120);
    m_table->setColumnWidth(3, 200);
    m_table->setColumnWidth(4, 130);
    m_table->setColumnWidth(5, 120);

    root->addWidget(m_table);

    // ---- Connections ----
    connect(addBtn,     &QPushButton::clicked, this, &CrmWidget::onAdd);
    connect(m_editBtn,  &QPushButton::clicked, this, &CrmWidget::onEdit);
    connect(m_deleteBtn,&QPushButton::clicked, this, &CrmWidget::onDelete);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &CrmWidget::onSearch);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &CrmWidget::onSelectionChanged);
    connect(m_table, &QTableView::doubleClicked, this, &CrmWidget::onEdit);
}

void CrmWidget::refresh()
{
    m_model->load(m_searchEdit ? m_searchEdit->text() : QString());
}

void CrmWidget::onSearch(const QString &text)
{
    m_model->load(text.trimmed());
}

void CrmWidget::onSelectionChanged()
{
    bool sel = m_table->selectionModel()->hasSelection();
    m_editBtn->setEnabled(sel);
    m_deleteBtn->setEnabled(sel);
}

void CrmWidget::onAdd()
{
    showCustomerDialog(-1);
}

void CrmWidget::onEdit()
{
    QModelIndexList sel = m_table->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;
    int id = m_model->data(sel.first(), Qt::UserRole).toInt();
    showCustomerDialog(id);
}

void CrmWidget::onDelete()
{
    QModelIndexList sel = m_table->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;

    int row = sel.first().row();
    const CustomerRecord &rec = m_model->record(row);

    auto btn = QMessageBox::question(this,
        tr("Deactivate Customer"),
        tr("Deactivate customer \"%1\"? The record will be preserved for historical data.")
            .arg(rec.name),
        QMessageBox::Yes | QMessageBox::Cancel);

    if (btn == QMessageBox::Yes)
        m_model->deactivateCustomer(rec.id);
}

void CrmWidget::showCustomerDialog(int customerId)
{
    // Find existing record if editing
    CustomerRecord rec;
    bool isNew = (customerId == -1);

    if (!isNew) {
        // Find record in model
        for (int i = 0; i < m_model->rowCount(); ++i) {
            if (m_model->record(i).id == customerId) {
                rec = m_model->record(i);
                break;
            }
        }
    }

    // Build dialog
    QDialog dlg(this);
    dlg.setWindowTitle(isNew ? tr("Add Customer") : tr("Edit Customer"));
    dlg.setMinimumWidth(480);

    auto *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto *title = new QLabel(isNew ? tr("New Customer") : tr("Edit Customer"), &dlg);
    QFont tf;
    tf.setPointSize(15);
    tf.setBold(true);
    title->setFont(tf);
    layout->addWidget(title);

    auto *form = new QFormLayout;
    form->setSpacing(10);

    auto *nameEdit    = new QLineEdit(rec.name, &dlg);
    auto *taxEdit     = new QLineEdit(rec.taxId, &dlg);
    auto *emailEdit   = new QLineEdit(rec.email, &dlg);
    auto *phoneEdit   = new QLineEdit(rec.phone, &dlg);
    auto *addrEdit    = new QLineEdit(rec.billingAddress, &dlg);
    auto *creditSpin  = new QDoubleSpinBox(&dlg);
    auto *currCombo   = new QComboBox(&dlg);

    creditSpin->setRange(0, 9999999.99);
    creditSpin->setDecimals(2);
    creditSpin->setValue(rec.creditLimit);
    creditSpin->setPrefix("  ");

    currCombo->addItems({"EUR", "USD", "GBP", "CHF", "AED", "SAR"});
    currCombo->setCurrentText(rec.currency.isEmpty() ? "EUR" : rec.currency);

    nameEdit->setPlaceholderText(tr("Company or person name"));
    taxEdit->setPlaceholderText(tr("e.g. DE123456789"));
    emailEdit->setPlaceholderText("contact@company.com");
    phoneEdit->setPlaceholderText("+49 30 1234567");
    addrEdit->setPlaceholderText(tr("Street, City, Country"));

    form->addRow(tr("Name *:"),         nameEdit);
    form->addRow(tr("Tax ID:"),         taxEdit);
    form->addRow(tr("Email:"),          emailEdit);
    form->addRow(tr("Phone:"),          phoneEdit);
    form->addRow(tr("Address:"),        addrEdit);
    form->addRow(tr("Credit Limit:"),   creditSpin);
    form->addRow(tr("Currency:"),       currCombo);

    layout->addLayout(form);

    auto *errorLbl = new QLabel(&dlg);
    errorLbl->setStyleSheet("color: #F85149;");
    errorLbl->hide();
    layout->addWidget(errorLbl);

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    btns->button(QDialogButtonBox::Ok)->setText(isNew ? tr("Add") : tr("Save"));
    btns->button(QDialogButtonBox::Ok)->setObjectName("primaryButton");
    layout->addWidget(btns);

    connect(btns, &QDialogButtonBox::accepted, [&]() {
        if (nameEdit->text().trimmed().isEmpty()) {
            errorLbl->setText(tr("Customer name is required."));
            errorLbl->show();
            return;
        }
        CustomerRecord nr;
        nr.id             = customerId;
        nr.name           = nameEdit->text().trimmed();
        nr.taxId          = taxEdit->text().trimmed();
        nr.email          = emailEdit->text().trimmed();
        nr.phone          = phoneEdit->text().trimmed();
        nr.billingAddress = addrEdit->text().trimmed();
        nr.creditLimit    = creditSpin->value();
        nr.currency       = currCombo->currentText();

        bool ok = isNew ? m_model->addCustomer(nr) : m_model->updateCustomer(nr);
        if (ok)
            dlg.accept();
        else {
            errorLbl->setText(tr("Failed to save. Please check the data and try again."));
            errorLbl->show();
        }
    });
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    dlg.exec();
}
