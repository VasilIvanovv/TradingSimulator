find_package(GTest               CONFIG REQUIRED)
find_package(httplib             CONFIG REQUIRED)
find_package(nlohmann_json              REQUIRED)
find_package(jwt-cpp             CONFIG REQUIRED)
find_package(unofficial-sodium   CONFIG REQUIRED)
find_package(unofficial-sqlite3  CONFIG REQUIRED)

add_library(trading_api STATIC
    src/ApiHandler.cpp
    src/ApiJson.cpp
    src/AuthHandler.cpp
    src/HttpTransport.cpp
    src/JwtService.cpp
    src/LogoManager.cpp
    src/SqliteUserStore.cpp
    src/SymbolManager.cpp
)
target_include_directories(trading_api PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(trading_api PUBLIC
    trading_controller
    trading_common
    httplib::httplib
    nlohmann_json::nlohmann_json
    jwt-cpp::jwt-cpp
    unofficial-sodium::sodium
    unofficial::sqlite3::sqlite3
)
trading_apply_warnings(trading_api)

# --- Tests ---
add_executable(trading_api_tests
    tests/JwtServiceTest.cpp
    tests/SqliteUserStoreTest.cpp
    tests/AuthHandlerTest.cpp
)
target_link_libraries(trading_api_tests PRIVATE
    trading_api
    GTest::gtest_main
    GTest::gmock
)
include(GoogleTest)
gtest_discover_tests(trading_api_tests)
trading_apply_warnings(trading_api_tests)

# --- Integration tests ---
add_executable(trading_api_integration_tests
    tests/integration/HttpTransportTest.cpp
)
target_link_libraries(trading_api_integration_tests PRIVATE
    trading_api
    GTest::gtest_main
)
# Run the whole binary as ONE ctest entry so SetUpTestSuite / TearDownTestSuite
# are shared across all test cases (they start/stop the HTTP server once).
# gtest_discover_tests would split each test into its own process, breaking that.
add_test(NAME trading_api_integration_tests COMMAND trading_api_integration_tests)
set_tests_properties(trading_api_integration_tests PROPERTIES TIMEOUT 60)
trading_apply_warnings(trading_api_integration_tests)
