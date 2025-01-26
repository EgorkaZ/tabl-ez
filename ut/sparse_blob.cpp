#include <tablez/sparse/blob.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <unordered_map>

using namespace testing;

class SparseBlobTest;

struct DestructionChecker {
    explicit DestructionChecker(SparseBlobTest *owner, uint32_t idx);

    ~DestructionChecker();

    SparseBlobTest *owner = nullptr;
    uint32_t idx;
};

class SparseBlobTest : public Test {
public:
    void Register(DestructionChecker *value, uint32_t idx) { registered.try_emplace(value, idx); }

    void Deregister(DestructionChecker *value) {
        ASSERT_TRUE(registered.contains(value));
        registered.erase(value);
    }

    std::unordered_map<DestructionChecker *, uint32_t> registered;
};

DestructionChecker::DestructionChecker(SparseBlobTest *owner, uint32_t idx) : owner{owner}, idx{idx} {
    owner->Register(this, idx);
}

DestructionChecker::~DestructionChecker() { owner->Deregister(this); }

TEST_F(SparseBlobTest, base) {
    auto blob = tablez::sparse::Blob<DestructionChecker>::with_capacity(32);

    blob.init_at(1, this, 1);
    ASSERT_EQ(blob.assume_init_at(1).idx, 1);
    ASSERT_EQ(blob.assume_init_at(1).owner, this);

    blob.destroy(32, [](uint32_t idx) { return idx == 1; });
    blob.dealloc();

    ASSERT_EQ(registered.size(), 0);
}
