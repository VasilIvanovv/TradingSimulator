find_package(CURL          REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(GTest         REQUIRED)

# --- MarketData library ---
add_library(trading_market_data STATIC
    src/DataBroker.cpp
    src/TwelveDataProvider.cpp
    src/AlpacaProvider.cpp
    src/CsvLocalCache.cpp
)
target_include_directories(trading_market_data PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(trading_market_data PUBLIC
    trading_core
    trading_domain
    nlohmann_json::nlohmann_json
    CURL::libcurl
)
trading_apply_warnings(trading_market_data)

# --- Tests ---
add_executable(trading_tests
    tests/DataBrokerTest.cpp
    tests/CsvLocalCacheTest.cpp
)
target_link_libraries(trading_tests PRIVATE
    trading_market_data
    GTest::gtest_main
    GTest::gmock
)
include(GoogleTest)
gtest_discover_tests(trading_tests)
trading_apply_warnings(trading_tests)
