#include "ConnectionPool.hpp"

#include <QSqlError>
#include <QMutexLocker>
#include <QDebug>

namespace edv::core::db {

// ── ConnectionToken ───────────────────────────────────────────────────────────

ConnectionToken::ConnectionToken(QString connName)
    : m_connName(std::move(connName)), m_valid(true)
{}

ConnectionToken::~ConnectionToken()
{
    if (m_valid)
        ConnectionPool::instance().returnConnection(m_connName);
}

ConnectionToken::ConnectionToken(ConnectionToken&& o) noexcept
    : m_connName(std::move(o.m_connName)), m_valid(o.m_valid)
{
    o.m_valid = false;
}

ConnectionToken& ConnectionToken::operator=(ConnectionToken&& o) noexcept
{
    if (this != &o) {
        if (m_valid) ConnectionPool::instance().returnConnection(m_connName);
        m_connName = std::move(o.m_connName);
        m_valid    = o.m_valid;
        o.m_valid  = false;
    }
    return *this;
}

QSqlDatabase ConnectionToken::connection() const
{
    return QSqlDatabase::database(m_connName, /*open=*/false);
}

// ── ConnectionPool ────────────────────────────────────────────────────────────

ConnectionPool::~ConnectionPool()
{
    shutdown();
}

bool ConnectionPool::initialise(
    const QString& host, int port,
    const QString& dbName,
    const QString& user, const QString& password,
    int poolSize)
{
    QMutexLocker lk(&m_mutex);

    if (m_initialised) return true;

    for (int i = 0; i < poolSize; ++i) {
        QString connName = QString("edv_pool_%1").arg(i);
        QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", connName);
        db.setHostName(host);
        db.setPort(port);
        db.setDatabaseName(dbName);
        db.setUserName(user);
        db.setPassword(password);
        db.setConnectOptions("connect_timeout=5;application_name=EDVEnterpriseSystem");

        if (!db.open()) {
            m_lastError = db.lastError().text();
            qWarning() << "[Pool] Failed to open connection" << connName << ":" << m_lastError;
            // Clean up all opened so far
            lk.unlock();
            shutdown();
            lk.relock();
            return false;
        }

        m_all.append(connName);
        m_available.append(connName);
        qDebug() << "[Pool] Connection" << connName << "opened";
    }

    m_initialised = true;
    return true;
}

void ConnectionPool::shutdown()
{
    QMutexLocker lk(&m_mutex);
    m_initialised = false;
    m_available.clear();
    for (const QString& name : std::as_const(m_all)) {
        QSqlDatabase::database(name, false).close();
        QSqlDatabase::removeDatabase(name);
    }
    m_all.clear();
    m_cond.wakeAll();
}

std::optional<ConnectionToken> ConnectionPool::acquire(int timeoutMs)
{
    QMutexLocker lk(&m_mutex);

    if (!m_initialised) {
        m_lastError = "Pool not initialised";
        return std::nullopt;
    }

    if (m_available.isEmpty()) {
        if (!m_cond.wait(&m_mutex, static_cast<unsigned long>(timeoutMs))) {
            m_lastError = "Connection pool timeout — all connections busy";
            return std::nullopt;
        }
    }

    if (m_available.isEmpty()) {
        m_lastError = "No connection available after wait";
        return std::nullopt;
    }

    QString name = m_available.takeLast();
    lk.unlock();

    // Ensure the connection is still alive (reconnect if needed)
    QSqlDatabase db = QSqlDatabase::database(name, false);
    if (!db.isOpen() && !db.open()) {
        lk.relock();
        m_available.append(name); // return it
        m_lastError = db.lastError().text();
        return std::nullopt;
    }

    return ConnectionToken(name);
}

void ConnectionPool::returnConnection(const QString& connName)
{
    QMutexLocker lk(&m_mutex);
    m_available.append(connName);
    m_cond.wakeOne();
}

QString ConnectionPool::lastError() const
{
    QMutexLocker lk(&m_mutex);
    return m_lastError;
}

bool ConnectionPool::isInitialised() const
{
    QMutexLocker lk(&m_mutex);
    return m_initialised;
}

} // namespace edv::core::db
