#pragma once

#include "core/security/Session.hpp"
#include "core/ui/ThemeManager.hpp"
#include <QMainWindow>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QPushButton;
class QLabel;
class LoginPage;

// Module indices inside moduleStack (offset by 1 because index 0 = login)
enum class ModuleIndex : int {
    Login     = 0,
    Dashboard = 1,
    Sd        = 2,
    Mm        = 3,
    Pp        = 4,
    Hr        = 5,
    Fico      = 6,
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

public slots:
    void switchModule(int index);
    void applyTheme(bool dark);

private slots:
    void onLoginSuccessful(const Session& session);
    void onDbSettingsRequested();
    void onLogout();
    void onThemeToggle();
    void updateDbStatus();

private:
    void showLoginPage();
    void showMainContent(const Session& session);
    void buildSidebarButtons();
    void buildModuleStack();
    void loadTheme(bool dark);
    void setSidebarVisible(bool visible);

    Ui::MainWindow        *ui;
    Session                m_session = Session::invalid();
    QVector<QPushButton*>  m_navBtns;
    QLabel                *m_dbStatusLabel{nullptr};
    LoginPage             *m_loginPage{nullptr};
    int                    m_currentModule{1};
};
