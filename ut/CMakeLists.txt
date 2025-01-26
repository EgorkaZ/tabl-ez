find_package(GTest REQUIRED)

file(GLOB TABLEZ_UT_SRC "*.cpp")

add_executable(tablez_ut ${TABLEZ_UT_SRC})
target_link_libraries(tablez_ut tablez GTest::gtest GTest::gtest_main GTest::gmock)

add_test(NAME tablez_ut COMMAND tablez_ut)
