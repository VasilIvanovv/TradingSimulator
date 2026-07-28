#pragma once

#include "ApiTypes.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

namespace trading {

/**
 * @brief Loads and periodically reloads the tradeable symbol catalogue
 *        from a JSON file on disk.
 *
 * The file is read once at construction. A background thread then re-reads
 * it every @p interval minutes; if the file has not been modified since the
 * last load the in-memory list is left unchanged.
 *
 * All public methods are thread-safe.
 */
class SymbolManager {
public:
    /**
     * @param jsonPath  Path to the symbols JSON file, e.g. "symbols.json".
     *                  Format: { "symbols": [ { "symbol", "name", "sector" }, … ] }
     */
    explicit SymbolManager(std::filesystem::path jsonPath);
    ~SymbolManager();

    /** @brief Return a snapshot of the current symbol list. */
    std::vector<SymbolInfo> getSymbols() const;

    /**
     * @brief Start the background reload thread.
     * @param interval  How often to check and reload. Default: 60 minutes.
     */
    void startAutoReload(std::chrono::minutes interval = std::chrono::minutes{60});

    /** @brief Stop the background thread and join it. */
    void stop();

private:
    void reloadLoop(std::chrono::minutes interval);
    std::vector<SymbolInfo> loadFromFile() const;

    std::filesystem::path           m_jsonFilePath;
    std::filesystem::file_time_type m_lastLoadedMtime{};
    mutable std::mutex              m_symbolsMutex; ///< mutable so getSymbols() (const) can lock it
    std::vector<SymbolInfo>         m_symbols;
    std::thread                     m_reloadThread;
    std::atomic<bool>               m_reloadRunning{false};
    std::condition_variable         m_stopCv;       ///< woken by stop() to cut the sleep short
    std::mutex                      m_stopMutex;
};

} // namespace trading
