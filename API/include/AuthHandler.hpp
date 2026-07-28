#pragma once

#include "ApiTypes.hpp"
#include <expected>
#include <string>

namespace trading {

class IUserStore;
class JwtService;

/// Argon2id cost parameters. Use productionHashParams() for normal use,
/// or construct with lower values in tests to keep hashing fast.
struct HashingParams {
    unsigned long long opsLimit;
    size_t             memLimit;
};

/// Returns interactive-strength Argon2id parameters (defined in .cpp using
/// libsodium constants so this header stays free of sodium.h).
HashingParams productionHashParams();

/**
 * @brief Handles user registration and login.
 *
 * Passwords are hashed with Argon2id via libsodium before being stored.
 * On login the stored hash is verified; on success a signed JWT is returned.
 * The JWT can then be sent with every subsequent request as:
 *   @c Authorization: Bearer <token>
 */
class AuthHandler {
public:
    AuthHandler(IUserStore& store, JwtService& jwt,
                HashingParams params = productionHashParams());

    /// Create a new account and return a login token.
    /// Returns an error string for expected failures (empty fields, duplicate username).
    /// Throws std::runtime_error only for system failures (Argon2id OOM).
    std::expected<AuthResponse, std::string> registerUser(const RegisterRequest& req);

    /// Verify credentials and return a login token.
    /// Returns an error string for invalid credentials — never reveals which field was wrong.
    std::expected<AuthResponse, std::string> loginUser(const LoginRequest& req);

private:
    IUserStore&   m_store;
    JwtService&   m_jwt;
    HashingParams m_params;
};

} // namespace trading
