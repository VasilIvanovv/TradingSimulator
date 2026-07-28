#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct sqlite3;

namespace trading {

class SqliteAccountManager {
public:
    static constexpr int kMaxAccountsPerUser = 10;

    struct AccountInfo {
        int         id{};
        std::string name;
    };

    explicit SqliteAccountManager(const std::filesystem::path& dbPath);
    ~SqliteAccountManager();

    // Returns the list of accounts owned by userId (may be empty on first call).
    std::vector<AccountInfo> listAccounts(int userId);

    // Creates an account named `name` for userId.
    // Returns the new account id, or nullopt if the cap is reached.
    std::optional<int> createAccount(int userId, const std::string& name);

    // Ensures the user has at least one account ("Default").
    // Returns the id of the default account (creating it if necessary).
    int ensureDefault(int userId);

    // Renames the account. Returns false if userId does not own accountId.
    bool renameAccount(int userId, int accountId, const std::string& newName);

    // Deletes the account. Returns false if userId does not own accountId.
    bool deleteAccount(int userId, int accountId);

    // Returns true if userId owns accountId.
    bool ownsAccount(int userId, int accountId);

private:
    void initSchema();

    sqlite3* m_db{};
};

} // namespace trading
