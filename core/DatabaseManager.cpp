#include "DatabaseManager.h"
#include "AppSettings.h"

#include <QSqlError>
#include <QSqlQuery>

DatabaseManager &DatabaseManager::instance()
{
    static DatabaseManager inst;
    return inst;
}

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent) {}

bool DatabaseManager::connect(const QString &host, int port, const QString &dbName,
                               const QString &user, const QString &password)
{
    // Remove existing connection if any
    if (QSqlDatabase::contains(CONNECTION_NAME))
        QSqlDatabase::removeDatabase(CONNECTION_NAME);

    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", CONNECTION_NAME);
    db.setHostName(host);
    db.setPort(port);
    db.setDatabaseName(dbName);
    db.setUserName(user);
    db.setPassword(password);
    db.setConnectOptions("connect_timeout=5");

    if (db.open()) {
        m_connected = true;
        m_lastError.clear();
        emit connectionStatusChanged(true);
        return true;
    }

    m_lastError = db.lastError().text();
    m_connected = false;
    emit connectionStatusChanged(false);
    return false;
}

bool DatabaseManager::connectFromSettings()
{
    auto &s = AppSettings::instance();
    return connect(s.dbHost(), s.dbPort(), s.dbName(), s.dbUser(), s.dbPassword());
}

bool DatabaseManager::isConnected() const
{
    if (!m_connected)
        return false;
    QSqlDatabase db = QSqlDatabase::database(CONNECTION_NAME, false);
    return db.isValid() && db.isOpen();
}

void DatabaseManager::disconnect()
{
    QSqlDatabase::database(CONNECTION_NAME).close();
    m_connected = false;
    emit connectionStatusChanged(false);
}
