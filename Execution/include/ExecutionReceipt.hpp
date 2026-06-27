#pragma once

namespace trading {

enum class ExecutionStatus { Filled, Rejected };

struct ExecutionReceipt {
    ExecutionStatus status{};
    double executionPrice{};
};

} // namespace trading
