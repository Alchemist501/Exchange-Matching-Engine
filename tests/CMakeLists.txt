include(FetchContent)

message(STATUS "Fetching GoogleTest...")
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(order_book_tests
    MatchingTests.cpp
    CancelTests.cpp
)

target_include_directories(order_book_tests PRIVATE ../include)
target_link_libraries(order_book_tests PRIVATE order_book_core gtest_main)

include(GoogleTest)
gtest_discover_tests(order_book_tests)
