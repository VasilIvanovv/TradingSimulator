#include "SqliteAccountManager.hpp"
#include "SqliteHelpers.hpp"

namespace trading {

using namespace db;

SqliteAccountManager::SqliteAccountManager(const std::filesystem::path& dbPath) {
    const int rc = sqlite3_open(dbPath.string().c_str(), &m_db);
    if (rc != SQLITE_OK) {
        const std::string err = sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        m_db = nullptr;
        throw std::runtime_error(
            std::format("SqliteAccountManager: cannot open '{}': {}", dbPath.string(), err));
    }
    initSchema();
}

SqliteAccountManager::~SqliteAccountManager() {
    if (m_db) sqlite3_close(m_db);
}

void SqliteAccountManager::initSchema() {
    exec(m_db, "PRAGMA journal_mode=WAL;");
    exec(m_db, R"(
        CREATE TABLE IF NOT EXISTS accounts (
            id      INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            name    TEXT    NOT NULL
        );
    )");
}

std::vector<SqliteAccountManager::AccountInfo>
SqliteAccountManager::listAccounts(int userId) {
    auto* stmt = prepare(m_db,
        "SELECT id, name FROM accounts WHERE user_id = ? ORDER BY id;");
    sqlite3_bind_int(stmt, 1, userId);

    std::vector<AccountInfo> result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        AccountInfo info;
        info.id   = sqlite3_column_int(stmt, 0);
        info.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result.push_back(std::move(info));
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<int> SqliteAccountManager::createAccount(int userId,
                                                        const std::string& name) {
    // Enforce cap
    auto* countStmt = prepare(m_db,
        "SELECT COUNT(*) FROM accounts WHERE user_id = ?;");
    sqlite3_bind_int(countStmt, 1, userId);
    sqlite3_step(countStmt);
    const int count = sqlite3_column_int(countStmt, 0);
    sqlite3_finalize(countStmt);

    if (count >= kMaxAccountsPerUser) return std::nullopt;

    auto* stmt = prepare(m_db,
        "INSERT INTO accounts (user_id, name) VALUES (?, ?);");
    sqlite3_bind_int (stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);
    stepDml(stmt);

    return static_cast<int>(sqlite3_last_insert_rowid(m_db));
}

int SqliteAccountManager::ensureDefault(int userId) {
    auto accounts = listAccounts(userId);
    if (!accounts.empty()) return accounts.front().id;
    return *createAccount(userId, "Default");
}

bool SqliteAccountManager::renameAccount(int userId, int accountId,
                                          const std::string& newName) {
    if (!ownsAccount(userId, accountId)) return false;
    auto* stmt = prepare(m_db,
        "UPDATE accounts SET name = ? WHERE id = ? AND user_id = ?;");
    sqlite3_bind_text(stmt, 1, newName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, accountId);
    sqlite3_bind_int (stmt, 3, userId);
    stepDml(stmt);
    return true;
}

bool SqliteAccountManager::deleteAccount(int userId, int accountId) {
    if (!ownsAccount(userId, accountId)) return false;

    auto* stmt = prepare(m_db,
        "DELETE FROM accounts WHERE id = ? AND user_id = ?;");
    sqlite3_bind_int(stmt, 1, accountId);
    sqlite3_bind_int(stmt, 2, userId);
    stepDml(stmt);
    return true;
}

bool SqliteAccountManager::ownsAccount(int userId, int accountId) {
    auto* stmt = prepare(m_db,
        "SELECT 1 FROM accounts WHERE id = ? AND user_id = ?;");
    sqlite3_bind_int(stmt, 1, accountId);
    sqlite3_bind_int(stmt, 2, userId);
    const bool found = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return found;
}

} // namespace trading
