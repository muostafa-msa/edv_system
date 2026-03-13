#pragma once

#include <QObject>
#include <QString>
#include <QFont>

namespace edv::core::ui {

enum class ThemeType {
    Dark,
    Light
};

/**
 * @brief Singleton ThemeManager — hot-swaps the global QSS stylesheet.
 *
 * Usage:
 *   ThemeManager::instance().applyTheme(ThemeType::Dark);
 *   ThemeManager::instance().toggle();
 *
 * The manager:
 *  - Loads QSS from the Qt resource system (:/themes/...)
 *  - Applies via qApp->setStyleSheet()
 *  - Persists the choice in AppConfig
 *  - Emits themeChanged() so any widget can react
 */
class ThemeManager : public QObject {
    Q_OBJECT
public:
    static ThemeManager& instance() {
        static ThemeManager inst;
        return inst;
    }
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    /**
     * @brief Apply a theme globally to qApp.
     * @param type  ThemeType::Dark or ThemeType::Light
     */
    void applyTheme(ThemeType type);

    /** @brief Toggle between Dark and Light. */
    void toggle();

    /** @brief Apply the theme saved in AppConfig (call at startup). */
    void applyFromConfig();

    /** @brief Current active theme. */
    [[nodiscard]] ThemeType currentTheme() const noexcept { return m_current; }
    [[nodiscard]] bool isDark()  const noexcept { return m_current == ThemeType::Dark; }
    [[nodiscard]] bool isLight() const noexcept { return m_current == ThemeType::Light; }

signals:
    /** Emitted after the stylesheet has been applied. */
    void themeChanged(ThemeType type);

private:
    explicit ThemeManager(QObject* parent = nullptr);
    [[nodiscard]] static QString loadQss(const QString& resourcePath);
    static void applyGlobalFont();

    ThemeType m_current{ThemeType::Dark};
};

} // namespace edv::core::ui

// Convenience alias
using ThemeManager = edv::core::ui::ThemeManager;
using ThemeType    = edv::core::ui::ThemeType;
