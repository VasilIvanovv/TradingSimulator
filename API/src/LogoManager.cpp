#include "LogoManager.hpp"
#include "Logging.hpp"

#include <httplib.h>
#include <fstream>

namespace trading {

LogoManager::LogoManager(std::filesystem::path cacheDir,
                         std::function<std::vector<std::string>()> symbolsProvider)
    : m_logoCacheDir(std::move(cacheDir)), m_symbolsProvider(std::move(symbolsProvider)) {
    std::filesystem::create_directories(m_logoCacheDir);
}

LogoManager::~LogoManager() { stop(); }

std::optional<std::vector<char>> LogoManager::get(const std::string& symbol) const {
    auto path = logoPath(symbol);
    if (!std::filesystem::exists(path)) return std::nullopt;

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return std::nullopt;

    const auto size = static_cast<std::streamsize>(file.tellg());
    file.seekg(0);
    std::vector<char> data(static_cast<size_t>(size));
    file.read(data.data(), size);
    if (!file) return std::nullopt;
    return data;
}

void LogoManager::startAutoRefresh(std::chrono::hours interval) {
    m_refreshRunning = true;
    m_refreshThread  = std::thread([this, interval] { refreshLoop(interval); });
}

void LogoManager::stop() {
    m_refreshRunning = false;
    m_stopCv.notify_all();
    if (m_refreshThread.joinable()) m_refreshThread.join();
}

void LogoManager::refreshLoop(std::chrono::hours interval) {
    while (m_refreshRunning) {
        auto lastRefresh = readLastRefreshTime();
        auto elapsed     = std::chrono::system_clock::now() - lastRefresh;
        auto wait        = elapsed >= interval
                           ? std::chrono::seconds{0}
                           : std::chrono::duration_cast<std::chrono::seconds>(interval - elapsed);

        if (wait > std::chrono::seconds{0}) {
            LOG(Info) << "LogoManager: next refresh in "
                      << std::chrono::duration_cast<std::chrono::minutes>(wait).count()
                      << " minutes";

            auto deadline = std::chrono::system_clock::now() + wait;
            while (m_refreshRunning && std::chrono::system_clock::now() < deadline) {
                std::unique_lock lock(m_stopMutex);
                m_stopCv.wait_for(lock, std::chrono::minutes{1},
                                  [this] { return !m_refreshRunning.load(); });
            }
        }

        if (!m_refreshRunning) break;

        LOG(Info) << "LogoManager: refreshing logos for all symbols";
        refresh();
        writeLastRefreshTime();
        LOG(Info) << "LogoManager: refresh complete";
    }
}

void LogoManager::refresh() {
    for (const auto& symbol : m_symbolsProvider()) {
        if (!m_refreshRunning) break;
        const auto bytes = fetch(symbol);
        if (!bytes.empty())
            save(symbol, bytes);
    }
}

std::vector<char> LogoManager::fetch(const std::string& symbol) {
    httplib::SSLClient client("assets.parqet.com");
    client.set_follow_location(true);
    client.set_connection_timeout(10);
    client.set_read_timeout(15);

    const auto res = client.Get("/logos/symbol/" + symbol + "?format=png");

    if (!res || res->status != 200 || res->body.empty()) {
        LOG(Warning) << "LogoManager: failed to fetch " << symbol
                     << " (HTTP=" << (res ? res->status : 0) << ")";
        return {};
    }

    return { res->body.begin(), res->body.end() };
}

void LogoManager::save(const std::string& symbol, const std::vector<char>& bytes) {
    std::ofstream file(logoPath(symbol), std::ios::binary);
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

std::filesystem::path LogoManager::logoPath(const std::string& symbol) const {
    return m_logoCacheDir / (symbol + ".png");
}

std::filesystem::path LogoManager::lastRefreshPath() const {
    return m_logoCacheDir / ".last_refresh";
}

std::chrono::system_clock::time_point LogoManager::readLastRefreshTime() const {
    const auto path = lastRefreshPath();
    if (!std::filesystem::exists(path))
        return std::chrono::system_clock::time_point{}; // epoch → refresh immediately

    std::ifstream f(path);
    long long ts = 0;
    if (!(f >> ts))
        return std::chrono::system_clock::time_point{};

    return std::chrono::system_clock::time_point{std::chrono::seconds{ts}};
}

void LogoManager::writeLastRefreshTime() {
    const auto ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::ofstream f(lastRefreshPath());
    f << ts;
}

} // namespace trading
