#include "SymbolManager.hpp"
#include "Logging.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

namespace trading {

SymbolManager::SymbolManager(std::filesystem::path jsonPath)
    : m_jsonFilePath(std::move(jsonPath)) {
    m_symbols = loadFromFile();
    if (std::filesystem::exists(m_jsonFilePath))
        m_lastLoadedMtime = std::filesystem::last_write_time(m_jsonFilePath);
}

SymbolManager::~SymbolManager() { stop(); }

std::vector<SymbolInfo> SymbolManager::getSymbols() const {
    std::lock_guard lock(m_symbolsMutex);
    return m_symbols;
}

void SymbolManager::startAutoReload(std::chrono::minutes interval) {
    m_reloadRunning = true;
    m_reloadThread  = std::thread([this, interval] { reloadLoop(interval); });
}

void SymbolManager::stop() {
    m_reloadRunning = false;
    m_stopCv.notify_all();
    if (m_reloadThread.joinable()) m_reloadThread.join();
}

void SymbolManager::reloadLoop(std::chrono::minutes interval) {
    while (m_reloadRunning) {
        {
            std::unique_lock lock(m_stopMutex);
            m_stopCv.wait_for(lock, interval, [this] { return !m_reloadRunning.load(); });
        }
        if (!m_reloadRunning) break;

        if (!std::filesystem::exists(m_jsonFilePath)) {
            LOG(Warning) << "SymbolManager: " << m_jsonFilePath << " not found, skipping reload";
            continue;
        }

        auto mtime = std::filesystem::last_write_time(m_jsonFilePath);
        if (mtime == m_lastLoadedMtime) continue;

        LOG(Info) << "SymbolManager: symbols.json changed, reloading";
        auto fresh = loadFromFile();
        {
            std::lock_guard lock(m_symbolsMutex);
            m_symbols         = std::move(fresh);
            m_lastLoadedMtime = mtime;
        }
        LOG(Info) << "SymbolManager: loaded " << m_symbols.size() << " symbols";
    }
}

std::vector<SymbolInfo> SymbolManager::loadFromFile() const {
    if (!std::filesystem::exists(m_jsonFilePath)) {
        LOG(Warning) << "SymbolManager: " << m_jsonFilePath << " not found, symbol list is empty";
        return {};
    }

    std::ifstream f(m_jsonFilePath);
    if (!f) throw std::runtime_error("SymbolManager: cannot open " + m_jsonFilePath.string());

    const auto j = nlohmann::json::parse(f);
    std::vector<SymbolInfo> result;

    for (const auto& entry : j.at("symbols")) {
        result.push_back({
            entry.at("symbol").get<std::string>(),
            entry.at("name").get<std::string>(),
            entry.value("sector", std::string{})
        });
    }

    LOG(Info) << "SymbolManager: loaded " << result.size() << " symbols from " << m_jsonFilePath;
    return result;
}

} // namespace trading
