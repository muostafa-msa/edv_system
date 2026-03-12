#include "mainwindow.h"
#include "core/AppSettings.h"
#include "core/DatabaseManager.h"
#include "ui/DashboardWidget.h"
#include "ui/CrmWidget.h"
#include "ui/InventoryWidget.h"
#include "ui/HrWidget.h"
#include "ui/FinanceWidget.h"
#include "ui/SettingsWidget.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QStatusBar>
#include <QTimer>
#include <QFont>
#include <QFile>
#include <QMessageBox>

// ─────────────────────────────────────────────────────────────────
// Sidebar button: text + active-state indicator
// ─────────────────────────────────────────────────────────────────
static QPushButton *makeSidebarButton(const QString &icon, const QString &label,
                                      QWidget *parent)
{
    auto *btn = new QPushButton(QString("  %1  %2").arg(icon, label), parent);
    btn->setObjectName("sidebarBtn");
    btn->setCheckable(true);
    btn->setFlat(true);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setMinimumHeight(46);
    btn->setMaximumHeight(46);
    QFont f = btn->font();
    f.setPointSize(12);
    btn->setFont(f);
    btn->setStyleSheet("");
    return btn;
}

// ─────────────────────────────────────────────────────────────────
MainWindow::MainWindow(const QString &username, int userId,
                       const QString &role, QWidget *parent)
    : QMainWindow(parent), m_username(username), m_userId(userId), m_userRole(role)
{
    setWindowTitle("EDV System");
    setMinimumSize(1100, 680);
    resize(1300, 800);
    buildUi();
    loadTheme(AppSettings::instance().darkMode());

    // Status bar: connection indicator
    QTimer *pingTimer = new QTimer(this);
    connect(pingTimer, &QTimer::timeout, [this]() {
        bool ok = DatabaseManager::instance().isConnected();
        m_statusDbLabel->setText(ok ? tr("  DB: Connected") : tr("  DB: Disconnected"));
        m_statusDbLabel->setStyleSheet(ok ? "color: #2EA043;" : "color: #F85149;");
    });
    pingTimer->start(10000);

    // Initial active button
    m_navBtns[0]->setChecked(true);
}

void MainWindow::buildUi()
{
    // Central widget with horizontal layout: sidebar | right panel
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *mainRow = new QHBoxLayout(central);
    mainRow->setContentsMargins(0, 0, 0, 0);
    mainRow->setSpacing(0);

    // ---- Sidebar ----
    m_sidebar = new QFrame(central);
    m_sidebar->setObjectName("sidebar");
    m_sidebar->setFixedWidth(230);
    buildSidebar(m_sidebar);
    mainRow->addWidget(m_sidebar);

    // ---- Right panel ----
    auto *rightPanel = new QWidget(central);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // Header
    m_header = new QWidget(rightPanel);
    m_header->setObjectName("topHeader");
    m_header->setFixedHeight(56);
    buildHeader(m_header);
    rightLayout->addWidget(m_header);

    // Content stack
    buildContent();
    rightLayout->addWidget(m_stack);

    mainRow->addWidget(rightPanel, 1);

    // ---- Status bar ----
    m_statusDbLabel = new QLabel(this);
    m_statusDbLabel->setText(tr("  DB: Connected"));
    m_statusDbLabel->setStyleSheet("color: #2EA043;");

    statusBar()->addPermanentWidget(m_statusDbLabel);
    statusBar()->showMessage(tr("  Logged in as: %1 (%2)").arg(m_username, m_userRole));
    statusBar()->setObjectName("mainStatusBar");
}

void MainWindow::buildSidebar(QWidget *sidebar)
{
    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ---- Logo area ----
    auto *logoFrame = new QWidget(sidebar);
    logoFrame->setObjectName("sidebarLogo");
    logoFrame->setFixedHeight(70);
    auto *logoLayout = new QVBoxLayout(logoFrame);
    logoLayout->setContentsMargins(16, 12, 16, 12);
    logoLayout->setSpacing(2);

    auto *logoLbl = new QLabel("EDV System", logoFrame);
    logoLbl->setObjectName("sidebarLogoText");
    QFont lf;
    lf.setPointSize(16);
    lf.setBold(true);
    logoLbl->setFont(lf);

    auto *subLbl = new QLabel(tr("Enterprise Suite"), logoFrame);
    subLbl->setObjectName("sidebarLogoSub");

    logoLayout->addWidget(logoLbl);
    logoLayout->addWidget(subLbl);
    layout->addWidget(logoFrame);

    // ---- Separator ----
    auto *sep = new QFrame(sidebar);
    sep->setFrameShape(QFrame::HLine);
    sep->setObjectName("sidebarSeparator");
    layout->addWidget(sep);
    layout->addSpacing(8);

    // ---- Navigation buttons ----
    struct NavItem { QString icon; QString label; };
    const QList<NavItem> items = {
        {"▣",  tr("Dashboard")},
        {"◉",  tr("Customers")},
        {"▤",  tr("Inventory")},
        {"◑",  tr("Human Resources")},
        {"◈",  tr("Finance")},
        {"⚙",  tr("Settings")},
    };

    auto *navWidget = new QWidget(sidebar);
    auto *navLayout = new QVBoxLayout(navWidget);
    navLayout->setContentsMargins(8, 0, 8, 0);
    navLayout->setSpacing(2);

    m_navBtns.clear();
    for (int i = 0; i < items.size(); ++i) {
        auto *btn = makeSidebarButton(items[i].icon, items[i].label, navWidget);
        m_navBtns.append(btn);
        navLayout->addWidget(btn);

        connect(btn, &QPushButton::clicked, this, [this, i]() {
            switchModule(i);
        });
    }

    navLayout->addStretch();
    layout->addWidget(navWidget, 1);

    // ---- Bottom: user info + logout ----
    auto *sep2 = new QFrame(sidebar);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setObjectName("sidebarSeparator");
    layout->addWidget(sep2);

    auto *bottomWidget = new QWidget(sidebar);
    auto *bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(8, 8, 8, 12);
    bottomLayout->setSpacing(4);

    auto *userInfoLbl = new QLabel(QString("  \u25CF  %1").arg(m_username), bottomWidget);
    userInfoLbl->setObjectName("sidebarUserInfo");
    auto *roleLbl = new QLabel("     " + m_userRole, bottomWidget);
    roleLbl->setObjectName("sidebarRoleLabel");

    auto *logoutBtn = new QPushButton("  \u2715  " + tr("Logout"), bottomWidget);
    logoutBtn->setObjectName("logoutBtn");
    logoutBtn->setCursor(Qt::PointingHandCursor);
    logoutBtn->setMinimumHeight(40);

    bottomLayout->addWidget(userInfoLbl);
    bottomLayout->addWidget(roleLbl);
    bottomLayout->addSpacing(4);
    bottomLayout->addWidget(logoutBtn);

    layout->addWidget(bottomWidget);

    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogout);
}

void MainWindow::buildHeader(QWidget *header)
{
    auto *layout = new QHBoxLayout(header);
    layout->setContentsMargins(24, 0, 24, 0);
    layout->setSpacing(16);

    m_pageTitleLabel = new QLabel(tr("Dashboard"), header);
    m_pageTitleLabel->setObjectName("headerPageTitle");
    QFont pf;
    pf.setPointSize(14);
    pf.setBold(true);
    m_pageTitleLabel->setFont(pf);

    layout->addWidget(m_pageTitleLabel);
    layout->addStretch();

    // User label
    m_userLabel = new QLabel(QString("\u25CF  %1").arg(m_username), header);
    m_userLabel->setObjectName("headerUserLabel");
    layout->addWidget(m_userLabel);

    // Theme toggle button
    auto *themeBtn = new QPushButton(AppSettings::instance().darkMode() ? "☀" : "☾", header);
    themeBtn->setObjectName("iconButton");
    themeBtn->setFixedSize(36, 36);
    themeBtn->setCursor(Qt::PointingHandCursor);
    themeBtn->setToolTip(tr("Toggle Theme"));
    layout->addWidget(themeBtn);

    connect(themeBtn, &QPushButton::clicked, [this, themeBtn]() {
        bool nowDark = !AppSettings::instance().darkMode();
        AppSettings::instance().setDarkMode(nowDark);
        themeBtn->setText(nowDark ? "☀" : "☾");
        applyTheme(nowDark);
    });
}

void MainWindow::buildContent()
{
    m_stack = new QStackedWidget(this);

    m_dashboard  = new DashboardWidget(m_stack);
    m_crm        = new CrmWidget(m_stack);
    m_inventory  = new InventoryWidget(m_stack);
    m_hr         = new HrWidget(m_stack);
    m_finance    = new FinanceWidget(m_stack);
    m_settings   = new SettingsWidget(m_stack);

    m_stack->addWidget(m_dashboard);   // 0
    m_stack->addWidget(m_crm);         // 1
    m_stack->addWidget(m_inventory);   // 2
    m_stack->addWidget(m_hr);          // 3
    m_stack->addWidget(m_finance);     // 4
    m_stack->addWidget(m_settings);    // 5

    connect(m_settings, &SettingsWidget::themeChanged, this, &MainWindow::applyTheme);
}

void MainWindow::switchModule(int index)
{
    if (index < 0 || index >= m_navBtns.size()) return;

    m_currentModule = index;
    m_stack->setCurrentIndex(index);
    updateNavButtons(index);

    const QStringList titles = {
        tr("Dashboard"), tr("Customers"), tr("Inventory"),
        tr("Human Resources"), tr("Finance"), tr("Settings")
    };
    m_pageTitleLabel->setText(titles.value(index, "EDV System"));
}

void MainWindow::updateNavButtons(int active)
{
    for (int i = 0; i < m_navBtns.size(); ++i)
        m_navBtns[i]->setChecked(i == active);
}

void MainWindow::applyTheme(bool dark)
{
    loadTheme(dark);
}

void MainWindow::loadTheme(bool dark)
{
    QString path = dark ? ":/styles/resources/styles/dark.qss"
                        : ":/styles/resources/styles/light.qss";
    QFile f(path);
    if (f.open(QFile::ReadOnly)) {
        qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
        f.close();
    }
}

void MainWindow::onLogout()
{
    auto btn = QMessageBox::question(this, tr("Logout"),
        tr("Are you sure you want to logout?"),
        QMessageBox::Yes | QMessageBox::Cancel);
    if (btn == QMessageBox::Yes) {
        close();
        // main.cpp will handle re-showing login when this window closes
        qApp->quit();
    }
}
