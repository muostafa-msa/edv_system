#include "SettingsWidget.h"
#include "core/AppSettings.h"
#include "core/DatabaseManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFrame>
#include <QFont>
#include <QMessageBox>
#include <QSqlQuery>

SettingsWidget::SettingsWidget(QWidget *parent) : QWidget(parent)
{
    m_dbStatusLabel = new QLabel(this);
    m_dbVersionLabel = new QLabel(this);
    setupUi();
    loadCurrentSettings();
}

void SettingsWidget::setupUi()
{
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *content = new QWidget(scroll);
    scroll->setWidget(content);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(scroll);

    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(24);

    // ---- Page title ----
    auto *title = new QLabel(tr("Settings"), content);
    QFont hf;
    hf.setPointSize(20);
    hf.setBold(true);
    title->setFont(hf);
    layout->addWidget(title);

    // ====================================================
    // Database Connection
    // ====================================================
    auto *dbGroup = new QGroupBox(tr("Database Connection"), content);
    auto *dbLayout = new QVBoxLayout(dbGroup);
    dbLayout->setSpacing(12);

    auto *form = new QFormLayout;
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_hostEdit   = new QLineEdit(dbGroup);
    m_portSpin   = new QSpinBox(dbGroup);
    m_portSpin->setRange(1, 65535);
    m_dbNameEdit = new QLineEdit(dbGroup);
    m_userEdit   = new QLineEdit(dbGroup);
    m_passwordEdit = new QLineEdit(dbGroup);
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    form->addRow(tr("Host:"),     m_hostEdit);
    form->addRow(tr("Port:"),     m_portSpin);
    form->addRow(tr("Database:"), m_dbNameEdit);
    form->addRow(tr("Username:"), m_userEdit);
    form->addRow(tr("Password:"), m_passwordEdit);

    dbLayout->addLayout(form);

    m_dbStatusLabel->setWordWrap(true);
    dbLayout->addWidget(m_dbStatusLabel);

    auto *dbBtnRow = new QHBoxLayout;
    auto *testBtn  = new QPushButton(tr("Test Connection"), dbGroup);
    auto *saveDbBtn = new QPushButton(tr("Save && Reconnect"), dbGroup);
    saveDbBtn->setObjectName("primaryButton");
    dbBtnRow->addWidget(testBtn);
    dbBtnRow->addStretch();
    dbBtnRow->addWidget(saveDbBtn);
    dbLayout->addLayout(dbBtnRow);

    layout->addWidget(dbGroup);

    // ====================================================
    // Appearance
    // ====================================================
    auto *appGroup = new QGroupBox(tr("Appearance & Language"), content);
    auto *appLayout = new QVBoxLayout(appGroup);
    appLayout->setSpacing(12);

    auto *appForm = new QFormLayout;
    appForm->setSpacing(10);
    appForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_langCombo = new QComboBox(appGroup);
    m_langCombo->addItem("English",  "en");
    m_langCombo->addItem("Deutsch",  "de");
    m_langCombo->addItem("\u0627\u0644\u0639\u0631\u0628\u064a\u0629", "ar");

    m_themeCombo = new QComboBox(appGroup);
    m_themeCombo->addItem(tr("Dark Theme"),  "dark");
    m_themeCombo->addItem(tr("Light Theme"), "light");

    appForm->addRow(tr("Language:"), m_langCombo);
    appForm->addRow(tr("Theme:"),    m_themeCombo);
    appLayout->addLayout(appForm);

    auto *noteLabel = new QLabel(
        tr("Note: Language and theme changes take effect after restarting the application."),
        appGroup);
    noteLabel->setWordWrap(true);
    noteLabel->setObjectName("secondaryText");
    appLayout->addWidget(noteLabel);

    auto *applyAppBtn = new QPushButton(tr("Apply Appearance"), appGroup);
    applyAppBtn->setObjectName("primaryButton");
    applyAppBtn->setMaximumWidth(200);
    appLayout->addWidget(applyAppBtn);

    layout->addWidget(appGroup);

    // ====================================================
    // About
    // ====================================================
    auto *aboutGroup = new QGroupBox(tr("About"), content);
    auto *aboutLayout = new QVBoxLayout(aboutGroup);

    auto makeAboutRow = [&](const QString &label, const QString &value) {
        auto *row = new QHBoxLayout;
        auto *lbl = new QLabel("<b>" + label + "</b>", aboutGroup);
        auto *val = new QLabel(value, aboutGroup);
        val->setTextInteractionFlags(Qt::TextSelectableByMouse);
        row->addWidget(lbl, 1);
        row->addWidget(val, 3);
        aboutLayout->addLayout(row);
    };

    makeAboutRow(tr("Application:"), "EDV System v1.0");
    makeAboutRow(tr("Qt Version:"),  QString("Qt %1").arg(QT_VERSION_STR));
    makeAboutRow(tr("DB Server:"),   "");
    m_dbVersionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    aboutLayout->addWidget(m_dbVersionLabel);

    layout->addWidget(aboutGroup);
    layout->addStretch();

    // ---- Connections ----
    connect(testBtn,    &QPushButton::clicked, this, &SettingsWidget::onTestConnection);
    connect(saveDbBtn,  &QPushButton::clicked, this, &SettingsWidget::onSaveDbSettings);
    connect(applyAppBtn,&QPushButton::clicked, this, &SettingsWidget::onApplyAppearance);
}

void SettingsWidget::loadCurrentSettings()
{
    auto &s = AppSettings::instance();
    m_hostEdit->setText(s.dbHost());
    m_portSpin->setValue(s.dbPort());
    m_dbNameEdit->setText(s.dbName());
    m_userEdit->setText(s.dbUser());
    m_passwordEdit->setText(s.dbPassword());

    // Language
    QString lang = s.language();
    for (int i = 0; i < m_langCombo->count(); ++i) {
        if (m_langCombo->itemData(i).toString() == lang) {
            m_langCombo->setCurrentIndex(i);
            break;
        }
    }

    // Theme
    m_themeCombo->setCurrentIndex(s.darkMode() ? 0 : 1);

    // DB version
    QSqlQuery q(DatabaseManager::instance().database());
    if (q.exec("SELECT VERSION()") && q.next()) {
        m_dbVersionLabel->setText(q.value(0).toString().left(60));
    }
}

void SettingsWidget::onTestConnection()
{
    bool ok = DatabaseManager::instance().connect(
        m_hostEdit->text().trimmed(),
        m_portSpin->value(),
        m_dbNameEdit->text().trimmed(),
        m_userEdit->text().trimmed(),
        m_passwordEdit->text()
    );
    if (ok) {
        m_dbStatusLabel->setStyleSheet("color: #2EA043; font-weight: bold;");
        m_dbStatusLabel->setText(tr("Connection successful!"));
    } else {
        m_dbStatusLabel->setStyleSheet("color: #F85149; font-weight: bold;");
        m_dbStatusLabel->setText(tr("Failed: ") + DatabaseManager::instance().lastError());
    }
}

void SettingsWidget::onSaveDbSettings()
{
    auto &s = AppSettings::instance();
    s.setDbHost(m_hostEdit->text().trimmed());
    s.setDbPort(m_portSpin->value());
    s.setDbName(m_dbNameEdit->text().trimmed());
    s.setDbUser(m_userEdit->text().trimmed());
    s.setDbPassword(m_passwordEdit->text());
    s.setDbConfigured(true);

    bool ok = DatabaseManager::instance().connectFromSettings();
    if (ok) {
        m_dbStatusLabel->setStyleSheet("color: #2EA043; font-weight: bold;");
        m_dbStatusLabel->setText(tr("Settings saved and reconnected successfully."));
    } else {
        m_dbStatusLabel->setStyleSheet("color: #F85149; font-weight: bold;");
        m_dbStatusLabel->setText(tr("Settings saved but reconnection failed: ")
                                  + DatabaseManager::instance().lastError());
    }
}

void SettingsWidget::onApplyAppearance()
{
    auto &s = AppSettings::instance();
    s.setLanguage(m_langCombo->currentData().toString());
    s.setDarkMode(m_themeCombo->currentData().toString() == "dark");

    emit themeChanged(s.darkMode());
    emit languageChanged(s.language());

    QMessageBox::information(this, tr("Appearance"),
        tr("Theme and language settings saved. Restart the application to apply language changes."));
}
