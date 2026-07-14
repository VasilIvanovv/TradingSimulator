#include "ApiJson.hpp"
#include "ExecutionReceipt.hpp"
#include "OrderSide.hpp"

#include <nlohmann/json.hpp>
#include <stdexcept>

namespace trading {

namespace {

OrderSide parseSide(const std::string& s) {
    if (s == "buy")  return OrderSide::Buy;
    if (s == "sell") return OrderSide::Sell;
    throw std::runtime_error("invalid side '" + s + "': expected \"buy\" or \"sell\"");
}

std::string sideToString(OrderSide side) {
    return side == OrderSide::Buy ? "buy" : "sell";
}

} // namespace

PlaceOrderRequest parsePlaceOrderRequest(const std::string& body) {
    const auto j = nlohmann::json::parse(body);
    return {
        j.at("symbol")  .get<std::string>(),
        parseSide(j.at("side").get<std::string>()),
        j.at("quantity").get<double>(),
        j.at("price")   .get<double>(),
        j.value("timestamp", "")
    };
}

AddRuleRequest parseAddRuleRequest(const std::string& body) {
    const auto j = nlohmann::json::parse(body);
    return {
        j.at("symbol")      .get<std::string>(),
        j.at("triggerPrice").get<double>(),
        parseSide(j.at("side").get<std::string>()),
        j.at("quantity")    .get<double>()
    };
}

std::string toJson(const AccountSnapshot& snapshot) {
    nlohmann::json trades = nlohmann::json::array();
    for (const auto& t : snapshot.tradeHistory) {
        trades.push_back({
            {"symbol",         t.ticket.symbol},
            {"side",           sideToString(t.ticket.side)},
            {"quantity",       t.ticket.quantity},
            {"price",          t.ticket.price},
            {"timestamp",      t.ticket.timestamp},
            {"executionPrice", t.receipt.executionPrice},
            {"status",         t.receipt.status == ExecutionStatus::Filled ? "filled" : "rejected"}
        });
    }
    return nlohmann::json{
        {"cash",         snapshot.cash},
        {"positions",    snapshot.positions},
        {"tradeHistory", trades}
    }.dump();
}

std::string toJson(const ExecutionReceipt& receipt) {
    return nlohmann::json{
        {"status",         receipt.status == ExecutionStatus::Filled ? "filled" : "rejected"},
        {"executionPrice", receipt.executionPrice}
    }.dump();
}

std::string toJson(const std::vector<PriceCandle>& candles) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& c : candles) {
        arr.push_back({
            {"timestamp", c.timestamp},
            {"open",      c.open},
            {"high",      c.high},
            {"low",       c.low},
            {"close",     c.close},
            {"volume",    c.volume}
        });
    }
    return nlohmann::json{{"candles", arr}}.dump();
}

std::string toJsonError(const std::string& message) {
    return nlohmann::json{{"error", message}}.dump();
}

} // namespace trading
