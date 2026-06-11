cmake_minimum_required(VERSION 3.20)

project(TradingBacktester
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TRADING_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

# --- Dependencies ---
include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install    OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(googletest)

# --- Libraries ---

# Header-only interfaces and POD types
add_library(trading_core INTERFACE)
target_include_directories(trading_core INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_compile_features(trading_core INTERFACE cxx_std_23)

# Concrete data-layer implementations
add_library(trading_data STATIC
    src/Logging.cpp
    src/data/DataBroker.cpp
    src/data/TwelveDataProvider.cpp
    src/data/AlpacaProvider.cpp
    src/data/CsvLocalCache.cpp
)
target_link_libraries(trading_data PUBLIC trading_core nlohmann_json::nlohmann_json)

if(WIN32)
    target_link_libraries(trading_data PUBLIC winhttp)
endif()

add_executable(trading_simulator src/main.cpp)
target_link_libraries(trading_simulator PRIVATE trading_data)

# --- Tests ---
enable_testing()

add_executable(trading_tests
    tests/DataBrokerTest.cpp
)
target_link_libraries(trading_tests PRIVATE
    trading_data
    GTest::gtest_main
    GTest::gmock
)
include(GoogleTest)
gtest_discover_tests(trading_tests)

set(_trading_targets trading_data trading_simulator trading_tests)

foreach(_target IN LISTS _trading_targets)
    if(MSVC)
        target_compile_options(${_target} PRIVATE /W4)
        if(TRADING_WARNINGS_AS_ERRORS)
            target_compile_options(${_target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${_target} PRIVATE -Wall -Wextra -Wpedantic)
        if(TRADING_WARNINGS_AS_ERRORS)
            target_compile_options(${_target} PRIVATE -Werror)
        endif()
    endif()
endforeach()
