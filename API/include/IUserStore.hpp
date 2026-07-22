#pragma once

#include <optional>
#include <string>

namespace trading {

struct UserRecord {
    int         userId;
    std::string passwordHash;
};

/**
 * @brief Abstract user repository.
 *
 * Decouples AuthHandler from any specific storage backend.
 * Implement this interface to swap SQLite for another database without
 * touching the auth layer.
 */
class IUserStore {
public:
    virtual ~IUserStore() = default;

    /**
     * @brief Insert a new user with a pre-hashed password.
     * @return The new user's id, or -1 if the username is already taken.
     */
    virtual int createUser(const std::string& username,
                           const std::string& passwordHash) = 0;

    /**
     * @brief Look up a user by username.
     * @return The record (id + stored hash), or @c std::nullopt if not found.
     */
    virtual std::optional<UserRecord> findByUsername(const std::string& username) = 0;
};

} // namespace trading
