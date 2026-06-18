#pragma once

#include <string>

namespace trading {

struct PriceCandle {
    std::string timestamp;
    double open{};
    double high{};
    double low{};
    double close{};
    long volume{};
};

} // namespace trading
