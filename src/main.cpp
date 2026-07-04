#include "Logging.hpp"
#include "TradingController.hpp"
#include "CsvLocalCache.hpp"
#include "PaperAccount.hpp"
#include "TwelveDataProvider.hpp"

#include <format>
#include <iostream>

int main() {
    std::cout << "TradingSimulator v0.1.0\n\n";

    constexpr std::string_view apiKey    = "09bc29fdccc54c0ca12c5c7353d3cb1f";
    constexpr std::string_view interval  = "1day";
    constexpr std::string_view startDate = "2025-01-01";

    std::vector<std::unique_ptr<trading::IDataProvider>> providers;
    providers.push_back(std::make_unique<trading::TwelveDataProvider>(std::string(apiKey)));

    trading::TradingController controller(
        std::make_unique<trading::PaperAccount>(10'000.0),
        std::make_unique<trading::DataBroker>(
            std::move(providers),
            std::make_unique<trading::CsvLocalCache>("cache")
        ),
        std::string(interval),
        std::string(startDate)
    );

    std::cout << std::format("Cash: ${:.2f}\n", controller.getAvailableCash());
    std::cout << "Positions: " << controller.getAllPositions().size() << "\n\n";

    // Place a manual order
    trading::OrderTicket ticket{ "AAPL", trading::OrderSide::Buy, 5.0, 190.0, "2025-01-15" };
    auto receipt = controller.placeOrder(ticket);
    std::cout << std::format("Manual BUY 5 AAPL @ $190.00 -> {}\n",
        receipt.status == trading::ExecutionStatus::Filled ? "Filled" : "Rejected");
    std::cout << std::format("Cash after order: ${:.2f}\n\n", controller.getAvailableCash());

    // Register a limit rule — will fire automatically on the next watcher tick
    controller.addLimitRule("AAPL", 200.0, trading::OrderSide::Sell, 5.0);
    LOG(Info) << "Limit rule added: SELL 5 AAPL @ $200.00";

    // Print trade history
    const auto& history = controller.getTradeHistory();
    std::cout << std::format("\nTrade history ({} trades):\n", history.size());
    for (const auto& t : history) {
        std::cout << std::format("  {} {} {} @ ${:.2f} -> {}\n",
            t.ticket.timestamp,
            t.ticket.side == trading::OrderSide::Buy ? "BUY" : "SELL",
            t.ticket.symbol,
            t.receipt.executionPrice,
            t.receipt.status == trading::ExecutionStatus::Filled ? "Filled" : "Rejected");
    }

    return 0;
}
