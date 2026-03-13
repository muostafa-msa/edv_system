#pragma once

#include "ConnectionPool.hpp"
#include <QString>
#include <optional>

namespace edv::core::db {

/**
 * @brief Thin singleton facade over ConnectionPool.
 *
 * Provides the public API used throughout the application:
 *   - connectFromConfig()   — initialises the pool from AppConfig
 *   - acquire()             — obtain a RAII connection token
 *   - isReady()             — true if pool is initialised
 *
 * Prefer using acquire() for all database access rather than named
 * QSqlDatabase connections to ensure thread-safety.
 */
class DatabaseManager {
public:
    static DatabaseManager& instance() {
        static DatabaseManager inst;
        return inst;
    }
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    /**
     * @brief Initialise the connection pool using credentials from AppConfig.
     * @return true on success.
     */
    [[nodiscard]] bool connectFromConfig();

    /**
     * @brief Initialise the connection pool with explicit credentials.
     * @return true on success.
     */
    [[nodiscard]] bool connect(
        const QString& host, int port,
        const QString& dbName,
        const QString& user, const QString& password,
        int poolSize = 8
    );

    /// Acquire a RAII connection token (blocks up to timeoutMs if pool is busy).
    [[nodiscard]] std::optional<ConnectionToken> acquire(int timeoutMs = 5000) {
        return ConnectionPool::instance().acquire(timeoutMs);
    }

    /// Returns true if the pool is initialised and has connections.
    [[nodiscard]] bool isReady() const {
        return ConnectionPool::instance().isInitialised();
    }

    /// Shutdown all connections (call before application exit).
    void shutdown() {
        ConnectionPool::instance().shutdown();
    }

    /// Last error from connect().
    [[nodiscard]] QString lastError() const {
        return ConnectionPool::instance().lastError();
    }

private:
    DatabaseManager() = default;
};

} // namespace edv::core::db

// Convenience alias for application code
using DatabaseManager = edv::core::db::DatabaseManager;
