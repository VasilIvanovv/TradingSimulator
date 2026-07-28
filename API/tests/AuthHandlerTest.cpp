#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "AuthHandler.hpp"
#include "IUserStore.hpp"
#include "JwtService.hpp"

#include <sodium.h>

using namespace trading;
using ::testing::_;
using ::testing::Return;

static HashingParams minParams() {
    return {crypto_pwhash_OPSLIMIT_MIN, crypto_pwhash_MEMLIMIT_MIN};
}

static std::string makeHash(const std::string& password) {
    sodium_init();
    char buf[crypto_pwhash_STRBYTES];
    crypto_pwhash_str(buf, password.c_str(), password.size(),
                      crypto_pwhash_OPSLIMIT_MIN, crypto_pwhash_MEMLIMIT_MIN);
    return buf;
}

// ---------------------------------------------------------------------------
// Mock
// ---------------------------------------------------------------------------
class MockUserStore : public IUserStore {
public:
    MOCK_METHOD(int, createUser,
                (const std::string&, const std::string&), (override));
    MOCK_METHOD(std::optional<UserRecord>, findByUsername,
                (const std::string&), (override));
};

// ---------------------------------------------------------------------------
// registerUser tests
// ---------------------------------------------------------------------------
class AuthHandlerRegisterTest : public ::testing::Test {
protected:
    MockUserStore mockStore;
    JwtService    jwt{"test-secret"};
    AuthHandler   auth{mockStore, jwt, minParams()};
};

TEST_F(AuthHandlerRegisterTest, RegisterUser_Success_ReturnsVerifiableToken) {
    EXPECT_CALL(mockStore, createUser("alice", _)).WillOnce(Return(7));

    const auto result = auth.registerUser({"alice", "password123"});

    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->token.empty());
    const auto userId = jwt.verify(result->token);
    ASSERT_TRUE(userId.has_value());
    EXPECT_EQ(*userId, 7);
}

TEST_F(AuthHandlerRegisterTest, RegisterUser_EmptyUsername_ReturnsError) {
    const auto result = auth.registerUser({"", "password"});
    EXPECT_FALSE(result.has_value());
}

TEST_F(AuthHandlerRegisterTest, RegisterUser_EmptyPassword_ReturnsError) {
    const auto result = auth.registerUser({"alice", ""});
    EXPECT_FALSE(result.has_value());
}

TEST_F(AuthHandlerRegisterTest, RegisterUser_DuplicateUsername_ReturnsError) {
    EXPECT_CALL(mockStore, createUser("alice", _)).WillOnce(Return(-1));
    const auto result = auth.registerUser({"alice", "password"});
    EXPECT_FALSE(result.has_value());
}

TEST_F(AuthHandlerRegisterTest, RegisterUser_StoreNotCalledOnValidationFailure) {
    EXPECT_CALL(mockStore, createUser(_, _)).Times(0);
    const auto result = auth.registerUser({"", ""});
    EXPECT_FALSE(result.has_value());
}

// ---------------------------------------------------------------------------
// loginUser tests
// ---------------------------------------------------------------------------
class AuthHandlerLoginTest : public ::testing::Test {
protected:
    MockUserStore mockStore;
    JwtService    jwt{"test-secret"};
    AuthHandler   auth{mockStore, jwt, minParams()};

    const std::string validHash = makeHash("correct-password");
};

TEST_F(AuthHandlerLoginTest, LoginUser_CorrectCredentials_ReturnsVerifiableToken) {
    EXPECT_CALL(mockStore, findByUsername("alice"))
        .WillOnce(Return(UserRecord{5, validHash}));

    const auto result = auth.loginUser({"alice", "correct-password"});

    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->token.empty());
    const auto userId = jwt.verify(result->token);
    ASSERT_TRUE(userId.has_value());
    EXPECT_EQ(*userId, 5);
}

TEST_F(AuthHandlerLoginTest, LoginUser_WrongPassword_ReturnsError) {
    EXPECT_CALL(mockStore, findByUsername("alice"))
        .WillOnce(Return(UserRecord{5, validHash}));

    const auto result = auth.loginUser({"alice", "wrong-password"});
    EXPECT_FALSE(result.has_value());
}

TEST_F(AuthHandlerLoginTest, LoginUser_UnknownUsername_ReturnsError) {
    EXPECT_CALL(mockStore, findByUsername("nobody"))
        .WillOnce(Return(std::nullopt));

    const auto result = auth.loginUser({"nobody", "password"});
    EXPECT_FALSE(result.has_value());
}

TEST_F(AuthHandlerLoginTest, LoginUser_WrongPassword_ErrorMessageObscured) {
    // Both "wrong user" and "wrong password" must return the same error string
    // so an attacker cannot enumerate valid usernames via error text.
    EXPECT_CALL(mockStore, findByUsername("alice"))
        .WillOnce(Return(UserRecord{5, validHash}));
    EXPECT_CALL(mockStore, findByUsername("nobody"))
        .WillOnce(Return(std::nullopt));

    const auto wrongPass    = auth.loginUser({"alice",  "wrong"});
    const auto unknownUser  = auth.loginUser({"nobody", "password"});

    ASSERT_FALSE(wrongPass.has_value());
    ASSERT_FALSE(unknownUser.has_value());
    EXPECT_EQ(wrongPass.error(), unknownUser.error());
}
