#pragma once

#include <tables/id.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <ranges>
#include <span>

namespace tables::sparse {

class IndexIter;
class IndexIterEnd {};

// doesn't necessarily own it's stuff
class Index {
    explicit Index(uint32_t capacity)
        : gens_(new(std::align_val_t{32}) uint32_t[capacity]), capacity_(capacity), count_(0) {
        std::fill_n(gens_, capacity_, EMPTY_MASK);
    }

public:
    constexpr Index() noexcept = default;

    constexpr static uint32_t EMPTY_MASK = 1;

    static Index with_capacity(uint32_t capacity) { return Index(capacity); }

    bool is_set(uint32_t idx) noexcept {
        assert(idx < capacity_);
        return !(gens_[idx] & EMPTY_MASK);
    }

    Id get_unchecked(uint32_t idx) noexcept {
        assert(idx < capacity_);
        assert(is_set(idx));
        return Id{gens_[idx], idx};
    }

    Id set_unchecked(uint32_t idx) noexcept {
        assert(idx < capacity_);
        assert(!is_set(idx));
        assert(gens_[idx] < std::numeric_limits<uint32_t>::max());

        uint32_t gen = ++gens_[idx];
        ++count_;
        return Id{gen, idx};
    }

    Id reset_unchecked(Id id) noexcept {
        assert(id.idx() < capacity_);
        assert(is_set(id.idx()));
        assert(id.gen() == gens_[id.idx()]);

        uint32_t gen = ++gens_[id.idx()];
        --count_;
        return Id{gen, id.idx()};
    }

    uint32_t capacity() const noexcept { return capacity_; }

    uint32_t count() const noexcept { return count_; }

    void dealloc() noexcept {
        if (gens_) {
            delete[] gens_;
            gens_ = nullptr;
        }
    }

    auto set_range() const noexcept {
        return std::span<uint32_t>{gens_, capacity_} |
               std::ranges::views::filter([](uint32_t gen) { return !(gen & EMPTY_MASK); }) |
               std::ranges::views::transform([base = gens_](const uint32_t &gen) {
                   uint32_t offset = (&gen - base);
                   return Id{gen, offset};
               });
    }

    void reserve_for(uint32_t new_capacity) {
        assert(new_capacity >= capacity_);
        auto *new_gens = new uint32_t[new_capacity];
        for (uint32_t i = 0; i < capacity_; ++i) {
            new_gens[i] = gens_[i];
        }
        for (uint32_t i = capacity_; i < new_capacity; ++i) {
            new_gens[i] = EMPTY_MASK;
        }
        delete[] gens_;
        gens_ = new_gens;
        capacity_ = new_capacity;
    }

    IndexIter begin() const noexcept;
    IndexIterEnd end() const noexcept;

    template <class Func>
        requires(std::is_invocable_r_v<void, Func, Id>)
    void for_each(Func &&func) noexcept(std::is_nothrow_invocable_v<Func, Id>) {
        for (uint32_t i = 0; i < capacity_; ++i) {
            auto gen = gens_[i];
            if (!(gen & EMPTY_MASK)) {
                func(Id{gen, i});
            }
        }
    }

private:
    uint32_t *gens_ = nullptr;
    uint32_t capacity_ = 0;
    uint32_t count_ = 0;
};

class IndexIter {
    friend class Index;

    IndexIter(uint32_t *base, uint32_t capacity) : gen_base_{base}, gen_size_{capacity}, curr_{0} {}

public:
    using difference_type = int64_t;
    using value_type = Id;

    Id operator*() const noexcept {
        Id id{gen_, curr_};
        assert(!id.is_empty());
        return id;
    }

    IndexIter &operator++() noexcept {
        while ((++curr_) != gen_size_) {
            if (!(gen_base_[curr_] & Index::EMPTY_MASK)) {
                gen_ = gen_base_[curr_];
                break;
            }
        }
        return *this;
    }

    IndexIter operator++(int) noexcept {
        auto copy = *this;
        operator++();
        return copy;
    }

    IndexIter &operator--() noexcept {
        while (curr_-- != 0) {
            if (!(gen_base_[curr_] & Index::EMPTY_MASK)) {
                gen_ = gen_base_[curr_];
                break;
            }
        }
        return *this;
    }

    IndexIter operator--(int) noexcept {
        auto copy = *this;
        operator--();
        return copy;
    }

    friend bool operator==(const IndexIter &lhs, const IndexIterEnd &rhs) noexcept {
        return lhs.curr_ == lhs.gen_size_;
    }

    friend bool operator!=(const IndexIter &lhs, const IndexIterEnd &rhs) noexcept {
        return lhs.curr_ != lhs.gen_size_;
    }

    auto operator<=>(const IndexIter &rhs) noexcept {
        assert(gen_size_ == rhs.gen_size_ && gen_base_ == rhs.gen_base_);
        return curr_ <=> rhs.curr_;
    }

private:
    uint32_t *gen_base_ = nullptr;
    uint32_t gen_size_ = 0;
    uint32_t curr_ = 0;
    uint32_t gen_ = 0;
};

inline IndexIter Index::begin() const noexcept { return IndexIter{gens_, capacity_}; }

inline IndexIterEnd Index::end() const noexcept { return IndexIterEnd{}; }

static_assert(std::ranges::range<Index>);
}  // namespace tables::sparse
