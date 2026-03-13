#include "LoginPage.hpp"
#include "ui_LoginPage.h"
#include "core/config/AppConfig.hpp"
#include "core/security/AuthManager.hpp"

#include <QKeyEvent>
#include <QApplication>

LoginPage::LoginPage(QWidget *parent)
    : QWidget(parent), ui(new Ui::LoginPage)
{
    ui->setupUi(this);

    // Pre-fill last username
    ui->usernameEdit->setText(AppConfig::instance().lastUser());

    // Language pre-select
    const QString lang = AppConfig::instance().language();
    if      (lang == "de") ui->languageCombo->setCurrentIndex(1);
    else if (lang == "ar") ui->languageCombo->setCurrentIndex(2);
    else                   ui->languageCombo->setCurrentIndex(0);

    connect(ui->loginBtn,      &QPushButton::clicked,
            this,              &LoginPage::onLoginClicked);
    connect(ui->dbSettingsBtn, &QPushButton::clicked,
            this,              &LoginPage::onDbSettings);
    connect(ui->languageCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this,              &LoginPage::onLanguageChanged);

    // Allow Enter key on password field to trigger login
    connect(ui->passwordEdit, &QLineEdit::returnPressed,
            ui->loginBtn,     &QPushButton::click);

    ui->usernameEdit->setFocus();
}

LoginPage::~LoginPage() { delete ui; }

void LoginPage::setError(const QString& msg)
{
    ui->errorLabel->setText(msg);
}

void LoginPage::clearError()
{
    ui->errorLabel->clear();
}

void LoginPage::resetForm()
{
    ui->passwordEdit->clear();
    ui->errorLabel->clear();
    ui->loginBtn->setEnabled(true);
    ui->usernameEdit->setFocus();
}

void LoginPage::onLoginClicked()
{
    clearError();
    ui->loginBtn->setEnabled(false);

    const QString user = ui->usernameEdit->text().trimmed();
    const QString pass = ui->passwordEdit->text();

    if (user.isEmpty() || pass.isEmpty()) {
        setError(tr("Username and password are required."));
        ui->loginBtn->setEnabled(true);
        return;
    }

    // First-run: auto-create admin account
    (void)AuthManager::instance().ensureAdminExists();

    auto result = AuthManager::instance().authenticate(user, pass);
    if (!result) {
        setError(tr("Login failed: ") + AuthManager::instance().lastError());
        ui->passwordEdit->clear();
        ui->passwordEdit->setFocus();
        ui->loginBtn->setEnabled(true);
        return;
    }

    AppConfig::instance().setLastUser(user);
    emit loginSuccessful(*result);
}

void LoginPage::onLanguageChanged(int index)
{
    static const QStringList langs = {"en", "de", "ar"};
    if (index >= 0 && index < langs.size()) {
        AppConfig::instance().setLanguage(langs[index]);
        qApp->setLayoutDirection(langs[index] == "ar" ? Qt::RightToLeft : Qt::LeftToRight);
    }
}

void LoginPage::onDbSettings()
{
    emit dbSettingsRequested();
}
