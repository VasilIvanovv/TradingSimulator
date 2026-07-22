#include "SqliteUserStore.hpp"
#include "SqliteHelpers.hpp"

namespace trading {

using namespace db;

SqliteUserStore::SqliteUserStore(const std::filesystem::path& dbPath) {
    const int rc = sqlite3_open(dbPath.string().c_str(), &m_db);
    if (rc != SQLITE_OK) {
        const std::string err = sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        m_db = nullptr;
        throw std::runtime_error(
            std::format("SqliteUserStore: cannot open '{}': {}", dbPath.string(), err));
    }
    initSchema();
}

SqliteUserStore::~SqliteUserStore() {
    if (m_db) sqlite3_close(m_db);
}

void SqliteUserStore::initSchema() {
    exec(m_db, "PRAGMA journal_mode=WAL;");
    exec(m_db, R"(
        CREATE TABLE IF NOT EXISTS users (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            username      TEXT    NOT NULL UNIQUE,
            password_hash TEXT    NOT NULL,
            created_at    TEXT    NOT NULL DEFAULT (datetime('now'))
        );
    )");
}

int SqliteUserStore::createUser(const std::string& username,
                                const std::string& passwordHash) {
    auto* stmt = prepare(m_db,
        "INSERT OR IGNORE INTO users (username, password_hash) VALUES (?, ?);");
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, passwordHash.c_str(), -1, SQLITE_STATIC);
    stepDml(stmt);

    if (sqlite3_changes(m_db) == 0)
        return -1; // username already taken

    return static_cast<int>(sqlite3_last_insert_rowid(m_db));
}

std::optional<UserRecord> SqliteUserStore::findByUsername(const std::string& username) {
    auto* stmt = prepare(m_db,
        "SELECT id, password_hash FROM users WHERE username = ?;");
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    UserRecord rec;
    rec.userId       = sqlite3_column_int(stmt, 0);
    rec.passwordHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    sqlite3_finalize(stmt);
    return rec;
}

} // namespace trading
