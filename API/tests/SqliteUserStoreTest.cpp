#include <gtest/gtest.h>
#include "SqliteUserStore.hpp"

using namespace trading;

// Each test gets a fresh in-memory database — no files written, no cleanup needed.
class SqliteUserStoreTest : public ::testing::Test {
protected:
    SqliteUserStore store{":memory:"};
};

TEST_F(SqliteUserStoreTest, CreateUser_ReturnsPositiveId) {
    EXPECT_GT(store.createUser("alice", "hash"), 0);
}

TEST_F(SqliteUserStoreTest, CreateUser_DuplicateUsername_ReturnsMinusOne) {
    store.createUser("alice", "hash1");
    EXPECT_EQ(store.createUser("alice", "hash2"), -1);
}

TEST_F(SqliteUserStoreTest, CreateUser_TwoUsers_GetDifferentIds) {
    const int id1 = store.createUser("alice", "hash1");
    const int id2 = store.createUser("bob",   "hash2");
    EXPECT_NE(id1, id2);
}

TEST_F(SqliteUserStoreTest, FindByUsername_ExistingUser_ReturnsRecord) {
    const int id = store.createUser("alice", "stored-hash");
    const auto rec = store.findByUsername("alice");
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->userId, id);
    EXPECT_EQ(rec->passwordHash, "stored-hash");
}

TEST_F(SqliteUserStoreTest, FindByUsername_UnknownUser_ReturnsNullopt) {
    EXPECT_FALSE(store.findByUsername("nobody").has_value());
}

TEST_F(SqliteUserStoreTest, FindByUsername_CaseSensitive) {
    store.createUser("alice", "hash");
    EXPECT_FALSE(store.findByUsername("Alice").has_value());
    EXPECT_FALSE(store.findByUsername("ALICE").has_value());
}

TEST_F(SqliteUserStoreTest, FindByUsername_AfterDuplicateAttempt_ReturnsOriginal) {
    const int id = store.createUser("alice", "original-hash");
    store.createUser("alice", "new-hash"); // ignored
    const auto rec = store.findByUsername("alice");
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->userId, id);
    EXPECT_EQ(rec->passwordHash, "original-hash");
}
