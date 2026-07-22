#include "SqliteAccountStore.hpp"
#include "SqliteHelpers.hpp"
#include "OrderSide.hpp"

namespace trading {

using namespace db;

SqliteAccountStore::SqliteAccountStore(const std::filesystem::path& dbPath, int userId)
    : m_userId(userId) {
    const int rc = sqlite3_open(dbPath.string().c_str(), &m_db);
    if (rc != SQLITE_OK) {
        const std::string err = sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        m_db = nullptr;
        throw std::runtime_error(
            std::format("SqliteAccountStore: cannot open '{}': {}", dbPath.string(), err));
    }
    initSchema();
}

SqliteAccountStore::~SqliteAccountStore() {
    if (m_db) sqlite3_close(m_db);
}

void SqliteAccountStore::initSchema() {
    exec(m_db, "PRAGMA journal_mode=WAL;");
    exec(m_db, R"(
        CREATE TABLE IF NOT EXISTS account_state (
            user_id INTEGER PRIMARY KEY,
            cash    REAL    NOT NULL
        );
        CREATE TABLE IF NOT EXISTS positions (
            user_id  INTEGER NOT NULL,
            symbol   TEXT    NOT NULL,
            quantity REAL    NOT NULL,
            PRIMARY KEY (user_id, symbol)
        );
        CREATE TABLE IF NOT EXISTS trade_history (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id         INTEGER NOT NULL,
            symbol          TEXT    NOT NULL,
            side            INTEGER NOT NULL,
            quantity        REAL    NOT NULL,
            price           REAL    NOT NULL,
            timestamp       TEXT    NOT NULL,
            execution_price REAL    NOT NULL
        );
    )");
}

bool SqliteAccountStore::load(double& outCash,
                               std::unordered_map<std::string, double>& outPositions,
                               std::vector<TradeRecord>& outTrades) {
    auto* stmt = prepare(m_db,
        "SELECT cash FROM account_state WHERE user_id = ?;");
    sqlite3_bind_int(stmt, 1, m_userId);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return false;
    }
    outCash = sqlite3_column_double(stmt, 0);
    sqlite3_finalize(stmt);

    stmt = prepare(m_db,
        "SELECT symbol, quantity FROM positions WHERE user_id = ?;");
    sqlite3_bind_int(stmt, 1, m_userId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const std::string symbol =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        outPositions[symbol] = sqlite3_column_double(stmt, 1);
    }
    sqlite3_finalize(stmt);

    stmt = prepare(m_db,
        "SELECT symbol, side, quantity, price, timestamp, execution_price "
        "FROM trade_history WHERE user_id = ? ORDER BY id;");
    sqlite3_bind_int(stmt, 1, m_userId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TradeRecord record;
        record.ticket.symbol =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        record.ticket.side      = sqlite3_column_int(stmt, 1) == 0 ? OrderSide::Buy : OrderSide::Sell;
        record.ticket.quantity  = sqlite3_column_double(stmt, 2);
        record.ticket.price     = sqlite3_column_double(stmt, 3);
        record.ticket.timestamp =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.receipt.executionPrice = sqlite3_column_double(stmt, 5);
        record.receipt.status   = ExecutionStatus::Filled;
        outTrades.push_back(std::move(record));
    }
    sqlite3_finalize(stmt);

    return true;
}

void SqliteAccountStore::persist(double cash,
                                  const std::string& symbol,
                                  double newPosition,
                                  const TradeRecord& trade) {
    exec(m_db, "BEGIN;");
    try {
        // Cash
        auto* stmt = prepare(m_db,
            "INSERT OR REPLACE INTO account_state (user_id, cash) VALUES (?, ?);");
        sqlite3_bind_int   (stmt, 1, m_userId);
        sqlite3_bind_double(stmt, 2, cash);
        stepDml(stmt);

        // Position — upsert or delete if fully closed
        if (newPosition > 0.0) {
            stmt = prepare(m_db,
                "INSERT OR REPLACE INTO positions (user_id, symbol, quantity) VALUES (?, ?, ?);");
            sqlite3_bind_int   (stmt, 1, m_userId);
            sqlite3_bind_text  (stmt, 2, symbol.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 3, newPosition);
        } else {
            stmt = prepare(m_db,
                "DELETE FROM positions WHERE user_id = ? AND symbol = ?;");
            sqlite3_bind_int (stmt, 1, m_userId);
            sqlite3_bind_text(stmt, 2, symbol.c_str(), -1, SQLITE_STATIC);
        }
        stepDml(stmt);

        // Trade record
        stmt = prepare(m_db,
            "INSERT INTO trade_history "
            "(user_id, symbol, side, quantity, price, timestamp, execution_price) "
            "VALUES (?, ?, ?, ?, ?, ?, ?);");
        sqlite3_bind_int   (stmt, 1, m_userId);
        sqlite3_bind_text  (stmt, 2, trade.ticket.symbol.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int   (stmt, 3, trade.ticket.side == OrderSide::Buy ? 0 : 1);
        sqlite3_bind_double(stmt, 4, trade.ticket.quantity);
        sqlite3_bind_double(stmt, 5, trade.ticket.price);
        sqlite3_bind_text  (stmt, 6, trade.ticket.timestamp.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 7, trade.receipt.executionPrice);
        stepDml(stmt);

        exec(m_db, "COMMIT;");
    } catch (...) {
        exec(m_db, "ROLLBACK;");
        throw;
    }
}

} // namespace trading
