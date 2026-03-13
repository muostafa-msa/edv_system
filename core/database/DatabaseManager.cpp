#include "DatabaseManager.hpp"
#include "core/config/AppConfig.hpp"

namespace edv::core::db {

bool DatabaseManager::connectFromConfig()
{
    const auto& cfg = AppConfig::instance();
    return connect(
        cfg.dbHost(), cfg.dbPort(),
        cfg.dbName(), cfg.dbUser(), cfg.dbPassword(),
        cfg.dbPoolSize()
    );
}

bool DatabaseManager::connect(
    const QString& host, int port,
    const QString& dbName,
    const QString& user, const QString& password,
    int poolSize)
{
    return ConnectionPool::instance().initialise(host, port, dbName, user, password, poolSize);
}

} // namespace edv::core::db
