#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>

class QStackedWidget;
class QPushButton;
class QLabel;
class QFrame;

class DashboardWidget;
class CrmWidget;
class InventoryWidget;
class HrWidget;
class FinanceWidget;
class SettingsWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(const QString &username, int userId,
                        const QString &role, QWidget *parent = nullptr);
    ~MainWindow() override = default;

public slots:
    void switchModule(int index);
    void applyTheme(bool dark);

private slots:
    void onLogout();

private:
    void buildUi();
    void buildSidebar(QWidget *sidebar);
    void buildHeader(QWidget *header);
    void buildContent();
    void updateNavButtons(int active);
    void loadTheme(bool dark);

    // Layout widgets
    QFrame          *m_sidebar;
    QWidget         *m_header;
    QStackedWidget  *m_stack;
    QLabel          *m_pageTitleLabel;
    QLabel          *m_userLabel;
    QLabel          *m_statusDbLabel;

    // Nav buttons (one per module)
    QVector<QPushButton*> m_navBtns;

    // Module widgets
    DashboardWidget *m_dashboard;
    CrmWidget       *m_crm;
    InventoryWidget *m_inventory;
    HrWidget        *m_hr;
    FinanceWidget   *m_finance;
    SettingsWidget  *m_settings;

    int     m_currentModule = 0;
    QString m_username;
    int     m_userId;
    QString m_userRole;
};

#endif // MAINWINDOW_H
