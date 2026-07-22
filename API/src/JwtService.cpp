#include "JwtService.hpp"

#include <jwt-cpp/jwt.h>

namespace trading {

JwtService::JwtService(std::string secret) : m_secret(std::move(secret)) {}

std::string JwtService::sign(int userId) const {
    return jwt::create()
        .set_type("JWT")
        .set_payload_claim("uid", jwt::claim(std::to_string(userId)))
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{24})
        .sign(jwt::algorithm::hs256{m_secret});
}

std::optional<int> JwtService::verify(const std::string& token) const {
    try {
        const auto decoded = jwt::decode(token);
        jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{m_secret})
            .with_type("JWT")
            .verify(decoded);
        return std::stoi(decoded.get_payload_claim("uid").as_string());
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace trading
