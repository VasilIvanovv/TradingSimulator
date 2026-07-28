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

std::string toJson(const std::vector<SymbolInfo>& symbols) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& s : symbols)
        arr.push_back({{"symbol", s.symbol}, {"name", s.name}, {"sector", s.sector}});
    return arr.dump();
}

std::string toJson(const std::vector<ActiveRule>& rules) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& r : rules)
        arr.push_back({
            {"symbol",       r.symbol},
            {"triggerPrice", r.triggerPrice},
            {"side",         sideToString(r.side)},
            {"quantity",     r.quantity}
        });
    return arr.dump();
}

std::string toJson(const std::vector<AccountInfo>& accounts) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& a : accounts)
        arr.push_back({{"id", a.id}, {"name", a.name}});
    return arr.dump();
}

DepositRequest parseDepositRequest(const std::string& body) {
    const auto j = nlohmann::json::parse(body);
    return { j.at("amount").get<double>() };
}

AccountNameRequest parseAccountNameRequest(const std::string& body) {
    const auto j = nlohmann::json::parse(body);
    return { j.at("name").get<std::string>() };
}

std::string toJson(const DepositAllResult& result) {
    return nlohmann::json{
        {"succeeded", result.succeeded},
        {"failed",    result.failed}
    }.dump();
}

std::string toJson(const AccountInfo& account) {
    return nlohmann::json{{"id", account.id}, {"name", account.name}}.dump();
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

RegisterRequest parseRegisterRequest(const std::string& body) {
    const auto j = nlohmann::json::parse(body);
    return {j.at("username").get<std::string>(), j.at("password").get<std::string>()};
}

LoginRequest parseLoginRequest(const std::string& body) {
    const auto j = nlohmann::json::parse(body);
    return {j.at("username").get<std::string>(), j.at("password").get<std::string>()};
}

std::string toJson(const AuthResponse& response) {
    return nlohmann::json{{"token", response.token}}.dump();
}

std::string toJsonError(const std::string& message) {
    return nlohmann::json{{"error", message}}.dump();
}

} // namespace trading
