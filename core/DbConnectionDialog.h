#ifndef DBCONNECTIONDIALOG_H
#define DBCONNECTIONDIALOG_H

#include <QDialog>

class QLineEdit;
class QSpinBox;
class QPushButton;
class QLabel;

class DbConnectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DbConnectionDialog(QWidget *parent = nullptr);

private slots:
    void onTest();
    void onConnect();

private:
    void setupUi();

    QLineEdit   *m_hostEdit;
    QSpinBox    *m_portSpin;
    QLineEdit   *m_dbNameEdit;
    QLineEdit   *m_userEdit;
    QLineEdit   *m_passwordEdit;
    QPushButton *m_testBtn;
    QPushButton *m_connectBtn;
    QLabel      *m_statusLabel;
};

#endif // DBCONNECTIONDIALOG_H
