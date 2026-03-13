#pragma once

#include "core/security/Session.hpp"
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class HrWidget; }
QT_END_NAMESPACE

class HrWidget : public QWidget {
    Q_OBJECT
public:
    explicit HrWidget(const Session& session, QWidget *parent = nullptr);
    ~HrWidget() override;

private slots:
    void loadEmployees();
    void onAddEmployee();
    void loadPayrollRuns();
    void onRunPayroll();
    void onPostPayroll();

private:
    void postPayrollJournal(int payrollRunId, double grossPay, double tax,
                            double socialSecurity, const QString& period);

    Ui::HrWidget *ui;
    Session       m_session;
};
