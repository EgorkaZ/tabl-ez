cmake_minimum_required(VERSION 3.15)
project(tables CXX)


file(GLOB TABLES_HEADERS "src/tables/*.h")
file(GLOB TABLES_SRC "src/tables/*.cpp")

add_library(tables ${TABLES_SRC})
target_include_directories(tables PUBLIC src)

foreach(HDR ${TABLES_HEADERS})
    set_target_properties(tables PROPERTIES PUBLIC_HEADER ${HDR})
endforeach()

install(TARGETS tables)

add_subdirectory(bench)
add_subdirectory(ut)
