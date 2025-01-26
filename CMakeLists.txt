cmake_minimum_required(VERSION 3.15)
project(tablez CXX)


file(GLOB TABLEZ_HEADERS "src/tablez/*.h")
file(GLOB TABLEZ_SRC "src/tablez/*.cpp")

add_library(tablez ${TABLEZ_SRC})
target_include_directories(tablez PUBLIC src)

foreach(HDR ${TABLEZ_HEADERS})
    set_target_properties(tablez PROPERTIES PUBLIC_HEADER ${HDR})
endforeach()

install(TARGETS tablez)

add_subdirectory(bench)
add_subdirectory(ut)
