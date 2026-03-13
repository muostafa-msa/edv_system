#include "DbSetupDialog.hpp"
#include "ui_DbSetupDialog.h"
#include "core/config/AppConfig.hpp"
#include "core/database/DatabaseManager.hpp"

#include <QMessageBox>

DbSetupDialog::DbSetupDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::DbSetupDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Pre-fill from saved settings
    const auto& cfg = AppConfig::instance();
    ui->hostEdit->setText(cfg.dbHost());
    ui->portSpin->setValue(cfg.dbPort());
    ui->dbNameEdit->setText(cfg.dbName());
    ui->userEdit->setText(cfg.dbUser());
    ui->passwordEdit->setText(cfg.dbPassword());
    ui->poolSpin->setValue(cfg.dbPoolSize());

    connect(ui->testBtn,    &QPushButton::clicked, this, &DbSetupDialog::onTestConnection);
    connect(ui->connectBtn, &QPushButton::clicked, this, &DbSetupDialog::onConnect);
}

DbSetupDialog::~DbSetupDialog()
{
    delete ui;
}

void DbSetupDialog::onTestConnection()
{
    ui->errorLabel->clear();
    ui->testBtn->setEnabled(false);

    auto& db = DatabaseManager::instance();
    bool ok = db.connect(
        ui->hostEdit->text(),
        ui->portSpin->value(),
        ui->dbNameEdit->text(),
        ui->userEdit->text(),
        ui->passwordEdit->text(),
        1  // single connection for test
    );

    if (ok) {
        db.shutdown(); // release test connection
        QMessageBox::information(this, tr("Connection Test"),
            tr("Connection successful!"));
    } else {
        ui->errorLabel->setText(tr("Error: ") + db.lastError());
    }

    ui->testBtn->setEnabled(true);
}

void DbSetupDialog::onConnect()
{
    ui->errorLabel->clear();
    ui->connectBtn->setEnabled(false);

    const QString host  = ui->hostEdit->text().trimmed();
    const QString dbName= ui->dbNameEdit->text().trimmed();
    const QString user  = ui->userEdit->text().trimmed();

    if (host.isEmpty() || dbName.isEmpty() || user.isEmpty()) {
        ui->errorLabel->setText(tr("Host, database name, and username are required."));
        ui->connectBtn->setEnabled(true);
        return;
    }

    // Persist settings
    auto& cfg = AppConfig::instance();
    cfg.setDbHost(host);
    cfg.setDbPort(ui->portSpin->value());
    cfg.setDbName(dbName);
    cfg.setDbUser(user);
    cfg.setDbPassword(ui->passwordEdit->text());
    cfg.setDbPoolSize(ui->poolSpin->value());
    cfg.setDbConfigured(true);

    accept();
}
