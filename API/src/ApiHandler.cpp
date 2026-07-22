#include "ApiHandler.hpp"

namespace trading {

ApiHandler::ApiHandler(ControllerFactory factory) : m_factory(std::move(factory)) {}

TradingController& ApiHandler::getOrCreate(int userId) {
    std::lock_guard lock(m_mutex);
    auto it = m_controllers.find(userId);
    if (it == m_controllers.end())
        it = m_controllers.emplace(userId, m_factory(userId)).first;
    return *it->second;
}

AccountSnapshot ApiHandler::getAccount(int userId) {
    auto& c = getOrCreate(userId);
    return {c.getAvailableCash(), c.getAllPositions(), c.getTradeHistory()};
}

ExecutionReceipt ApiHandler::placeOrder(int userId, const PlaceOrderRequest& req) {
    return getOrCreate(userId).placeOrder(
        {req.symbol, req.side, req.quantity, req.price, req.timestamp});
}

void ApiHandler::addRule(int userId, const AddRuleRequest& req) {
    getOrCreate(userId).addLimitRule(req.symbol, req.triggerPrice, req.side, req.quantity);
}

void ApiHandler::removeRules(int userId, const std::string& symbol) {
    getOrCreate(userId).removeRules(symbol);
}

std::optional<std::vector<PriceCandle>> ApiHandler::getHistory(
    int userId, const GetHistoryRequest& req) {
    return getOrCreate(userId).getHistory(req.symbol, req.interval, req.startDate);
}

} // namespace trading
