#pragma once

#include "core/security/Session.hpp"
#include <QDialog>
#include <optional>

QT_BEGIN_NAMESPACE
namespace Ui { class LoginDialog; }
QT_END_NAMESPACE

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog() override;

    /// Returns the authenticated session after accept().
    [[nodiscard]] Session session() const { return m_session; }

private slots:
    void onLogin();
    void onLanguageChanged(int index);

private:
    void applyLanguage(const QString& lang);

    Ui::LoginDialog *ui;
    Session          m_session = Session::invalid();
};
