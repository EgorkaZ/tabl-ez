#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tablez/dense/table.h>

using namespace testing;

class DenseTableTest : public Test {};

MATCHER_P(ColumnIs, expected, "") {
    auto it = arg.begin();
    auto exp_it = expected.begin();
    auto exp_end = expected.end();
    size_t idx = 0;

    for (; it != arg.end() && exp_it != exp_end; ++it, ++exp_it, ++idx) {
        auto [_id, val] = *it;
        if (val != *exp_it) {
            if (result_listener->IsInterested()) {
                *(result_listener->stream()) << val << " != " << *exp_it << " at " << idx;
            }
            return false;
        }
    }
    if (it != arg.end()) {
        if (result_listener->IsInterested()) {
            *(result_listener->stream()) << "column is longer than " << idx;
        }
        return false;
    }
    if (exp_it != exp_end) {
        if (result_listener->IsInterested()) {
            *(result_listener->stream()) << "column is shorter than " << expected.size();
        }
        return false;
    }
    return true;
};

TEST_F(DenseTableTest, base) {
    tablez::dense::Table<int, double, bool> table;

    ASSERT_EQ(table.count(), 0);
    ASSERT_EQ(table.capacity(), 0);

    auto fst = table.insert(1, 0.1, true);
    auto sec = table.insert(2, 0.2, false);
    auto thd = table.insert(3, 0.3, false);

    ASSERT_EQ(table.count(), 3);
    ASSERT_EQ(table.capacity(), 4);

    ASSERT_THAT(table.column<int>(), ColumnIs(std::array{1, 2, 3}));
    ASSERT_THAT(table.column<double>(), ColumnIs(std::array{0.1, 0.2, 0.3}));
    ASSERT_THAT(table.column<bool>(), ColumnIs(std::array{true, false, false}));

    ASSERT_TRUE(table.remove(fst));
    ASSERT_TRUE(table.remove(sec));
    ASSERT_EQ(table.count(), 1);

    table.insert(4, 0.4, true);
    // NB! order is not really preserved
    ASSERT_THAT(table.column<int>(), ColumnIs(std::array{3, 4}));
    ASSERT_THAT(table.column<double>(), ColumnIs(std::array{0.3, 0.4}));
    ASSERT_THAT(table.column<bool>(), ColumnIs(std::array{false, true}));
}

TEST_F(DenseTableTest, strings) {
    tablez::dense::Table<int, std::string> table;

    table.insert(1, "kek");
    auto lol = table.insert(2, "lol");
    table.insert(3, "three");
    table.insert(4, "four");

    ASSERT_THAT(table.column<int>(), ColumnIs(std::array{1, 2, 3, 4}));
    ASSERT_THAT(table.column<std::string>(), ColumnIs(std::array{"kek", "lol", "three", "four"}));

    ASSERT_TRUE(table.remove(lol));

    ASSERT_THAT(table.column<int>(), ColumnIs(std::array{1, 4, 3}));
    ASSERT_THAT(table.column<std::string>(), ColumnIs(std::array{"kek", "four", "three"}));
}
