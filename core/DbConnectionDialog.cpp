#include "DbConnectionDialog.h"
#include "DatabaseManager.h"
#include "AppSettings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>

DbConnectionDialog::DbConnectionDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Database Connection Setup"));
    setMinimumWidth(460);
    setupUi();
}

void DbConnectionDialog::setupUi()
{
    auto &cfg = AppSettings::instance();

    auto *root = new QVBoxLayout(this);
    root->setSpacing(14);
    root->setContentsMargins(28, 28, 28, 28);

    // ---- Title ----
    auto *title = new QLabel(tr("Connect to PostgreSQL Server"), this);
    title->setObjectName("dialogTitle");
    QFont f = title->font();
    f.setPointSize(16);
    f.setBold(true);
    title->setFont(f);
    root->addWidget(title);

    auto *sub = new QLabel(tr("Enter the connection details for your PostgreSQL server."), this);
    sub->setObjectName("dialogSubtitle");
    root->addWidget(sub);

    // ---- Separator ----
    auto *sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    root->addWidget(sep);

    // ---- Form ----
    auto *form = new QFormLayout;
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_hostEdit = new QLineEdit(cfg.dbHost(), this);
    m_hostEdit->setPlaceholderText("localhost");

    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(cfg.dbPort());

    m_dbNameEdit = new QLineEdit(cfg.dbName(), this);
    m_dbNameEdit->setPlaceholderText("edv_system");

    m_userEdit = new QLineEdit(cfg.dbUser(), this);
    m_userEdit->setPlaceholderText("postgres");

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(tr("Enter password"));
    m_passwordEdit->setText(cfg.dbPassword());

    form->addRow(tr("Host:"),     m_hostEdit);
    form->addRow(tr("Port:"),     m_portSpin);
    form->addRow(tr("Database:"), m_dbNameEdit);
    form->addRow(tr("Username:"), m_userEdit);
    form->addRow(tr("Password:"), m_passwordEdit);

    root->addLayout(form);

    // ---- Status label ----
    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setMinimumHeight(36);
    root->addWidget(m_statusLabel);

    // ---- Buttons ----
    auto *btnRow = new QHBoxLayout;
    m_testBtn    = new QPushButton(tr("Test Connection"), this);
    m_connectBtn = new QPushButton(tr("Connect && Save"), this);
    m_connectBtn->setObjectName("primaryButton");

    btnRow->addWidget(m_testBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_connectBtn);
    root->addLayout(btnRow);

    connect(m_testBtn,    &QPushButton::clicked, this, &DbConnectionDialog::onTest);
    connect(m_connectBtn, &QPushButton::clicked, this, &DbConnectionDialog::onConnect);
}

void DbConnectionDialog::onTest()
{
    m_statusLabel->setStyleSheet("color: #888;");
    m_statusLabel->setText(tr("Testing connection…"));

    bool ok = DatabaseManager::instance().connect(
        m_hostEdit->text().trimmed(),
        m_portSpin->value(),
        m_dbNameEdit->text().trimmed(),
        m_userEdit->text().trimmed(),
        m_passwordEdit->text()
    );

    if (ok) {
        m_statusLabel->setStyleSheet("color: #2EA043; font-weight: bold;");
        m_statusLabel->setText(tr("Connection successful!"));
    } else {
        m_statusLabel->setStyleSheet("color: #F85149; font-weight: bold;");
        m_statusLabel->setText(tr("Failed: ") + DatabaseManager::instance().lastError());
    }
}

void DbConnectionDialog::onConnect()
{
    auto &cfg = AppSettings::instance();
    cfg.setDbHost(m_hostEdit->text().trimmed());
    cfg.setDbPort(m_portSpin->value());
    cfg.setDbName(m_dbNameEdit->text().trimmed());
    cfg.setDbUser(m_userEdit->text().trimmed());
    cfg.setDbPassword(m_passwordEdit->text());
    cfg.setDbConfigured(true);
    accept();
}
