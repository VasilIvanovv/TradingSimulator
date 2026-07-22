#include "AuthHandler.hpp"
#include "IUserStore.hpp"
#include "JwtService.hpp"

#include <sodium.h>
#include <stdexcept>

namespace trading {

AuthHandler::AuthHandler(IUserStore& store, JwtService& jwt)
    : m_store(store), m_jwt(jwt) {
    if (sodium_init() < 0)
        throw std::runtime_error("AuthHandler: failed to initialize libsodium");
}

AuthResponse AuthHandler::registerUser(const RegisterRequest& req) {
    if (req.username.empty() || req.password.empty())
        throw std::invalid_argument("Username and password must not be empty");

    char hash[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str(hash,
                          req.password.c_str(),
                          req.password.size(),
                          crypto_pwhash_OPSLIMIT_INTERACTIVE,
                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
        throw std::runtime_error("Password hashing failed (out of memory)");

    const int userId = m_store.createUser(req.username, hash);
    if (userId == -1)
        throw std::invalid_argument("Username '" + req.username + "' is already taken");

    return {m_jwt.sign(userId)};
}

AuthResponse AuthHandler::loginUser(const LoginRequest& req) {
    const auto record = m_store.findByUsername(req.username);
    if (!record)
        throw std::invalid_argument("Invalid username or password");

    if (crypto_pwhash_str_verify(record->passwordHash.c_str(),
                                  req.password.c_str(),
                                  req.password.size()) != 0)
        throw std::invalid_argument("Invalid username or password");

    return {m_jwt.sign(record->userId)};
}

} // namespace trading
