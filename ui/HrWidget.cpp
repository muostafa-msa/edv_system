#include "HrWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QFont>
#include <QSqlQuery>
#include <QDate>

#include "core/DatabaseManager.h"

HrWidget::HrWidget(QWidget *parent) : QWidget(parent)
{
    setupUi();
    refresh();
}

void HrWidget::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    // Header
    auto *hdrRow = new QHBoxLayout;
    auto *title = new QLabel(tr("Human Resources"), this);
    QFont hf;
    hf.setPointSize(20);
    hf.setBold(true);
    title->setFont(hf);

    auto *addBtn = new QPushButton(tr("+ Add Employee"), this);
    addBtn->setObjectName("primaryButton");
    addBtn->setCursor(Qt::PointingHandCursor);

    hdrRow->addWidget(title);
    hdrRow->addStretch();
    hdrRow->addWidget(addBtn);
    root->addLayout(hdrRow);

    // Search
    auto *actRow = new QHBoxLayout;
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search by name, number or department…"));
    m_searchEdit->setObjectName("searchField");
    m_searchEdit->setMinimumWidth(280);
    actRow->addWidget(m_searchEdit);
    actRow->addStretch();
    root->addLayout(actRow);

    // Table
    m_table = new QTableWidget(this);
    m_table->setObjectName("dataTable");
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        tr("Emp. No."), tr("Name"), tr("Job Title"),
        tr("Department"), tr("Hire Date"), tr("Salary"), tr("Status")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->setSortingEnabled(true);

    m_table->setColumnWidth(0, 100);
    m_table->setColumnWidth(1, 180);
    m_table->setColumnWidth(2, 160);
    m_table->setColumnWidth(3, 140);
    m_table->setColumnWidth(4, 100);
    m_table->setColumnWidth(5, 110);

    root->addWidget(m_table);

    connect(addBtn, &QPushButton::clicked, this, &HrWidget::onAddEmployee);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &HrWidget::onSearch);
}

void HrWidget::refresh()
{
    loadData(m_searchEdit ? m_searchEdit->text() : QString());
}

void HrWidget::onSearch(const QString &text)
{
    loadData(text.trimmed());
}

void HrWidget::loadData(const QString &filter)
{
    m_table->setSortingEnabled(false);
    m_table->setRowCount(0);

    QString sql =
        "SELECT e.employee_number, e.first_name || ' ' || e.last_name AS full_name, "
        "       e.job_title, d.name AS dept, e.hire_date, e.salary, e.is_active "
        "FROM employees e "
        "LEFT JOIN departments d ON d.id = e.department_id "
        "WHERE e.is_active = true ";

    if (!filter.isEmpty())
        sql += " AND (LOWER(e.first_name || ' ' || e.last_name) LIKE :f OR e.employee_number LIKE :f)";

    sql += " ORDER BY e.last_name, e.first_name";

    QSqlQuery q(DatabaseManager::instance().database());
    q.prepare(sql);
    if (!filter.isEmpty())
        q.bindValue(":f", "%" + filter.toLower() + "%");

    if (!q.exec()) return;

    while (q.next()) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        auto makeItem = [](const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            return item;
        };

        m_table->setItem(row, 0, makeItem(q.value(0).toString()));
        m_table->setItem(row, 1, makeItem(q.value(1).toString()));
        m_table->setItem(row, 2, makeItem(q.value(2).toString()));
        m_table->setItem(row, 3, makeItem(q.value(3).toString()));
        m_table->setItem(row, 4, makeItem(q.value(4).toDate().toString("dd.MM.yyyy")));

        auto *salItem = makeItem(QString::number(q.value(5).toDouble(), 'f', 2));
        salItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 5, salItem);

        bool active = q.value(6).toBool();
        auto *statusItem = makeItem(active ? tr("Active") : tr("Inactive"));
        statusItem->setForeground(active ? QColor("#2EA043") : QColor("#F85149"));
        QFont sf;
        sf.setBold(true);
        statusItem->setFont(sf);
        m_table->setItem(row, 6, statusItem);
    }

    m_table->setSortingEnabled(true);
}

void HrWidget::onAddEmployee()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Add Employee"));
    dlg.setMinimumWidth(460);

    auto *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto *title = new QLabel(tr("New Employee"), &dlg);
    QFont tf;
    tf.setPointSize(15);
    tf.setBold(true);
    title->setFont(tf);
    layout->addWidget(title);

    auto *form = new QFormLayout;
    form->setSpacing(10);

    auto *firstEdit  = new QLineEdit(&dlg);
    auto *lastEdit   = new QLineEdit(&dlg);
    auto *emailEdit  = new QLineEdit(&dlg);
    auto *phoneEdit  = new QLineEdit(&dlg);
    auto *titleEdit  = new QLineEdit(&dlg);
    auto *hireDate   = new QDateEdit(QDate::currentDate(), &dlg);
    auto *salSpin    = new QDoubleSpinBox(&dlg);

    hireDate->setCalendarPopup(true);
    hireDate->setDisplayFormat("dd.MM.yyyy");
    salSpin->setRange(0, 9999999.99);
    salSpin->setDecimals(2);

    firstEdit->setPlaceholderText(tr("First name"));
    lastEdit->setPlaceholderText(tr("Last name"));
    emailEdit->setPlaceholderText("employee@company.com");
    titleEdit->setPlaceholderText(tr("e.g. Software Engineer"));

    form->addRow(tr("First Name *:"), firstEdit);
    form->addRow(tr("Last Name *:"),  lastEdit);
    form->addRow(tr("Email:"),        emailEdit);
    form->addRow(tr("Phone:"),        phoneEdit);
    form->addRow(tr("Job Title:"),    titleEdit);
    form->addRow(tr("Hire Date:"),    hireDate);
    form->addRow(tr("Monthly Salary:"), salSpin);

    layout->addLayout(form);

    auto *errorLbl = new QLabel(&dlg);
    errorLbl->setStyleSheet("color: #F85149;");
    errorLbl->hide();
    layout->addWidget(errorLbl);

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    btns->button(QDialogButtonBox::Ok)->setText(tr("Add Employee"));
    btns->button(QDialogButtonBox::Ok)->setObjectName("primaryButton");
    layout->addWidget(btns);

    connect(btns, &QDialogButtonBox::accepted, [&]() {
        if (firstEdit->text().trimmed().isEmpty() || lastEdit->text().trimmed().isEmpty()) {
            errorLbl->setText(tr("First and last name are required."));
            errorLbl->show();
            return;
        }

        // Generate employee number
        QSqlQuery numQ(DatabaseManager::instance().database());
        numQ.exec("SELECT NEXTVAL('seq_employee')");
        QString empNum = "EMP001";
        if (numQ.next())
            empNum = QString("EMP%1").arg(numQ.value(0).toInt(), 4, 10, QChar('0'));

        QSqlQuery q(DatabaseManager::instance().database());
        q.prepare(
            "INSERT INTO employees (employee_number, first_name, last_name, "
            "email, phone, job_title, hire_date, salary) "
            "VALUES (:num, :first, :last, :email, :phone, :title, :hire, :sal)"
        );
        q.bindValue(":num",   empNum);
        q.bindValue(":first", firstEdit->text().trimmed());
        q.bindValue(":last",  lastEdit->text().trimmed());
        q.bindValue(":email", emailEdit->text().trimmed());
        q.bindValue(":phone", phoneEdit->text().trimmed());
        q.bindValue(":title", titleEdit->text().trimmed());
        q.bindValue(":hire",  hireDate->date().toString("yyyy-MM-dd"));
        q.bindValue(":sal",   salSpin->value());

        if (q.exec()) {
            dlg.accept();
            refresh();
        } else {
            errorLbl->setText(tr("Failed to save employee."));
            errorLbl->show();
        }
    });
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    dlg.exec();
}
