find_package(benchmark REQUIRED)

file(GLOB TABLEZ_BENCH_SRC "*.cpp")

add_executable(tablez_bench ${TABLEZ_BENCH_SRC})
target_link_libraries(tablez_bench tablez benchmark::benchmark benchmark::benchmark_main)

add_test(NAME tablez_bench COMMAND tablez_bench)
