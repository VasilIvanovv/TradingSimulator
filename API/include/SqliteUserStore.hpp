#pragma once

#include "IUserStore.hpp"
#include <filesystem>

struct sqlite3;

namespace trading {

class SqliteUserStore : public IUserStore {
public:
    explicit SqliteUserStore(const std::filesystem::path& dbPath);
    ~SqliteUserStore() override;

    int createUser(const std::string& username,
                   const std::string& passwordHash) override;

    std::optional<UserRecord> findByUsername(const std::string& username) override;

private:
    void initSchema();

    sqlite3* m_db{};
};

} // namespace trading
