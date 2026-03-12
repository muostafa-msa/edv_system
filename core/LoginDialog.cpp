#include "LoginDialog.h"
#include "DatabaseManager.h"
#include "AppSettings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QFrame>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QInputDialog>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setFixedSize(420, 560);

    // Center on screen
    if (QScreen *s = QApplication::primaryScreen()) {
        QRect sg = s->availableGeometry();
        move(sg.center() - rect().center());
    }

    setupUi();
}

void LoginDialog::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(40, 44, 40, 44);
    root->setSpacing(0);

    // ---- Branding ----
    auto *logoLbl = new QLabel("EDV System", this);
    logoLbl->setAlignment(Qt::AlignCenter);
    QFont lf;
    lf.setPointSize(24);
    lf.setBold(true);
    logoLbl->setFont(lf);
    logoLbl->setObjectName("loginLogo");

    auto *tagLbl = new QLabel(tr("Enterprise Resource Planning"), this);
    tagLbl->setAlignment(Qt::AlignCenter);
    tagLbl->setObjectName("loginTagline");

    root->addWidget(logoLbl);
    root->addSpacing(4);
    root->addWidget(tagLbl);
    root->addSpacing(28);

    // ---- First-run notice ----
    m_firstRunLabel = new QLabel(this);
    m_firstRunLabel->setObjectName("firstRunLabel");
    m_firstRunLabel->setWordWrap(true);
    m_firstRunLabel->setAlignment(Qt::AlignCenter);
    m_firstRunLabel->hide();
    root->addWidget(m_firstRunLabel);

    // ---- Language selector ----
    m_langCombo = new QComboBox(this);
    m_langCombo->addItem("English",  "en");
    m_langCombo->addItem("Deutsch",  "de");
    m_langCombo->addItem("\u0627\u0644\u0639\u0631\u0628\u064a\u0629", "ar"); // Arabic
    // Select saved language
    QString saved = AppSettings::instance().language();
    for (int i = 0; i < m_langCombo->count(); ++i) {
        if (m_langCombo->itemData(i).toString() == saved) {
            m_langCombo->setCurrentIndex(i);
            break;
        }
    }
    root->addWidget(m_langCombo);
    root->addSpacing(12);

    // ---- Username ----
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText(tr("Username"));
    m_usernameEdit->setObjectName("loginField");
    m_usernameEdit->setText(AppSettings::instance().currentUsername);
    root->addWidget(m_usernameEdit);
    root->addSpacing(10);

    // ---- Password ----
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText(tr("Password"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setObjectName("loginField");
    root->addWidget(m_passwordEdit);
    root->addSpacing(6);

    // ---- Error label ----
    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName("loginError");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setMinimumHeight(32);
    root->addWidget(m_errorLabel);
    root->addSpacing(4);

    // ---- Login button ----
    m_loginBtn = new QPushButton(tr("Sign In"), this);
    m_loginBtn->setObjectName("primaryButton");
    m_loginBtn->setCursor(Qt::PointingHandCursor);
    m_loginBtn->setMinimumHeight(46);
    root->addWidget(m_loginBtn);

    root->addStretch();

    // ---- Version footer ----
    auto *verLbl = new QLabel("EDV System v1.0", this);
    verLbl->setAlignment(Qt::AlignCenter);
    verLbl->setObjectName("loginVersion");
    root->addWidget(verLbl);

    // ---- Signals ----
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLogin);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLogin);
    connect(m_langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LoginDialog::onLanguageChanged);

    // Check first run
    if (isFirstRun()) {
        m_firstRunLabel->setText(
            tr("No user accounts found. Please create the first administrator account."));
        m_firstRunLabel->show();
        m_loginBtn->setText(tr("Create Admin Account"));
        disconnect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLogin);
        connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onFirstRunSetup);
    }
}

void LoginDialog::onLanguageChanged(int index)
{
    QString lang = m_langCombo->itemData(index).toString();
    AppSettings::instance().setLanguage(lang);
    // Note: full language reload happens after login in main.cpp
}

QString LoginDialog::username() const
{
    return m_usernameEdit->text().trimmed();
}

QString LoginDialog::selectedLang() const
{
    return m_langCombo->currentData().toString();
}

void LoginDialog::onLogin()
{
    m_errorLabel->setText("");
    QString user = m_usernameEdit->text().trimmed();
    QString pass = m_passwordEdit->text();

    if (user.isEmpty() || pass.isEmpty()) {
        m_errorLabel->setText(tr("Please enter username and password."));
        return;
    }

    if (validateCredentials(user, pass)) {
        AppSettings::instance().currentUsername = user;
        accept();
    } else {
        m_errorLabel->setText(tr("Invalid username or password."));
        m_passwordEdit->clear();
        m_passwordEdit->setFocus();
    }
}

bool LoginDialog::validateCredentials(const QString &user, const QString &pass)
{
    QByteArray hash = QCryptographicHash::hash(pass.toUtf8(), QCryptographicHash::Sha256).toHex();

    QSqlQuery q(DatabaseManager::instance().database());
    q.prepare(
        "SELECT id, role FROM users "
        "WHERE username = :u AND password_hash = :h AND is_active = true"
    );
    q.bindValue(":u", user);
    q.bindValue(":h", QString::fromLatin1(hash));

    if (q.exec() && q.next()) {
        m_userId   = q.value(0).toInt();
        m_userRole = q.value(1).toString();

        QSqlQuery upd(DatabaseManager::instance().database());
        upd.prepare("UPDATE users SET last_login = NOW() WHERE id = :id");
        upd.bindValue(":id", m_userId);
        upd.exec();
        return true;
    }
    return false;
}

bool LoginDialog::isFirstRun()
{
    QSqlQuery q(DatabaseManager::instance().database());
    q.exec("SELECT COUNT(*) FROM users");
    if (q.next())
        return q.value(0).toInt() == 0;
    return false;
}

void LoginDialog::onFirstRunSetup()
{
    QString user = m_usernameEdit->text().trimmed();
    QString pass = m_passwordEdit->text();

    if (user.isEmpty() || pass.isEmpty()) {
        m_errorLabel->setText(tr("Enter a username and password for the admin account."));
        return;
    }
    if (pass.length() < 6) {
        m_errorLabel->setText(tr("Password must be at least 6 characters."));
        return;
    }

    createAdminAccount(user, pass);
    m_firstRunLabel->hide();
    m_loginBtn->setText(tr("Sign In"));
    disconnect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onFirstRunSetup);
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLogin);
    m_errorLabel->setText(tr("Admin account created. Please sign in."));
    m_passwordEdit->clear();
}

void LoginDialog::createAdminAccount(const QString &user, const QString &pass)
{
    QByteArray hash = QCryptographicHash::hash(pass.toUtf8(), QCryptographicHash::Sha256).toHex();

    QSqlQuery q(DatabaseManager::instance().database());
    q.prepare(
        "INSERT INTO users (username, password_hash, role, is_active) "
        "VALUES (:u, :h, 'admin', true)"
    );
    q.bindValue(":u", user);
    q.bindValue(":h", QString::fromLatin1(hash));
    q.exec();
}
