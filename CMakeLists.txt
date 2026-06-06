cmake_minimum_required(VERSION 3.20)

project(TradingBacktester
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TRADING_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(trading_core INTERFACE)
target_include_directories(trading_core INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_compile_features(trading_core INTERFACE cxx_std_23)

add_executable(trading_simulator src/main.cpp)
target_link_libraries(trading_simulator PRIVATE trading_core)

if(MSVC)
    target_compile_options(trading_simulator PRIVATE /W4)
    if(TRADING_WARNINGS_AS_ERRORS)
        target_compile_options(trading_simulator PRIVATE /WX)
    endif()
else()
    target_compile_options(trading_simulator PRIVATE -Wall -Wextra -Wpedantic)
    if(TRADING_WARNINGS_AS_ERRORS)
        target_compile_options(trading_simulator PRIVATE -Werror)
    endif()
endif()
