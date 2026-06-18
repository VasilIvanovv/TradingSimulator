#include "PaperAccount.hpp"

namespace trading {

PaperAccount::PaperAccount(double initialCash) : m_cash(initialCash) {}

ExecutionReceipt PaperAccount::executeOrder(const OrderTicket& ticket) {
    const double totalCost = ticket.price * ticket.quantity;

    if (ticket.side == OrderSide::Buy) {
        if (m_cash < totalCost)
            return ExecutionReceipt{ ExecutionStatus::Rejected, 0.0 };
        m_cash -= totalCost;
        m_positions[ticket.symbol] += ticket.quantity;
    } else {
        auto it = m_positions.find(ticket.symbol);
        if (it == m_positions.end() || it->second < ticket.quantity)
            return ExecutionReceipt{ ExecutionStatus::Rejected, 0.0 };
        it->second -= ticket.quantity;
        if (it->second == 0.0)
            m_positions.erase(it);
        m_cash += totalCost;
    }

    return ExecutionReceipt{ ExecutionStatus::Filled, ticket.price };
}

double PaperAccount::getAvailableCash() const { return m_cash; }

double PaperAccount::getPosition(const std::string& symbol) const {
    const auto it = m_positions.find(symbol);
    return it != m_positions.end() ? it->second : 0.0;
}

} // namespace trading
