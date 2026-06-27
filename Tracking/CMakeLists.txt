find_package(GTest REQUIRED)

add_library(trading_tracking STATIC
    src/UserLimitTracker.cpp
    src/MarketWatcher.cpp
)
target_include_directories(trading_tracking PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(trading_tracking PUBLIC
    trading_core
    trading_domain
)
trading_apply_warnings(trading_tracking)

add_executable(trading_tracking_tests
    tests/UserLimitTrackerTest.cpp
    tests/MarketWatcherTest.cpp
)
target_link_libraries(trading_tracking_tests PRIVATE
    trading_tracking
    GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(trading_tracking_tests)
trading_apply_warnings(trading_tracking_tests)
