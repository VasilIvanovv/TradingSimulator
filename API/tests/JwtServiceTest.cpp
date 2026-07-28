#include <gtest/gtest.h>
#include "JwtService.hpp"

using namespace trading;

class JwtServiceTest : public ::testing::Test {
protected:
    JwtService jwt{"test-secret"};
};

TEST_F(JwtServiceTest, SignVerify_RoundTrip_ReturnsCorrectUserId) {
    const auto token = jwt.sign(42);
    const auto result = jwt.verify(token);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

TEST_F(JwtServiceTest, SignVerify_DifferentUserIds_RoundTripCorrectly) {
    EXPECT_EQ(*jwt.verify(jwt.sign(1)),  1);
    EXPECT_EQ(*jwt.verify(jwt.sign(99)), 99);
}

TEST_F(JwtServiceTest, Sign_DifferentUserIds_ProduceDifferentTokens) {
    EXPECT_NE(jwt.sign(1), jwt.sign(2));
}

TEST_F(JwtServiceTest, Verify_TamperedSignature_ReturnsNullopt) {
    auto token = jwt.sign(42);
    token.back() ^= 0x01; // flip one bit in the last character
    EXPECT_FALSE(jwt.verify(token).has_value());
}

TEST_F(JwtServiceTest, Verify_DifferentSecret_ReturnsNullopt) {
    JwtService other{"different-secret"};
    EXPECT_FALSE(other.verify(jwt.sign(42)).has_value());
}

TEST_F(JwtServiceTest, Verify_EmptyString_ReturnsNullopt) {
    EXPECT_FALSE(jwt.verify("").has_value());
}

TEST_F(JwtServiceTest, Verify_GarbageString_ReturnsNullopt) {
    EXPECT_FALSE(jwt.verify("not.a.jwt").has_value());
    EXPECT_FALSE(jwt.verify("garbage").has_value());
}
