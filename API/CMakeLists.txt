find_package(httplib       CONFIG REQUIRED)
find_package(nlohmann_json        REQUIRED)

add_library(trading_api STATIC
    src/ApiHandler.cpp
    src/ApiJson.cpp
    src/HttpTransport.cpp
)
target_include_directories(trading_api PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(trading_api PUBLIC
    trading_controller
    httplib::httplib
    nlohmann_json::nlohmann_json
)
trading_apply_warnings(trading_api)
