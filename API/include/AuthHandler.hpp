#pragma once

#include "ApiTypes.hpp"

namespace trading {

class IUserStore;
class JwtService;

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
    AuthHandler(IUserStore& store, JwtService& jwt);

    /**
     * @brief Create a new account and return a login token.
     * @throws std::invalid_argument if username/password is empty or username is taken.
     * @throws std::runtime_error if password hashing fails (out of memory).
     */
    AuthResponse registerUser(const RegisterRequest& req);

    /**
     * @brief Verify credentials and return a login token.
     * @throws std::invalid_argument if credentials are invalid.
     */
    AuthResponse loginUser(const LoginRequest& req);

private:
    IUserStore& m_store;
    JwtService& m_jwt;
};

} // namespace trading
