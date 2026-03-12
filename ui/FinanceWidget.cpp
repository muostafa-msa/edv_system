#include "FinanceWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QDateEdit>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QFont>
#include <QDate>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include "core/DatabaseManager.h"

FinanceWidget::FinanceWidget(QWidget *parent) : QWidget(parent)
{
    setupUi();
    refresh();
}

void FinanceWidget::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    // ---- Header ----
    auto *hdrRow = new QHBoxLayout;
    auto *title = new QLabel(tr("Finance & Accounting"), this);
    QFont hf;
    hf.setPointSize(20);
    hf.setBold(true);
    title->setFont(hf);

    auto *addBtn = new QPushButton(tr("+ Manual Entry"), this);
    addBtn->setObjectName("primaryButton");
    addBtn->setCursor(Qt::PointingHandCursor);

    hdrRow->addWidget(title);
    hdrRow->addStretch();
    hdrRow->addWidget(addBtn);
    root->addLayout(hdrRow);

    // ---- Filter bar ----
    auto *filterRow = new QHBoxLayout;
    filterRow->setSpacing(12);

    auto *fromLbl = new QLabel(tr("From:"), this);
    m_fromDate = new QDateEdit(QDate::currentDate().addMonths(-1), this);
    m_fromDate->setCalendarPopup(true);
    m_fromDate->setDisplayFormat("dd.MM.yyyy");

    auto *toLbl = new QLabel(tr("To:"), this);
    m_toDate = new QDateEdit(QDate::currentDate(), this);
    m_toDate->setCalendarPopup(true);
    m_toDate->setDisplayFormat("dd.MM.yyyy");

    auto *filterBtn = new QPushButton(tr("Apply"), this);

    filterRow->addWidget(fromLbl);
    filterRow->addWidget(m_fromDate);
    filterRow->addWidget(toLbl);
    filterRow->addWidget(m_toDate);
    filterRow->addWidget(filterBtn);
    filterRow->addStretch();
    root->addLayout(filterRow);

    // ---- Journal table ----
    m_table = new QTableWidget(this);
    m_table->setObjectName("dataTable");
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        tr("Entry No."), tr("Date"), tr("Description"),
        tr("Account"), tr("Debit"), tr("Credit"), tr("Posted")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->setSortingEnabled(true);

    m_table->setColumnWidth(0, 110);
    m_table->setColumnWidth(1, 100);
    m_table->setColumnWidth(2, 220);
    m_table->setColumnWidth(3, 160);
    m_table->setColumnWidth(4, 100);
    m_table->setColumnWidth(5, 100);

    root->addWidget(m_table);

    connect(addBtn,    &QPushButton::clicked, this, &FinanceWidget::onAddJournalEntry);
    connect(filterBtn, &QPushButton::clicked, this, &FinanceWidget::onFilterChanged);
}

void FinanceWidget::refresh()
{
    loadJournal();
}

void FinanceWidget::onFilterChanged()
{
    loadJournal();
}

void FinanceWidget::loadJournal()
{
    m_table->setSortingEnabled(false);
    m_table->setRowCount(0);

    QSqlQuery q(DatabaseManager::instance().database());
    q.prepare(
        "SELECT je.entry_number, je.entry_date, je.description, "
        "       coa.account_number || ' - ' || coa.name AS account, "
        "       jl.debit_amount, jl.credit_amount, je.is_posted "
        "FROM journal_entries je "
        "JOIN journal_entry_lines jl ON jl.entry_id = je.id "
        "JOIN chart_of_accounts coa  ON coa.id = jl.account_id "
        "WHERE je.entry_date BETWEEN :from AND :to "
        "ORDER BY je.entry_date DESC, je.entry_number"
    );
    q.bindValue(":from", m_fromDate->date().toString("yyyy-MM-dd"));
    q.bindValue(":to",   m_toDate->date().toString("yyyy-MM-dd"));

    if (!q.exec()) {
        qWarning() << "FinanceWidget::loadJournal:" << q.lastError().text();
        return;
    }

    while (q.next()) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        auto makeItem = [](const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            return item;
        };

        m_table->setItem(row, 0, makeItem(q.value(0).toString()));
        m_table->setItem(row, 1, makeItem(q.value(1).toDate().toString("dd.MM.yyyy")));
        m_table->setItem(row, 2, makeItem(q.value(2).toString()));
        m_table->setItem(row, 3, makeItem(q.value(3).toString()));

        double debit  = q.value(4).toDouble();
        double credit = q.value(5).toDouble();

        auto *debitItem = makeItem(debit > 0 ? QString::number(debit, 'f', 2) : "");
        debitItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (debit > 0) debitItem->setForeground(QColor("#2EA043"));
        m_table->setItem(row, 4, debitItem);

        auto *creditItem = makeItem(credit > 0 ? QString::number(credit, 'f', 2) : "");
        creditItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (credit > 0) creditItem->setForeground(QColor("#F85149"));
        m_table->setItem(row, 5, creditItem);

        bool posted = q.value(6).toBool();
        auto *postedItem = makeItem(posted ? tr("Posted") : tr("Draft"));
        postedItem->setForeground(posted ? QColor("#2EA043") : QColor("#D29922"));
        QFont pf;
        pf.setBold(true);
        postedItem->setFont(pf);
        m_table->setItem(row, 6, postedItem);
    }

    m_table->setSortingEnabled(true);
}

void FinanceWidget::onAddJournalEntry()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("New Journal Entry"));
    dlg.setMinimumWidth(500);

    auto *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto *title = new QLabel(tr("Manual Journal Entry"), &dlg);
    QFont tf;
    tf.setPointSize(15);
    tf.setBold(true);
    title->setFont(tf);
    layout->addWidget(title);

    auto *form = new QFormLayout;
    form->setSpacing(10);

    auto *dateEdit = new QDateEdit(QDate::currentDate(), &dlg);
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("dd.MM.yyyy");

    auto *descEdit  = new QLineEdit(&dlg);
    auto *debitSpin = new QDoubleSpinBox(&dlg);
    auto *creditSpin= new QDoubleSpinBox(&dlg);

    descEdit->setPlaceholderText(tr("Entry description"));
    debitSpin->setRange(0, 9999999.99);
    debitSpin->setDecimals(2);
    creditSpin->setRange(0, 9999999.99);
    creditSpin->setDecimals(2);

    form->addRow(tr("Date:"),        dateEdit);
    form->addRow(tr("Description:"), descEdit);
    form->addRow(tr("Debit (EUR):"), debitSpin);
    form->addRow(tr("Credit (EUR):"),creditSpin);

    layout->addLayout(form);

    auto *noteLbl = new QLabel(tr("Note: For full double-entry booking, assign accounts in the detailed ledger view."), &dlg);
    noteLbl->setWordWrap(true);
    noteLbl->setObjectName("secondaryText");
    layout->addWidget(noteLbl);

    auto *errorLbl = new QLabel(&dlg);
    errorLbl->setStyleSheet("color: #F85149;");
    errorLbl->hide();
    layout->addWidget(errorLbl);

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    btns->button(QDialogButtonBox::Ok)->setText(tr("Save Entry"));
    btns->button(QDialogButtonBox::Ok)->setObjectName("primaryButton");
    layout->addWidget(btns);

    connect(btns, &QDialogButtonBox::accepted, [&]() {
        if (descEdit->text().trimmed().isEmpty()) {
            errorLbl->setText(tr("Description is required."));
            errorLbl->show();
            return;
        }

        QSqlQuery numQ(DatabaseManager::instance().database());
        numQ.exec("SELECT NEXTVAL('seq_journal_entry')");
        QString entryNum = "JE001";
        if (numQ.next())
            entryNum = QString("JE%1").arg(numQ.value(0).toInt(), 6, 10, QChar('0'));

        QSqlQuery q(DatabaseManager::instance().database());
        q.prepare(
            "INSERT INTO journal_entries (entry_number, entry_date, description, reference_type) "
            "VALUES (:num, :date, :desc, 'manual')"
        );
        q.bindValue(":num",  entryNum);
        q.bindValue(":date", dateEdit->date().toString("yyyy-MM-dd"));
        q.bindValue(":desc", descEdit->text().trimmed());

        if (q.exec()) {
            dlg.accept();
            refresh();
        } else {
            errorLbl->setText(tr("Failed to save entry."));
            errorLbl->show();
        }
    });
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    dlg.exec();
}
