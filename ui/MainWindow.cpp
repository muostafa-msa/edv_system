#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "LoginPage.hpp"
#include "DashboardWidget.hpp"
#include "DbSetupDialog.hpp"
#include "core/config/AppConfig.hpp"
#include "core/database/DatabaseManager.hpp"
#include "core/ui/ThemeManager.hpp"

#include "modules/fico/ui/FicoWidget.hpp"
#include "modules/sd/ui/SdWidget.hpp"
#include "modules/mm/ui/MmWidget.hpp"
#include "modules/pp/ui/PpWidget.hpp"
#include "modules/hr/ui/HrWidget.hpp"

#include <QApplication>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QFont>
#include <QMessageBox>
#include <QVBoxLayout>

namespace {

QPushButton* makeSidebarBtn(const QString& icon, const QString& label, QWidget* parent)
{
    auto* btn = new QPushButton(QString("  %1  %2").arg(icon, label), parent);
    btn->setObjectName("sidebarBtn");
    btn->setCheckable(true);
    btn->setFlat(true);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setMinimumHeight(46);
    btn->setMaximumHeight(46);
    QFont f = btn->font();
    f.setPointSize(11);
    btn->setFont(f);
    return btn;
}

} // anonymous namespace

// ── Constructor ───────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("EDV Enterprise System");
    resize(1440, 860);
    setMinimumSize(1100, 680);

    m_dbStatusLabel = new QLabel(tr("  DB: Connecting..."), this);
    m_dbStatusLabel->setStyleSheet("color: #D29922;");
    statusBar()->addPermanentWidget(m_dbStatusLabel);

    connect(ui->themeToggleBtn, &QPushButton::clicked, this, &MainWindow::onThemeToggle);
    connect(ui->logoutBtn,      &QPushButton::clicked, this, &MainWindow::onLogout);

    showLoginPage();
}

MainWindow::~MainWindow() { delete ui; }

// ── Login page ────────────────────────────────────────────────────────────────

void MainWindow::showLoginPage()
{
    setSidebarVisible(false);
    ui->topHeader->hide();

    if (!m_loginPage) {
        m_loginPage = new LoginPage(this);
        ui->moduleStack->insertWidget(0, m_loginPage);

        connect(m_loginPage, &LoginPage::loginSuccessful,
                this,        &MainWindow::onLoginSuccessful);
        connect(m_loginPage, &LoginPage::dbSettingsRequested,
                this,        &MainWindow::onDbSettingsRequested);
    }

    m_loginPage->resetForm();
    ui->moduleStack->setCurrentIndex(0);
    statusBar()->showMessage(tr("  Please sign in to continue."));
}

void MainWindow::onLoginSuccessful(const Session& session)
{
    m_session = session;
    showMainContent(session);
}

void MainWindow::onDbSettingsRequested()
{
    DbSetupDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        auto& db = DatabaseManager::instance();
        if (db.connectFromConfig()) {
            m_dbStatusLabel->setText(tr("  DB: Connected"));
            m_dbStatusLabel->setStyleSheet("color: #2EA043;");
        } else {
            QMessageBox::critical(this, tr("Connection Failed"),
                tr("Could not connect:\n\n%1").arg(db.lastError()));
        }
    }
}

// ── Main content ──────────────────────────────────────────────────────────────

void MainWindow::showMainContent(const Session& session)
{
    // Build module stack only once (modules start at index 1; index 0 = login)
    if (ui->moduleStack->count() <= 1) {
        buildSidebarButtons();
        buildModuleStack();
    }

    ui->userDisplayLabel->setText(QString("● %1").arg(session.displayName));
    ui->roleDisplayLabel->setText("  " + edv::core::security::roleToString(session.role));
    ui->headerUserLabel->setText(QString("● %1").arg(session.username));

    setSidebarVisible(true);
    ui->topHeader->show();

    const bool dark = AppConfig::instance().darkMode();
    ThemeManager::instance().applyTheme(dark ? ThemeType::Dark : ThemeType::Light);
    ui->themeToggleBtn->setText(dark ? "☀" : "☾");

    statusBar()->showMessage(
        tr("  Logged in as: %1  |  Role: %2")
            .arg(session.username, edv::core::security::roleToString(session.role))
    );
    m_dbStatusLabel->setText(tr("  DB: Connected"));
    m_dbStatusLabel->setStyleSheet("color: #2EA043;");

    auto* pingTimer = new QTimer(this);
    connect(pingTimer, &QTimer::timeout, this, &MainWindow::updateDbStatus);
    pingTimer->start(15000);

    switchModule(1); // Dashboard
}

void MainWindow::buildSidebarButtons()
{
    struct NavItem { QString icon; QString label; int idx; };
    const QList<NavItem> items = {
        {"▣", tr("Dashboard"),        1},
        {"◉", tr("Sales (SD)"),       2},
        {"▤", tr("Inventory (MM)"),   3},
        {"⚙", tr("Production (PP)"),  4},
        {"◑", tr("Human Resources"),  5},
        {"◈", tr("Finance (FICO)"),   6},
    };

    auto* layout = qobject_cast<QVBoxLayout*>(ui->navButtonsWidget->layout());
    if (!layout) return;

    m_navBtns.clear();
    for (const auto& item : items) {
        auto* btn = makeSidebarBtn(item.icon, item.label, ui->navButtonsWidget);
        m_navBtns.append(btn);
        layout->addWidget(btn);
        const int idx = item.idx;
        connect(btn, &QPushButton::clicked, this, [this, idx]() { switchModule(idx); });
    }
    layout->addStretch();
}

void MainWindow::buildModuleStack()
{
    // index 0 = LoginPage (already inserted)
    ui->moduleStack->addWidget(new DashboardWidget(ui->moduleStack));       // 1
    ui->moduleStack->addWidget(new SdWidget(m_session, ui->moduleStack));   // 2
    ui->moduleStack->addWidget(new MmWidget(m_session, ui->moduleStack));   // 3
    ui->moduleStack->addWidget(new PpWidget(m_session, ui->moduleStack));   // 4
    ui->moduleStack->addWidget(new HrWidget(m_session, ui->moduleStack));   // 5
    ui->moduleStack->addWidget(new FicoWidget(m_session, ui->moduleStack)); // 6
}

void MainWindow::setSidebarVisible(bool visible)
{
    ui->sidebar->setVisible(visible);
}

// ── Module switching ──────────────────────────────────────────────────────────

void MainWindow::switchModule(int index)
{
    if (index < 1 || index >= ui->moduleStack->count()) return;

    m_currentModule = index;
    ui->moduleStack->setCurrentIndex(index);

    const int btnIdx = index - 1;
    for (int i = 0; i < m_navBtns.size(); ++i)
        m_navBtns[i]->setChecked(i == btnIdx);

    const QStringList titles = {
        "",
        tr("Dashboard"),
        tr("Sales & Distribution"),
        tr("Materials Management"),
        tr("Production Planning"),
        tr("Human Resources"),
        tr("Finance & Controlling"),
    };
    ui->pageTitleLabel->setText(titles.value(index, "EDV Enterprise"));
}

// ── Theme ─────────────────────────────────────────────────────────────────────

void MainWindow::applyTheme(bool dark)
{
    ThemeManager::instance().applyTheme(dark ? ThemeType::Dark : ThemeType::Light);
    ui->themeToggleBtn->setText(dark ? "☀" : "☾");
}

void MainWindow::loadTheme(bool dark) { applyTheme(dark); }

void MainWindow::onThemeToggle()
{
    ThemeManager::instance().toggle();
    ui->themeToggleBtn->setText(ThemeManager::instance().isDark() ? "☀" : "☾");
}

// ── Status / Logout ───────────────────────────────────────────────────────────

void MainWindow::updateDbStatus()
{
    const bool ok = DatabaseManager::instance().isReady();
    m_dbStatusLabel->setText(ok ? tr("  DB: Connected") : tr("  DB: Disconnected"));
    m_dbStatusLabel->setStyleSheet(ok ? "color: #2EA043;" : "color: #F85149;");
}

void MainWindow::onLogout()
{
    if (QMessageBox::question(this, tr("Logout"),
            tr("Are you sure you want to log out?"),
            QMessageBox::Yes | QMessageBox::Cancel) != QMessageBox::Yes)
        return;

    m_session = Session::invalid();
    showLoginPage();
}
