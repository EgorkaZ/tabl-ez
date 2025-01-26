find_package(GTest REQUIRED)

file(GLOB TABLES_UT_SRC "*.cpp")

add_executable(tables_ut ${TABLES_UT_SRC})
target_link_libraries(tables_ut tables GTest::gtest GTest::gtest_main GTest::gmock)

add_test(NAME tables_ut COMMAND tables_ut)
