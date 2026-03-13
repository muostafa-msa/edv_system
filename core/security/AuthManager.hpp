#pragma once

#include "Session.hpp"
#include <QString>
#include <optional>
#include <functional>

namespace edv::core::security {

/**
 * @brief Authentication and authorisation manager.
 *
 * Handles:
 *  - Password hashing (SHA-256)
 *  - User validation against the `users` DB table
 *  - First-run admin account creation
 *  - RBAC permission checks
 *
 * All methods are thread-safe.
 */
class AuthManager {
public:
    static AuthManager& instance() {
        static AuthManager inst;
        return inst;
    }
    AuthManager(const AuthManager&) = delete;
    AuthManager& operator=(const AuthManager&) = delete;

    /**
     * @brief Authenticate a user with username + plaintext password.
     * @return Session on success, nullopt on failure.
     */
    [[nodiscard]] std::optional<Session> authenticate(
        const QString& username,
        const QString& plainPassword
    );

    /**
     * @brief Create the initial admin user if the users table is empty.
     * @return true if admin was created (or already exists).
     */
    [[nodiscard]] bool ensureAdminExists(
        const QString& adminUser    = "admin",
        const QString& adminPass    = "Admin1234!",
        const QString& displayName  = "System Administrator"
    );

    /**
     * @brief Change a user's password.
     * @return true on success.
     */
    [[nodiscard]] bool changePassword(
        int userId,
        const QString& currentPlainPassword,
        const QString& newPlainPassword
    );

    /**
     * @brief Hash a plaintext password using SHA-256.
     * @return Lowercase hex string.
     */
    [[nodiscard]] static QString hashPassword(const QString& plain);

    /// Returns true if the users table is empty (first run).
    [[nodiscard]] bool isFirstRun();

    /// Last error message.
    [[nodiscard]] QString lastError() const { return m_lastError; }

private:
    AuthManager() = default;
    mutable QString m_lastError;
};

} // namespace edv::core::security

using AuthManager = edv::core::security::AuthManager;
