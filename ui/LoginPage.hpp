#pragma once

#include "core/security/Session.hpp"
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class LoginPage; }
QT_END_NAMESPACE

/**
 * @brief Full-window login widget — embedded in MainWindow stack (page 0).
 *
 * Emits loginSuccessful(Session) after authentication.
 * Emits dbSettingsRequested() when user clicks DB Settings.
 */
class LoginPage : public QWidget {
    Q_OBJECT
public:
    explicit LoginPage(QWidget *parent = nullptr);
    ~LoginPage() override;

    void setError(const QString& msg);
    void clearError();
    void resetForm();

signals:
    void loginSuccessful(const Session& session);
    void dbSettingsRequested();

private slots:
    void onLoginClicked();
    void onLanguageChanged(int index);
    void onDbSettings();

private:
    Ui::LoginPage *ui;
};
