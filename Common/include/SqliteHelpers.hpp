#pragma once

#include <sqlite3.h>
#include <format>
#include <stdexcept>
#include <string>

namespace trading::db {

/// Prepare a statement and return the handle. Throws on failure.
inline sqlite3_stmt* prepare(sqlite3* db, const char* sql) {
    sqlite3_stmt* stmt{};
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error(
            std::format("SQLite prepare failed: {}", sqlite3_errmsg(db)));
    return stmt;
}

/// Execute one DML step and finalize. Throws if the step does not return DONE.
inline void stepDml(sqlite3_stmt* stmt) {
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE)
        throw std::runtime_error(
            std::format("SQLite DML step failed ({})", rc));
}

/// Execute one or more SQL statements with no result rows. Throws on failure.
inline void exec(sqlite3* db, const char* sql) {
    char* errMsg{};
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string err = errMsg ? errMsg : "unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error(err);
    }
}

} // namespace trading::db
