#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QString>

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static DatabaseManager &instance();

    bool connect(const QString &host, int port, const QString &dbName,
                 const QString &user, const QString &password);
    bool connectFromSettings();

    bool isConnected() const;
    void disconnect();

    // Returns the named connection for use with QSqlQuery
    QSqlDatabase database() const { return QSqlDatabase::database(CONNECTION_NAME); }

    QString lastError() const { return m_lastError; }

signals:
    void connectionStatusChanged(bool connected);

private:
    explicit DatabaseManager(QObject *parent = nullptr);

    static constexpr const char *CONNECTION_NAME = "edv_main";
    QString m_lastError;
    bool    m_connected = false;
};

#endif // DATABASEMANAGER_H
