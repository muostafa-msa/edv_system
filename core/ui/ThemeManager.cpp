#include "ThemeManager.hpp"
#include "core/config/AppConfig.hpp"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QDebug>

namespace edv::core::ui {

// ── Resource paths ────────────────────────────────────────────────────────────
static constexpr const char* DARK_QSS  = ":/themes/dark_theme.qss";
static constexpr const char* LIGHT_QSS = ":/themes/light_theme.qss";

// ── Constructor ───────────────────────────────────────────────────────────────

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    applyGlobalFont();
}

// ── Public API ────────────────────────────────────────────────────────────────

void ThemeManager::applyTheme(ThemeType type)
{
    const QString path = (type == ThemeType::Dark) ? DARK_QSS : LIGHT_QSS;
    const QString qss  = loadQss(path);

    if (qss.isEmpty()) {
        qWarning() << "[ThemeManager] Failed to load QSS:" << path;
        return;
    }

    qApp->setStyleSheet(qss);
    m_current = type;

    // Persist choice
    AppConfig::instance().setDarkMode(type == ThemeType::Dark);

    qDebug() << "[ThemeManager] Applied theme:"
             << (type == ThemeType::Dark ? "Dark" : "Light");

    emit themeChanged(type);
}

void ThemeManager::toggle()
{
    applyTheme(isDark() ? ThemeType::Light : ThemeType::Dark);
}

void ThemeManager::applyFromConfig()
{
    const bool dark = AppConfig::instance().darkMode();
    applyTheme(dark ? ThemeType::Dark : ThemeType::Light);
}

// ── Private helpers ───────────────────────────────────────────────────────────

QString ThemeManager::loadQss(const QString& resourcePath)
{
    QFile f(resourcePath);
    if (!f.open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << "[ThemeManager] Cannot open:" << resourcePath;
        return {};
    }
    const QString content = QString::fromUtf8(f.readAll());
    f.close();
    return content;
}

void ThemeManager::applyGlobalFont()
{
    // Prefer 'Inter' (if installed), fall back to 'Segoe UI' (Windows system font)
    QFont font;
    const QStringList preferred = {"Inter", "Segoe UI", "SF Pro Display", "Helvetica Neue", "Arial"};
    for (const QString& name : preferred) {
        font = QFont(name);
        if (font.exactMatch()) break;
    }
    font.setPointSize(10);
    font.setStyleHint(QFont::SansSerif);
    font.setHintingPreference(QFont::PreferFullHinting);
    qApp->setFont(font);
}

} // namespace edv::core::ui
