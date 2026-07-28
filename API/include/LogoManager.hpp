#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace trading {

/**
 * @brief Manages the full lifecycle of stock logo assets: fetching from a CDN,
 *        persisting to disk, and serving cached PNG bytes on demand.
 *
 * Logos are fetched from https://assets.parqet.com/logos/symbol/{SYMBOL}?format=png
 * and stored in @p cacheDir as {SYMBOL}.png.
 *
 * The refresh interval is wall-clock based: the timestamp of the last successful
 * refresh is persisted in @p cacheDir/.last_refresh, so server restarts do not
 * reset the 24-hour window.
 *
 * On startup:
 *   - If no .last_refresh exists, or 24 h have elapsed → fetch all logos immediately.
 *   - Otherwise → wait the remaining time before the first fetch.
 *
 * All public methods are thread-safe.
 */
class LogoManager {
public:
    /**
     * @param cacheDir        Directory where PNG files and .last_refresh are stored.
     *                        Created automatically if it does not exist.
     * @param symbolsProvider Callable that returns the current symbol list to fetch.
     */
    LogoManager(std::filesystem::path cacheDir,
                std::function<std::vector<std::string>()> symbolsProvider);
    ~LogoManager();

    /**
     * @brief Return the raw PNG bytes for @p symbol, or nullopt if not cached.
     *
     * Reads directly from disk on every call; relies on OS page cache for
     * performance. No in-memory buffer to avoid holding large data.
     */
    std::optional<std::vector<char>> get(const std::string& symbol) const;

    /**
     * @brief Start the background refresh thread.
     * @param interval  Full refresh period. Default: 24 hours.
     */
    void startAutoRefresh(std::chrono::hours interval = std::chrono::hours{24});

    /** @brief Stop the background thread and join it. */
    void stop();

private:
    void refreshLoop(std::chrono::hours interval);
    void refresh();

    // Returns raw PNG bytes, or empty vector on failure.
    // HTTP client is inline here; extract to a LogoFetcher if the source ever needs to change.
    std::vector<char> fetch(const std::string& symbol);
    void              save(const std::string& symbol, const std::vector<char>& bytes);

    std::filesystem::path logoPath(const std::string& symbol) const;
    std::filesystem::path lastRefreshPath() const;
    std::chrono::system_clock::time_point readLastRefreshTime() const;
    void writeLastRefreshTime();

    std::filesystem::path                     m_logoCacheDir;
    std::function<std::vector<std::string>()> m_symbolsProvider;
    std::thread                               m_refreshThread;
    std::atomic<bool>                         m_refreshRunning{false};
    std::condition_variable                   m_stopCv;    ///< woken by stop() to cut the sleep short
    std::mutex                                m_stopMutex;
};

} // namespace trading
