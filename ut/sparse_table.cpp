#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <tables/sparse/table.h>

using namespace testing;

class SparseTableTest : public Test {};

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

TEST_F(SparseTableTest, base) {
    tables::sparse::Table<int, double, bool> table;

    ASSERT_EQ(table.count(), 0);
    ASSERT_EQ(table.capacity(), 0);

    auto fst = table.insert(1, 0.1, true);
    auto sec = table.insert(2, 0.2, false);
    auto thd = table.insert(3, 0.3, false);

    ASSERT_EQ(table.count(), 3);
    ASSERT_EQ(table.capacity(), 4);

    ASSERT_THAT(table.column<int>().range(), ColumnIs(std::array{1, 2, 3}));
    ASSERT_THAT(table.column<double>().range(), ColumnIs(std::array{0.1, 0.2, 0.3}));
    ASSERT_THAT(table.column<bool>().range(), ColumnIs(std::array{true, false, false}));

    table.remove(sec);
    table.remove(fst);
    ASSERT_EQ(table.count(), 1);

    table.insert(4, 0.4, true);
    ASSERT_THAT(table.column<int>().range(), ColumnIs(std::array{4, 3}));
    ASSERT_THAT(table.column<double>().range(), ColumnIs(std::array{0.4, 0.3}));
    ASSERT_THAT(table.column<bool>().range(), ColumnIs(std::array{true, false}));
}

TEST_F(SparseTableTest, strings) {
    tables::sparse::Table<int, std::string> table;

    table.insert(1, "kek");
    auto lol = table.insert(2, "lol");
    table.insert(3, "three");
    table.insert(4, "four");

    ASSERT_THAT(table.column<int>().range(), ColumnIs(std::array{1, 2, 3, 4}));
    ASSERT_THAT(table.column<std::string>().range(), ColumnIs(std::array{"kek", "lol", "three", "four"}));

    table.remove(lol);

    ASSERT_THAT(table.column<int>().range(), ColumnIs(std::array{1, 3, 4}));
    ASSERT_THAT(table.column<std::string>().range(), ColumnIs(std::array{"kek", "three", "four"}));
}
