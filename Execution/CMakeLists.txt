find_package(GTest REQUIRED)

add_library(trading_execution STATIC
    src/PaperAccount.cpp
)
target_include_directories(trading_execution PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(trading_execution PUBLIC
    trading_core
)
trading_apply_warnings(trading_execution)

add_executable(trading_execution_tests
    tests/PaperAccountTest.cpp
)
target_link_libraries(trading_execution_tests PRIVATE
    trading_execution
    GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(trading_execution_tests)
trading_apply_warnings(trading_execution_tests)
