#include "TradingController.hpp"
#include "Logging.hpp"

namespace trading {

TradingController::TradingController(std::unique_ptr<IBrokerAccount> account,
                                     std::unique_ptr<DataBroker> broker,
                                     std::string dataInterval,
                                     std::string startDate,
                                     std::chrono::seconds pollInterval)
    : m_account(std::move(account)), m_broker(std::move(broker)),
      m_dataInterval(std::move(dataInterval)),
      m_startDate(std::move(startDate)),
      m_watcher(
          [this](std::string_view symbol) -> std::vector<PriceCandle> {
              auto result =
                  m_broker->getHistory(symbol, m_dataInterval, m_startDate);
              return result.value_or(std::vector<PriceCandle>{});
          },
          [this](const OrderTicket &ticket) {
              return m_account->executeOrder(ticket).status ==
                     ExecutionStatus::Filled;
          },
          [](const OrderTicket &ticket) {
              LOG(Warning) << "Order rejected: "
                           << (ticket.side == OrderSide::Buy ? "BUY" : "SELL")
                           << " " << ticket.quantity << " " << ticket.symbol
                           << " @ " << ticket.price;
          }) {
    m_watcher.setInterval(pollInterval);
}

void TradingController::addLimitRule(const std::string &symbol,
                                     double triggerPrice, OrderSide side,
                                     double quantity) {
    m_watcher.addRule(symbol, triggerPrice, side, quantity);
}

void TradingController::addEngine(std::unique_ptr<IDecisionEngine> engine) {
    m_watcher.addEngine(std::move(engine));
}

void TradingController::removeRules(const std::string &symbol) {
    m_watcher.removeRules(symbol);
}

std::vector<UserLimitTracker::RuleEntry> TradingController::getAllLimitRules() const {
    return m_watcher.getAllRules();
}

double TradingController::getAvailableCash() const {
    return m_account->getAvailableCash();
}

std::unordered_map<std::string, double>
TradingController::getAllPositions() const {
    return m_account->getAllPositions();
}

const std::vector<TradeRecord> &TradingController::getTradeHistory() const {
    return m_account->getTradeHistory();
}

ExecutionReceipt TradingController::placeOrder(const OrderTicket &ticket) {
    return m_account->executeOrder(ticket);
}

void TradingController::deposit(double amount) {
    m_account->deposit(amount);
}

std::optional<std::vector<PriceCandle>> TradingController::getHistory(
    const std::string& symbol, const std::string& interval, const std::string& startDate) {
    return m_broker->getHistory(symbol, interval, startDate);
}

} // namespace trading
