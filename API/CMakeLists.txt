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
    src/SqliteUserStore.cpp
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
