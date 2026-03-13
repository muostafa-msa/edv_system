#pragma once

#include <QString>
#include <QSqlDatabase>
#include <QMutex>
#include <QWaitCondition>
#include <QVector>
#include <QAtomicInt>
#include <memory>
#include <optional>

namespace edv::core::db {

/**
 * @brief RAII connection token returned by the pool.
 *
 * Holds a named QSqlDatabase connection. On destruction, the connection
 * is returned to the pool automatically. Never copy — only move.
 *
 * Usage:
 *   auto token = ConnectionPool::instance().acquire();
 *   if (!token) { handle_error(); return; }
 *   QSqlQuery q(token->connection());
 */
class ConnectionToken {
public:
    ~ConnectionToken();
    ConnectionToken(ConnectionToken&&) noexcept;
    ConnectionToken& operator=(ConnectionToken&&) noexcept;
    ConnectionToken(const ConnectionToken&) = delete;
    ConnectionToken& operator=(const ConnectionToken&) = delete;

    /// Returns the underlying database connection (always open).
    [[nodiscard]] QSqlDatabase connection() const;

    /// True if the token holds a valid connection.
    [[nodiscard]] bool isValid() const { return m_valid; }
    explicit operator bool() const { return m_valid; }

private:
    friend class ConnectionPool;
    explicit ConnectionToken(QString connName);

    QString m_connName;
    bool    m_valid{false};
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Thread-safe PostgreSQL connection pool.
 *
 * Manages a fixed set of QSqlDatabase connections named "edv_pool_N".
 * Callers acquire() a token; the connection is returned on token destruction.
 * Blocks for up to timeoutMs if all connections are busy.
 */
class ConnectionPool {
public:
    static ConnectionPool& instance() {
        static ConnectionPool inst;
        return inst;
    }
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    /**
     * @brief Initialise pool with the given credentials.
     * @param poolSize Number of connections to maintain.
     */
    [[nodiscard]] bool initialise(
        const QString& host, int port,
        const QString& dbName,
        const QString& user, const QString& password,
        int poolSize = 8
    );

    /// Shut down all connections (call before QApplication exit).
    void shutdown();

    /**
     * @brief Acquire a connection token (blocks up to timeoutMs).
     * @return Token on success, empty optional on timeout/error.
     */
    [[nodiscard]] std::optional<ConnectionToken> acquire(int timeoutMs = 5000);

    /// Returns the last error message from initialise().
    [[nodiscard]] QString lastError() const;

    [[nodiscard]] bool isInitialised() const;

private:
    ConnectionPool() = default;
    ~ConnectionPool();

    void returnConnection(const QString& connName);
    friend class ConnectionToken;

    mutable QMutex    m_mutex;
    QWaitCondition    m_cond;
    QVector<QString>  m_available;   // free connection names
    QVector<QString>  m_all;         // all connection names
    QString           m_lastError;
    bool              m_initialised{false};
};

} // namespace edv::core::db
