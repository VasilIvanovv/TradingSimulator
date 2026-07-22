#pragma once

#include <optional>
#include <string>

namespace trading {

/**
 * @brief Signs and verifies JWT tokens using HS256 (HMAC-SHA256).
 *
 * Tokens carry a single claim — the user's integer id — and expire after 24 h.
 * The secret key must stay consistent across restarts; changing it invalidates
 * all existing tokens.
 */
class JwtService {
public:
    explicit JwtService(std::string secret);

    /**
     * @brief Create a signed JWT encoding @p userId.
     * @return Compact JWT string: header.payload.signature
     */
    std::string sign(int userId) const;

    /**
     * @brief Verify a JWT and extract the user id.
     * @return The user id on success, @c std::nullopt if the token is invalid,
     *         expired, or has been tampered with.
     */
    std::optional<int> verify(const std::string& token) const;

private:
    std::string m_secret;
};

} // namespace trading
