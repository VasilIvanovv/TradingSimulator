#include "SqliteAccountStore.hpp"
#include "OrderSide.hpp"

#include <sqlite3.h>
#include <format>
#include <stdexcept>

namespace trading {

namespace {

sqlite3_stmt* prepare(sqlite3* db, const char* sql) {
    sqlite3_stmt* stmt{};
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error(
            std::format("SqliteAccountStore: prepare failed: {}", sqlite3_errmsg(db)));
    return stmt;
}

// For DML (INSERT/UPDATE/DELETE): executes one step and finalizes.
void stepDml(sqlite3_stmt* stmt) {
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE)
        throw std::runtime_error(
            std::format("SqliteAccountStore: DML step failed ({})", rc));
}

} // namespace

SqliteAccountStore::SqliteAccountStore(const std::filesystem::path& dbPath) {
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

void SqliteAccountStore::exec(const char* sql) {
    char* errMsg{};
    const int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string err = errMsg ? errMsg : "unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error(std::format("SqliteAccountStore: {}", err));
    }
}

void SqliteAccountStore::initSchema() {
    exec(R"(
        CREATE TABLE IF NOT EXISTS account_state (
            id   INTEGER PRIMARY KEY CHECK (id = 1),
            cash REAL NOT NULL
        );
        CREATE TABLE IF NOT EXISTS positions (
            symbol   TEXT PRIMARY KEY,
            quantity REAL NOT NULL
        );
        CREATE TABLE IF NOT EXISTS trade_history (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            symbol          TEXT NOT NULL,
            side            INTEGER NOT NULL,
            quantity        REAL NOT NULL,
            price           REAL NOT NULL,
            timestamp       TEXT NOT NULL,
            execution_price REAL NOT NULL
        );
    )");
}

bool SqliteAccountStore::load(double& outCash,
                               std::unordered_map<std::string, double>& outPositions,
                               std::vector<TradeRecord>& outTrades) {
    sqlite3_stmt* stmt = prepare(m_db, "SELECT cash FROM account_state WHERE id = 1;");
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return false;
    }
    outCash = sqlite3_column_double(stmt, 0);
    sqlite3_finalize(stmt);

    stmt = prepare(m_db, "SELECT symbol, quantity FROM positions;");
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const std::string symbol =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        outPositions[symbol] = sqlite3_column_double(stmt, 1);
    }
    sqlite3_finalize(stmt);

    stmt = prepare(m_db,
        "SELECT symbol, side, quantity, price, timestamp, execution_price "
        "FROM trade_history ORDER BY id;");
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
    exec("BEGIN;");
    try {
        // Cash
        auto* stmt = prepare(m_db,
            "INSERT OR REPLACE INTO account_state (id, cash) VALUES (1, ?);");
        sqlite3_bind_double(stmt, 1, cash);
        stepDml(stmt);

        // Position — upsert or delete if fully closed
        if (newPosition > 0.0) {
            stmt = prepare(m_db,
                "INSERT OR REPLACE INTO positions (symbol, quantity) VALUES (?, ?);");
            sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 2, newPosition);
        } else {
            stmt = prepare(m_db, "DELETE FROM positions WHERE symbol = ?;");
            sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_STATIC);
        }
        stepDml(stmt);

        // Trade record
        stmt = prepare(m_db,
            "INSERT INTO trade_history "
            "(symbol, side, quantity, price, timestamp, execution_price) "
            "VALUES (?, ?, ?, ?, ?, ?);");
        sqlite3_bind_text  (stmt, 1, trade.ticket.symbol.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int   (stmt, 2, trade.ticket.side == OrderSide::Buy ? 0 : 1);
        sqlite3_bind_double(stmt, 3, trade.ticket.quantity);
        sqlite3_bind_double(stmt, 4, trade.ticket.price);
        sqlite3_bind_text  (stmt, 5, trade.ticket.timestamp.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 6, trade.receipt.executionPrice);
        stepDml(stmt);

        exec("COMMIT;");
    } catch (...) {
        exec("ROLLBACK;");
        throw;
    }
}

} // namespace trading
