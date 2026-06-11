add_executable(order_book_benchmark
    BenchmarkRunner.cpp
)

target_include_directories(order_book_benchmark PRIVATE ../include)
target_link_libraries(order_book_benchmark PRIVATE order_book_core)
