#pragma once

#include <QString>
#include <QSettings>
#include <QMutex>
#include <QMutexLocker>

namespace edv::core {

/**
 * @brief Thread-safe application configuration singleton.
 *
 * Wraps QSettings with typed accessors. All DB credentials are stored
 * under the "database/" group. UI preferences under "ui/".
 */
class AppConfig {
public:
    static AppConfig& instance() {
        static AppConfig inst;
        return inst;
    }

    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;

    // ── Database ──────────────────────────────────────────────────────────────
    QString dbHost()     const { QMutexLocker lk(&m_mutex); return m_s.value("database/host",     "localhost").toString(); }
    int     dbPort()     const { QMutexLocker lk(&m_mutex); return m_s.value("database/port",     5432).toInt(); }
    QString dbName()     const { QMutexLocker lk(&m_mutex); return m_s.value("database/name",     "edv_erp").toString(); }
    QString dbUser()     const { QMutexLocker lk(&m_mutex); return m_s.value("database/user",     "postgres").toString(); }
    QString dbPassword() const { QMutexLocker lk(&m_mutex); return m_s.value("database/password", "").toString(); }
    bool    dbConfigured() const { QMutexLocker lk(&m_mutex); return m_s.value("database/configured", false).toBool(); }
    int     dbPoolSize() const { QMutexLocker lk(&m_mutex); return m_s.value("database/pool_size", 8).toInt(); }

    void setDbHost(const QString& v)     { QMutexLocker lk(&m_mutex); m_s.setValue("database/host",     v); }
    void setDbPort(int v)                { QMutexLocker lk(&m_mutex); m_s.setValue("database/port",     v); }
    void setDbName(const QString& v)     { QMutexLocker lk(&m_mutex); m_s.setValue("database/name",     v); }
    void setDbUser(const QString& v)     { QMutexLocker lk(&m_mutex); m_s.setValue("database/user",     v); }
    void setDbPassword(const QString& v) { QMutexLocker lk(&m_mutex); m_s.setValue("database/password", v); }
    void setDbConfigured(bool v)         { QMutexLocker lk(&m_mutex); m_s.setValue("database/configured", v); }
    void setDbPoolSize(int v)            { QMutexLocker lk(&m_mutex); m_s.setValue("database/pool_size", v); }

    // ── UI preferences ────────────────────────────────────────────────────────
    bool    darkMode()  const { QMutexLocker lk(&m_mutex); return m_s.value("ui/dark_mode", false).toBool(); }
    QString language()  const { QMutexLocker lk(&m_mutex); return m_s.value("ui/language",  "en").toString(); }

    void setDarkMode(bool v)         { QMutexLocker lk(&m_mutex); m_s.setValue("ui/dark_mode", v); }
    void setLanguage(const QString& v) { QMutexLocker lk(&m_mutex); m_s.setValue("ui/language", v); }

    // ── Session ───────────────────────────────────────────────────────────────
    void   setLastUser(const QString& u) { QMutexLocker lk(&m_mutex); m_s.setValue("session/last_user", u); }
    QString lastUser() const             { QMutexLocker lk(&m_mutex); return m_s.value("session/last_user").toString(); }

private:
    explicit AppConfig()
        : m_s(QSettings::IniFormat, QSettings::UserScope, "EDV-System", "EDV-ERP")
    {}

    mutable QMutex m_mutex;
    QSettings      m_s;
};

} // namespace edv::core

// Convenience alias
using AppConfig = edv::core::AppConfig;
