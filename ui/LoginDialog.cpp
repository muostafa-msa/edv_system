#include "LoginDialog.hpp"
#include "ui_LoginDialog.h"
#include "core/config/AppConfig.hpp"
#include "core/security/AuthManager.hpp"

#include <QApplication>
#include <QTranslator>
#include <QCoreApplication>
#include <QKeyEvent>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

    // Pre-fill last used username
    ui->usernameEdit->setText(AppConfig::instance().lastUser());

    // Language combo — pre-select current setting
    const QString lang = AppConfig::instance().language();
    if      (lang == "de") ui->languageCombo->setCurrentIndex(1);
    else if (lang == "ar") ui->languageCombo->setCurrentIndex(2);
    else                   ui->languageCombo->setCurrentIndex(0);

    connect(ui->loginBtn,       &QPushButton::clicked,    this, &LoginDialog::onLogin);
    connect(ui->languageCombo,  qOverload<int>(&QComboBox::currentIndexChanged),
            this, &LoginDialog::onLanguageChanged);

    ui->usernameEdit->setFocus();
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::onLogin()
{
    ui->errorLabel->clear();
    ui->loginBtn->setEnabled(false);

    const QString user = ui->usernameEdit->text().trimmed();
    const QString pass = ui->passwordEdit->text();

    if (user.isEmpty() || pass.isEmpty()) {
        ui->errorLabel->setText(tr("Username and password are required."));
        ui->loginBtn->setEnabled(true);
        return;
    }

    // Ensure admin exists on first run
    (void)AuthManager::instance().ensureAdminExists();

    auto result = AuthManager::instance().authenticate(user, pass);
    if (!result) {
        ui->errorLabel->setText(tr("Login failed: ") + AuthManager::instance().lastError());
        ui->passwordEdit->clear();
        ui->passwordEdit->setFocus();
        ui->loginBtn->setEnabled(true);
        return;
    }

    m_session = *result;
    AppConfig::instance().setLastUser(user);
    accept();
}

void LoginDialog::onLanguageChanged(int index)
{
    static const QStringList langs = {"en", "de", "ar"};
    if (index >= 0 && index < langs.size())
        AppConfig::instance().setLanguage(langs[index]);
}
