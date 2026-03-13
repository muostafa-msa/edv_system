#pragma once

#include <QString>
#include <QDateTime>
#include <QStringList>

namespace edv::core::security {

/**
 * @brief RBAC roles — 4-level hierarchy.
 *
 * Permissions are cumulative (Admin ⊃ Executive ⊃ Manager ⊃ Staff).
 */
enum class Role {
    Staff     = 1,  ///< Read-only access to own module
    Manager   = 2,  ///< Full CRUD within assigned modules
    Executive = 3,  ///< Cross-module read + financial reports
    Admin     = 4   ///< Full system access including user management
};

inline QString roleToString(Role r) {
    switch (r) {
        case Role::Admin:     return QStringLiteral("Admin");
        case Role::Executive: return QStringLiteral("Executive");
        case Role::Manager:   return QStringLiteral("Manager");
        default:              return QStringLiteral("Staff");
    }
}

inline Role roleFromString(const QString& s) {
    if (s.compare("Admin",     Qt::CaseInsensitive) == 0) return Role::Admin;
    if (s.compare("Executive", Qt::CaseInsensitive) == 0) return Role::Executive;
    if (s.compare("Manager",   Qt::CaseInsensitive) == 0) return Role::Manager;
    return Role::Staff;
}

/**
 * @brief Immutable value object representing a logged-in session.
 *
 * Created by AuthManager after successful authentication.
 * Passed through the application as a const reference.
 */
struct Session {
    int       userId;
    QString   username;
    QString   displayName;
    Role      role;
    QDateTime loginAt;

    /// Returns true if this session has at least the required role level.
    [[nodiscard]] bool hasRole(Role required) const noexcept {
        return static_cast<int>(role) >= static_cast<int>(required);
    }

    [[nodiscard]] bool isAdmin()     const noexcept { return hasRole(Role::Admin); }
    [[nodiscard]] bool isExecutive() const noexcept { return hasRole(Role::Executive); }
    [[nodiscard]] bool isManager()   const noexcept { return hasRole(Role::Manager); }

    [[nodiscard]] bool isValid() const noexcept { return userId > 0; }

    static Session invalid() { return {0, {}, {}, Role::Staff, {}}; }
};

} // namespace edv::core::security

// Convenience alias
using Session = edv::core::security::Session;
using Role    = edv::core::security::Role;
