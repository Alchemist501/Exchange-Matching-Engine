cmake_minimum_required(VERSION 3.20)
project(OrderBookSimulator CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable common warnings for high quality systems engineering
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -O3)
endif()

include(FetchContent)

# 1. Fetch JSON (nlohmann)
message(STATUS "Fetching nlohmann/json...")
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# 2. Fetch standalone ASIO (required by Crow)
message(STATUS "Fetching standalone Asio...")
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-28-0
)
FetchContent_MakeAvailable(asio)
set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include" CACHE PATH "Path to ASIO headers" FORCE)
include_directories(SYSTEM "${ASIO_INCLUDE_DIR}")

# 3. Fetch Crow Web Framework
message(STATUS "Fetching Crow C++ Web Framework...")
FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/Crowcpp/Crow.git
    GIT_TAG v1.2.0
)
# Crow relies on ASIO. We force Crow to build in header-only mode with standalone ASIO
set(CROW_FEATURES "" CACHE STRING "Features enabled in Crow")
set(CROW_AMALGAMATED OFF CACHE BOOL "Use amalgamated Crow")
FetchContent_MakeAvailable(crow)

# Include directories
include_directories(include)

# 3. Core Engine Library
add_library(order_book_core STATIC
    src/OrderBook.cpp
    src/MatchingEngine.cpp
    src/Statistics.cpp
    src/RiskEngine.cpp
    src/Portfolio.cpp
)
target_link_libraries(order_book_core PRIVATE nlohmann_json::nlohmann_json)

# 4. Main Server Executable
add_executable(order_book_server
    src/main.cpp
    src/WebServer.cpp
)
target_link_libraries(order_book_server PRIVATE order_book_core Crow::Crow nlohmann_json::nlohmann_json)

# 5. Testing and Benchmarking
enable_testing()
add_subdirectory(tests)
add_subdirectory(benchmark)
