#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QSettings>
#include <QString>

// Singleton wrapper around QSettings for application-wide preferences.
class AppSettings
{
public:
    static AppSettings &instance()
    {
        static AppSettings s;
        return s;
    }

    // ---- Database connection ----
    QString dbHost() const { return m_s.value("db/host", "localhost").toString(); }
    void    setDbHost(const QString &v) { m_s.setValue("db/host", v); }

    int  dbPort() const { return m_s.value("db/port", 5432).toInt(); }
    void setDbPort(int v) { m_s.setValue("db/port", v); }

    QString dbName() const { return m_s.value("db/name", "edv_system").toString(); }
    void    setDbName(const QString &v) { m_s.setValue("db/name", v); }

    QString dbUser() const { return m_s.value("db/user", "postgres").toString(); }
    void    setDbUser(const QString &v) { m_s.setValue("db/user", v); }

    QString dbPassword() const { return m_s.value("db/password", "").toString(); }
    void    setDbPassword(const QString &v) { m_s.setValue("db/password", v); }

    bool dbConfigured() const { return m_s.value("db/configured", false).toBool(); }
    void setDbConfigured(bool v) { m_s.setValue("db/configured", v); }

    // ---- UI preferences ----
    bool darkMode() const { return m_s.value("ui/darkMode", true).toBool(); }
    void setDarkMode(bool v) { m_s.setValue("ui/darkMode", v); }

    // Language: "en", "de", "ar"
    QString language() const { return m_s.value("ui/language", "en").toString(); }
    void    setLanguage(const QString &v) { m_s.setValue("ui/language", v); }

    // ---- User session (runtime only, not persisted) ----
    QString currentUsername;
    int     currentUserId = -1;
    QString currentUserRole;

private:
    AppSettings() : m_s("EDV-System", "EDV-ERP") {}
    QSettings m_s;
};

#endif // APPSETTINGS_H
