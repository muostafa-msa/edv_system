#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>
#include <qlabel.h>

class QLineEdit;
class QSpinBox;
class QComboBox;
class QCheckBox;

class SettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsWidget(QWidget *parent = nullptr);

signals:
    void themeChanged(bool dark);
    void languageChanged(const QString &lang);

private slots:
    void onSaveDbSettings();
    void onTestConnection();
    void onApplyAppearance();

private:
    void setupUi();
    void loadCurrentSettings();

    // DB settings
    QLineEdit *m_hostEdit;
    QSpinBox  *m_portSpin;
    QLineEdit *m_dbNameEdit;
    QLineEdit *m_userEdit;
    QLineEdit *m_passwordEdit;
    QLabel    *m_dbStatusLabel;

    // Appearance
    QComboBox *m_langCombo;
    QComboBox *m_themeCombo;

    // About
    QLabel *m_dbVersionLabel;
};

#endif // SETTINGSWIDGET_H
