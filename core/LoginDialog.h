#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

class QLineEdit;
class QPushButton;
class QLabel;
class QComboBox;

class LoginDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);

    QString username()    const;
    QString selectedLang() const;
    int     userId()      const { return m_userId; }
    QString userRole()    const { return m_userRole; }

private slots:
    void onLogin();
    void onLanguageChanged(int index);
    void onFirstRunSetup();

private:
    void setupUi();
    bool validateCredentials(const QString &user, const QString &pass);
    bool isFirstRun();
    void createAdminAccount(const QString &user, const QString &pass);

    QLineEdit   *m_usernameEdit;
    QLineEdit   *m_passwordEdit;
    QPushButton *m_loginBtn;
    QLabel      *m_errorLabel;
    QLabel      *m_firstRunLabel;
    QComboBox   *m_langCombo;

    int     m_userId   = -1;
    QString m_userRole;
};

#endif // LOGINDIALOG_H
