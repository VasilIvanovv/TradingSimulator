cmake_minimum_required(VERSION 3.20)

project(TradingBacktester
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static CRT to match the static vcpkg triplet (x64-windows-static) — keeps
# every dependency, including GTest, statically linked. GTest built as a DLL
# has a known MSVC issue where TEST()/TEST_F() static-initializer
# registration silently fails to cross the DLL boundary.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(TRADING_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

# Shared helper: apply standard warnings to a target.
macro(trading_apply_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        if(TRADING_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(TRADING_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endmacro()

# --- Core library (Logging + shared include path) ---
add_library(trading_core STATIC src/Logging.cpp)
target_include_directories(trading_core PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_features(trading_core PUBLIC cxx_std_23)
trading_apply_warnings(trading_core)

# --- Shared domain types (header-only) ---
add_library(trading_domain INTERFACE)
target_include_directories(trading_domain INTERFACE ${PROJECT_SOURCE_DIR}/Domain/include)

# --- Modules ---
enable_testing()
add_subdirectory(MarketData)
add_subdirectory(Execution)
add_subdirectory(Tracking)

# --- Executable ---
add_executable(trading_simulator src/main.cpp)
target_link_libraries(trading_simulator PRIVATE trading_market_data)
trading_apply_warnings(trading_simulator)
