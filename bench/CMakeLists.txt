find_package(benchmark REQUIRED)

file(GLOB TABLES_BENCH_SRC "*.cpp")

add_executable(tables_bench ${TABLES_BENCH_SRC})
target_link_libraries(tables_bench tables benchmark::benchmark benchmark::benchmark_main)

add_test(NAME tables_bench COMMAND tables_bench)
