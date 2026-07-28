#include "AuthHandler.hpp"
#include "IUserStore.hpp"
#include "JwtService.hpp"

#include <sodium.h>
#include <stdexcept>

namespace trading {

HashingParams productionHashParams() {
    return {crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE};
}

AuthHandler::AuthHandler(IUserStore &store, JwtService &jwt,
                         HashingParams params)
    : m_store(store), m_jwt(jwt), m_params(params) {
    if (sodium_init() < 0)
        throw std::runtime_error("AuthHandler: failed to initialize libsodium");
}

std::expected<AuthResponse, std::string>
AuthHandler::registerUser(const RegisterRequest &req) {
    if (req.username.empty() || req.password.empty())
        return std::unexpected("Username and password must not be empty");

    char hash[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str(hash, req.password.c_str(), req.password.size(),
                          m_params.opsLimit, m_params.memLimit) != 0)
        throw std::runtime_error("Password hashing failed (out of memory)");

    const int userId = m_store.createUser(req.username, hash);
    if (userId == -1)
        return std::unexpected("Username '" + req.username + "' is already taken");

    return AuthResponse{m_jwt.sign(userId)};
}

std::expected<AuthResponse, std::string>
AuthHandler::loginUser(const LoginRequest &req) {
    const auto record = m_store.findByUsername(req.username);
    if (!record)
        return std::unexpected("Invalid username or password");

    if (crypto_pwhash_str_verify(record->passwordHash.c_str(),
                                 req.password.c_str(),
                                 req.password.size()) != 0)
        return std::unexpected("Invalid username or password");

    return AuthResponse{m_jwt.sign(record->userId)};
}

} // namespace trading
