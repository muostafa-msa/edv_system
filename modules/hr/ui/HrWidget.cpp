#include "HrWidget.hpp"
#include "ui_HrWidget.h"
#include "core/database/DatabaseManager.hpp"
#include "core/journal/PostManager.hpp"

#include <QSqlQuery>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QComboBox>

HrWidget::HrWidget(const Session& session, QWidget *parent)
    : QWidget(parent), ui(new Ui::HrWidget), m_session(session)
{
    ui->setupUi(this);
    ui->payPeriodEdit->setDate(QDate::currentDate());
    ui->payPeriodEdit->setDisplayFormat("yyyy-MM");

    connect(ui->empRefreshBtn,   &QPushButton::clicked, this, &HrWidget::loadEmployees);
    connect(ui->addEmpBtn,       &QPushButton::clicked, this, &HrWidget::onAddEmployee);
    connect(ui->runPayrollBtn,   &QPushButton::clicked, this, &HrWidget::onRunPayroll);
    connect(ui->postPayrollBtn,  &QPushButton::clicked, this, &HrWidget::onPostPayroll);
    connect(ui->empSearchEdit,   &QLineEdit::textChanged, this, &HrWidget::loadEmployees);

    if (!session.hasRole(edv::core::security::Role::Manager)) {
        ui->addEmpBtn->hide();
        ui->runPayrollBtn->hide();
        ui->postPayrollBtn->hide();
    }

    loadEmployees();
    loadPayrollRuns();
}

HrWidget::~HrWidget() { delete ui; }

void HrWidget::loadEmployees()
{
    auto* tbl = ui->employeeTable;
    tbl->setRowCount(0);
    tbl->horizontalHeader()->setStretchLastSection(true);
    tbl->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    auto token = DatabaseManager::instance().acquire();
    if (!token) return;

    const QString search = ui->empSearchEdit->text().trimmed();
    QString sql =
        "SELECT e.employee_number, e.first_name || ' ' || e.last_name, "
        "       e.department, e.position, cc.code, e.salary, e.hire_date "
        "FROM employees e "
        "LEFT JOIN cost_centers cc ON cc.id=e.cost_center_id "
        "WHERE e.is_active=true ";
    if (!search.isEmpty())
        sql += "AND (e.first_name ILIKE '%" + search + "%' OR e.last_name ILIKE '%" + search + "%' "
               "OR e.employee_number ILIKE '%" + search + "%') ";
    sql += "ORDER BY e.employee_number";

    QSqlQuery q(token->connection());
    q.exec(sql);

    int row = 0;
    while (q.next()) {
        tbl->insertRow(row);
        tbl->setItem(row, 0, new QTableWidgetItem(q.value(0).toString()));
        tbl->setItem(row, 1, new QTableWidgetItem(q.value(1).toString()));
        tbl->setItem(row, 2, new QTableWidgetItem(q.value(2).toString()));
        tbl->setItem(row, 3, new QTableWidgetItem(q.value(3).toString()));
        tbl->setItem(row, 4, new QTableWidgetItem(q.value(4).toString()));
        auto* sal = new QTableWidgetItem(QString("%1").arg(q.value(5).toDouble(), 0, 'f', 2));
        sal->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        tbl->setItem(row, 5, sal);
        tbl->setItem(row, 6, new QTableWidgetItem(q.value(6).toDate().toString("yyyy-MM-dd")));
        ++row;
    }
}

void HrWidget::onAddEmployee()
{
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle(tr("Add Employee"));
    auto* form = new QFormLayout(dlg);
    form->setSpacing(12); form->setContentsMargins(24,24,24,24);

    auto* firstEdit  = new QLineEdit(dlg);
    auto* lastEdit   = new QLineEdit(dlg);
    auto* deptEdit   = new QLineEdit(dlg);
    auto* posEdit    = new QLineEdit(dlg);
    auto* emailEdit  = new QLineEdit(dlg);
    auto* salaryEdit = new QDoubleSpinBox(dlg);
    salaryEdit->setRange(0, 9999999); salaryEdit->setDecimals(2);
    auto* hireDate   = new QDateEdit(QDate::currentDate(), dlg);
    hireDate->setCalendarPopup(true);
    auto* ccCombo    = new QComboBox(dlg);

    {
        auto token = DatabaseManager::instance().acquire();
        if (token) {
            QSqlQuery q(token->connection());
            q.exec("SELECT id, code FROM cost_centers ORDER BY code");
            while (q.next())
                ccCombo->addItem(q.value(1).toString(), q.value(0).toInt());
        }
    }

    form->addRow(tr("First Name:"),   firstEdit);
    form->addRow(tr("Last Name:"),    lastEdit);
    form->addRow(tr("Department:"),   deptEdit);
    form->addRow(tr("Position:"),     posEdit);
    form->addRow(tr("Email:"),        emailEdit);
    form->addRow(tr("Salary (EUR):"), salaryEdit);
    form->addRow(tr("Hire Date:"),    hireDate);
    form->addRow(tr("Cost Center:"),  ccCombo);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dlg);
    form->addRow(btns);
    connect(btns, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

    if (dlg->exec() != QDialog::Accepted) return;
    if (firstEdit->text().trimmed().isEmpty() || lastEdit->text().trimmed().isEmpty()) return;

    auto token = DatabaseManager::instance().acquire();
    if (!token) return;

    QSqlQuery seq(token->connection());
    seq.exec("SELECT NEXTVAL('seq_employee')");
    const QString empNum = seq.next()
        ? QString("EMP%1").arg(seq.value(0).toLongLong(), 6, 10, QChar('0'))
        : "EMP000000";

    QSqlQuery ins(token->connection());
    ins.prepare(
        "INSERT INTO employees "
        "(employee_number,first_name,last_name,department,position,email,salary,hire_date,cost_center_id,is_active) "
        "VALUES (:en,:fn,:ln,:dept,:pos,:email,:sal,:hd,:ccid,true)"
    );
    ins.bindValue(":en",   empNum);
    ins.bindValue(":fn",   firstEdit->text().trimmed());
    ins.bindValue(":ln",   lastEdit->text().trimmed());
    ins.bindValue(":dept", deptEdit->text().trimmed());
    ins.bindValue(":pos",  posEdit->text().trimmed());
    ins.bindValue(":email",emailEdit->text().trimmed());
    ins.bindValue(":sal",  salaryEdit->value());
    ins.bindValue(":hd",   hireDate->date());
    ins.bindValue(":ccid", ccCombo->currentData().toInt());

    if (ins.exec())
        QMessageBox::information(this, tr("Created"), tr("Employee %1 created.").arg(empNum));
    loadEmployees();
}

void HrWidget::loadPayrollRuns()
{
    auto* tbl = ui->payrollRunsTable;
    tbl->setRowCount(0);
    tbl->horizontalHeader()->setStretchLastSection(true);

    auto token = DatabaseManager::instance().acquire();
    if (!token) return;

    QSqlQuery q(token->connection());
    q.exec(
        "SELECT pr.pay_period, "
        "       SUM(pl.gross_pay), SUM(pl.tax_deduction), "
        "       SUM(pl.social_security), SUM(pl.net_pay), "
        "       pr.status, pr.journal_doc_num, pr.id "
        "FROM payroll_runs pr "
        "LEFT JOIN payroll_lines pl ON pl.payroll_run_id=pr.id "
        "GROUP BY pr.id, pr.pay_period, pr.status, pr.journal_doc_num "
        "ORDER BY pr.pay_period DESC LIMIT 24"
    );

    int row = 0;
    while (q.next()) {
        tbl->insertRow(row);
        tbl->setItem(row, 0, new QTableWidgetItem(q.value(0).toDate().toString("yyyy-MM")));
        for (int c = 1; c <= 4; ++c) {
            auto* it = new QTableWidgetItem(QString("%1").arg(q.value(c).toDouble(), 0,'f',2));
            it->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            tbl->setItem(row, c, it);
        }
        tbl->setItem(row, 5, new QTableWidgetItem(q.value(5).toString()));
        tbl->setItem(row, 6, new QTableWidgetItem(q.value(6).toString()));
        ++row;
    }
}

void HrWidget::onRunPayroll()
{
    const QDate period = ui->payPeriodEdit->date();
    const QDate periodStart = QDate(period.year(), period.month(), 1);
    const QDate periodEnd   = periodStart.addMonths(1).addDays(-1);

    auto token = DatabaseManager::instance().acquire();
    if (!token) return;
    QSqlDatabase db = token->connection();

    // Check if already run
    QSqlQuery check(db);
    check.prepare("SELECT id FROM payroll_runs WHERE pay_period=:p AND status != 'cancelled'");
    check.bindValue(":p", periodStart);
    if (check.exec() && check.next()) {
        QMessageBox::warning(this, tr("Already Run"),
            tr("Payroll for %1 has already been calculated.").arg(periodStart.toString("yyyy-MM")));
        return;
    }

    db.transaction();

    // Create payroll run
    QSqlQuery ins(db);
    ins.prepare(
        "INSERT INTO payroll_runs (pay_period, pay_period_end, status, run_by_user_id) "
        "VALUES (:ps, :pe, 'calculated', :uid) RETURNING id"
    );
    ins.bindValue(":ps",  periodStart);
    ins.bindValue(":pe",  periodEnd);
    ins.bindValue(":uid", m_session.userId);
    if (!ins.exec() || !ins.next()) { db.rollback(); return; }
    int runId = ins.value(0).toInt();

    // Calculate payroll lines for each active employee
    QSqlQuery emps(db);
    emps.exec("SELECT id, salary FROM employees WHERE is_active=true AND salary > 0");

    while (emps.next()) {
        int    empId  = emps.value(0).toInt();
        double monthly= emps.value(1).toDouble() / 12.0;
        double tax    = monthly * 0.19;          // simplified 19% tax
        double ss     = monthly * 0.0935;        // 9.35% employee social security
        double net    = monthly - tax - ss;

        QSqlQuery line(db);
        line.prepare(
            "INSERT INTO payroll_lines "
            "(payroll_run_id, employee_id, gross_pay, tax_deduction, social_security, net_pay, currency) "
            "VALUES (:rid, :eid, :gross, :tax, :ss, :net, 'EUR')"
        );
        line.bindValue(":rid",   runId);
        line.bindValue(":eid",   empId);
        line.bindValue(":gross", monthly);
        line.bindValue(":tax",   tax);
        line.bindValue(":ss",    ss);
        line.bindValue(":net",   net);
        line.exec();
    }

    db.commit();
    QMessageBox::information(this, tr("Payroll Calculated"),
        tr("Payroll run for %1 completed.").arg(periodStart.toString("yyyy-MM")));
    loadPayrollRuns();
}

void HrWidget::onPostPayroll()
{
    const int row = ui->payrollRunsTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, tr("Selection"), tr("Please select a payroll run."));
        return;
    }

    const QString status = ui->payrollRunsTable->item(row, 5)->text();
    if (status != "calculated") {
        QMessageBox::warning(this, tr("Status"), tr("Only calculated payroll runs can be posted."));
        return;
    }

    // Fetch totals for the selected run
    auto token = DatabaseManager::instance().acquire();
    if (!token) return;

    // Get the run id — we stored period to look up
    const QString period = ui->payrollRunsTable->item(row, 0)->text() + "-01";
    QSqlQuery q(token->connection());
    q.prepare(
        "SELECT pr.id, SUM(pl.gross_pay), SUM(pl.tax_deduction), SUM(pl.social_security) "
        "FROM payroll_runs pr "
        "JOIN payroll_lines pl ON pl.payroll_run_id=pr.id "
        "WHERE pr.pay_period=:p AND pr.status='calculated' "
        "GROUP BY pr.id"
    );
    q.bindValue(":p", QDate::fromString(period, "yyyy-MM-dd"));
    if (!q.exec() || !q.next()) return;

    int    runId = q.value(0).toInt();
    double gross = q.value(1).toDouble();
    double tax   = q.value(2).toDouble();
    double ss    = q.value(3).toDouble();

    postPayrollJournal(runId, gross, tax, ss, ui->payrollRunsTable->item(row, 0)->text());
    loadPayrollRuns();
}

void HrWidget::postPayrollJournal(int payrollRunId, double grossPay,
                                   double tax, double socialSecurity,
                                   const QString& period)
{
    double netPay  = grossPay - tax - socialSecurity;
    double empSS   = socialSecurity;
    double erSS    = grossPay * 0.0935; // employer matching contribution

    PostingRequest req;
    req.documentType   = DocumentType::HR;
    req.postingDate    = QDate::currentDate();
    req.documentDate   = QDate::currentDate();
    req.reference      = "PAYROLL-" + period;
    req.headerText     = "Payroll posting: " + period;
    req.postedByUserId = m_session.userId;

    // Dr 6000 Salaries (gross)
    JournalLine drSal; drSal.glAccount = 6000; drSal.amount = grossPay;
    drSal.text = "Gross salaries " + period; req.lines.append(drSal);

    // Dr 6100 Employer SS
    JournalLine drSS; drSS.glAccount = 6100; drSS.amount = erSS;
    drSS.text = "Employer social security " + period; req.lines.append(drSS);

    // Cr 2200 Accrued Salaries (net pay)
    JournalLine crNet; crNet.glAccount = 2200; crNet.amount = -netPay;
    crNet.text = "Net salary payable " + period; req.lines.append(crNet);

    // Cr 2100 Tax payable
    JournalLine crTax; crTax.glAccount = 2100; crTax.amount = -tax;
    crTax.text = "Employee income tax " + period; req.lines.append(crTax);

    // Cr 2300 Social security payable (employee + employer)
    JournalLine crSS; crSS.glAccount = 2300; crSS.amount = -(empSS + erSS);
    crSS.text = "Social security payable " + period; req.lines.append(crSS);

    auto result = PostManager::instance().post(req);
    if (!result.success) {
        QMessageBox::critical(this, tr("Posting Failed"), result.errorMessage);
        return;
    }

    // Update payroll run status
    auto token = DatabaseManager::instance().acquire();
    if (!token) return;
    QSqlQuery upd(token->connection());
    upd.prepare("UPDATE payroll_runs SET status='posted', journal_doc_num=:dn WHERE id=:id");
    upd.bindValue(":dn", result.documentNumber);
    upd.bindValue(":id", payrollRunId);
    upd.exec();

    QMessageBox::information(this, tr("Posted"),
        tr("Payroll posted as document %1").arg(result.documentNumber));
}
