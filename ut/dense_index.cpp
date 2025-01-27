#include <gtest/gtest.h>
#include <tablez/dense/index.h>

using namespace testing;
using namespace tablez;

class DenseIndexTest : public Test {};

TEST_F(DenseIndexTest, single_id) {
    dense::Index index;

    auto id = index.push_realloc();
    ASSERT_EQ(id.gen(), 2);
    ASSERT_EQ(id.idx(), 0);

    ASSERT_EQ(index.try_get_idx(id), 0);
    ASSERT_EQ(index.try_remove(id), 0);
    ASSERT_EQ(index.try_remove(id), -1);
    ASSERT_EQ(index.try_remove(id), -1);
    ASSERT_EQ(index.try_get_idx(id), std::nullopt);

    index.dealloc();
}

TEST_F(DenseIndexTest, single_spot) {
    dense::Index index;

    auto fst = index.push_realloc();
    auto sec = index.push_realloc();
    auto thd = index.push_realloc();

    ASSERT_EQ(index.try_get_idx(fst), 0);
    ASSERT_EQ(index.try_get_idx(sec), 1);
    ASSERT_EQ(index.try_get_idx(thd), 2);

    ASSERT_EQ(index.try_remove(sec), 1);
    ASSERT_EQ(index.try_remove(sec), -1);

    ASSERT_EQ(index.count(), 2);

    ASSERT_EQ(index.try_get_idx(fst), 0);
    ASSERT_EQ(index.try_get_idx(sec), std::nullopt);
    ASSERT_EQ(index.try_get_idx(thd), 1);

    auto sec_ = index.push_realloc();
    ASSERT_GT(sec_.gen(), sec.gen());
    ASSERT_EQ(sec_.idx(), sec.idx());

    ASSERT_EQ(index.try_get_idx(fst), 0);
    ASSERT_EQ(index.try_get_idx(sec_), 2);
    ASSERT_EQ(index.try_get_idx(thd), 1);

    ASSERT_EQ(index.try_remove(fst), 0);

    ASSERT_EQ(index.try_get_idx(fst), std::nullopt);
    ASSERT_EQ(index.try_get_idx(sec_), 0);
    ASSERT_EQ(index.try_get_idx(thd), 1);

    auto fst_ = index.push_realloc();
    ASSERT_GT(fst_.gen(), fst.gen());
    ASSERT_EQ(fst_.idx(), fst.idx());

    ASSERT_EQ(index.try_get_idx(fst_), 2);
    ASSERT_EQ(index.try_get_idx(sec_), 0);
    ASSERT_EQ(index.try_get_idx(thd), 1);

    index.dealloc();
}

TEST_F(DenseIndexTest, refill) {
    dense::Index index;

    std::vector ids{index.push_realloc(), index.push_realloc(), index.push_realloc()};
    for (uint32_t i = 0; i < ids.size(); ++i) {
        auto id = ids[i];
        ASSERT_EQ(id.gen(), 2);
        ASSERT_EQ(id.idx(), i);
        ASSERT_NE(index.try_remove(id), -1);
    }

    ids = {index.push_realloc(), index.push_realloc(), index.push_realloc(), index.push_realloc(),
           index.push_realloc()};
    std::vector<uint32_t> idxs{2, 1, 0, 3, 4};
    for (uint32_t i = 0; i < ids.size(); ++i) {
        Id id = ids[i];
        uint32_t idx = idxs[i];

        ASSERT_EQ(id.gen(), i < 3 ? 4 : 2);
        ASSERT_EQ(id.idx(), idx);
        ASSERT_EQ(index.try_get_idx(id), i);
    }

    index.dealloc();
}
