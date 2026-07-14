#pragma once

#include "IAccountStore.hpp"
#include <filesystem>

struct sqlite3; // opaque SQLite handle — full definition only needed in .cpp

namespace trading {

class SqliteAccountStore : public IAccountStore {
public:
    explicit SqliteAccountStore(const std::filesystem::path& dbPath);
    ~SqliteAccountStore() override;

    bool load(double& outCash,
              std::unordered_map<std::string, double>& outPositions,
              std::vector<TradeRecord>& outTrades) override;

    void persist(double cash,
                 const std::string& symbol,
                 double newPosition,
                 const TradeRecord& trade) override;

private:
    void initSchema();
    void exec(const char* sql);

    sqlite3* m_db{};
};

} // namespace trading
