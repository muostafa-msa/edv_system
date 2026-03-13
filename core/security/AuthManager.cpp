#include "AuthManager.hpp"
#include "core/database/DatabaseManager.hpp"

#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>

namespace edv::core::security {

QString AuthManager::hashPassword(const QString& plain)
{
    return QCryptographicHash::hash(plain.toUtf8(), QCryptographicHash::Sha256)
               .toHex()
               .toLower();
}

std::optional<Session> AuthManager::authenticate(
    const QString& username,
    const QString& plainPassword)
{
    auto token = DatabaseManager::instance().acquire();
    if (!token) {
        m_lastError = QStringLiteral("Database unavailable");
        return std::nullopt;
    }

    const QString hash = hashPassword(plainPassword);

    QSqlQuery q(token->connection());
    q.prepare(
        "SELECT id, username, display_name, role "
        "FROM users "
        "WHERE username = :u AND password_hash = :h AND is_active = true"
    );
    q.bindValue(":u", username);
    q.bindValue(":h", hash);

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return std::nullopt;
    }

    if (!q.next()) {
        m_lastError = QStringLiteral("Invalid credentials");
        return std::nullopt;
    }

    // Update last_login timestamp
    {
        QSqlQuery upd(token->connection());
        upd.prepare("UPDATE users SET last_login = NOW() WHERE id = :id");
        upd.bindValue(":id", q.value(0).toInt());
        upd.exec();
    }

    Session s;
    s.userId      = q.value(0).toInt();
    s.username    = q.value(1).toString();
    s.displayName = q.value(2).toString();
    s.role        = roleFromString(q.value(3).toString());
    s.loginAt     = QDateTime::currentDateTime();

    qDebug() << "[Auth] Authenticated:" << s.username << "as" << roleToString(s.role);
    return s;
}

bool AuthManager::isFirstRun()
{
    auto token = DatabaseManager::instance().acquire();
    if (!token) return false;

    QSqlQuery q(token->connection());
    q.exec("SELECT COUNT(*) FROM users");
    return q.next() && q.value(0).toInt() == 0;
}

bool AuthManager::ensureAdminExists(
    const QString& adminUser,
    const QString& adminPass,
    const QString& displayName)
{
    auto token = DatabaseManager::instance().acquire();
    if (!token) {
        m_lastError = "Database unavailable";
        return false;
    }

    QSqlQuery check(token->connection());
    check.exec("SELECT COUNT(*) FROM users");
    if (check.next() && check.value(0).toInt() > 0)
        return true; // already exists

    const QString hash = hashPassword(adminPass);

    QSqlQuery ins(token->connection());
    ins.prepare(
        "INSERT INTO users (username, display_name, password_hash, role, is_active) "
        "VALUES (:u, :d, :h, 'Admin', true)"
    );
    ins.bindValue(":u", adminUser);
    ins.bindValue(":d", displayName);
    ins.bindValue(":h", hash);

    if (!ins.exec()) {
        m_lastError = ins.lastError().text();
        return false;
    }

    qDebug() << "[Auth] Admin user created:" << adminUser;
    return true;
}

bool AuthManager::changePassword(
    int userId,
    const QString& currentPlain,
    const QString& newPlain)
{
    auto token = DatabaseManager::instance().acquire();
    if (!token) {
        m_lastError = "Database unavailable";
        return false;
    }

    // Verify current password
    QSqlQuery verify(token->connection());
    verify.prepare("SELECT id FROM users WHERE id = :id AND password_hash = :h");
    verify.bindValue(":id", userId);
    verify.bindValue(":h", hashPassword(currentPlain));

    if (!verify.exec() || !verify.next()) {
        m_lastError = "Current password is incorrect";
        return false;
    }

    QSqlQuery upd(token->connection());
    upd.prepare("UPDATE users SET password_hash = :h WHERE id = :id");
    upd.bindValue(":h", hashPassword(newPlain));
    upd.bindValue(":id", userId);

    if (!upd.exec()) {
        m_lastError = upd.lastError().text();
        return false;
    }

    return true;
}

} // namespace edv::core::security
