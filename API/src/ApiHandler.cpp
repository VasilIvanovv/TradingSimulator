#include "ApiHandler.hpp"

namespace trading {

ApiHandler::ApiHandler(TradingController& controller) : m_controller(controller) {}

AccountSnapshot ApiHandler::getAccount() const {
    return {
        m_controller.getAvailableCash(),
        m_controller.getAllPositions(),
        m_controller.getTradeHistory()
    };
}

ExecutionReceipt ApiHandler::placeOrder(const PlaceOrderRequest& req) {
    return m_controller.placeOrder({ req.symbol, req.side, req.quantity, req.price, req.timestamp });
}

void ApiHandler::addRule(const AddRuleRequest& req) {
    m_controller.addLimitRule(req.symbol, req.triggerPrice, req.side, req.quantity);
}

void ApiHandler::removeRules(const std::string& symbol) {
    m_controller.removeRules(symbol);
}

std::optional<std::vector<PriceCandle>> ApiHandler::getHistory(const GetHistoryRequest& req) {
    return m_controller.getHistory(req.symbol, req.interval, req.startDate);
}

} // namespace trading
