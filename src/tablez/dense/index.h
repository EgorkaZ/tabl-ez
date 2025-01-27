#pragma once

#include <tablez/id.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>

namespace tablez::dense {

class Index {
    // looks like Id, but actually index into ids_ storage
    //   with generation stored
    struct GenIdx {
        uint32_t gen = Id::EMPTY_GEN;
        uint32_t idx;
    };

public:
    void reserve_at_least(uint32_t new_capacity) {
        if (new_capacity <= capacity_) {
            return;
        }

        replace_index(new_capacity);
        replace_ids(new_capacity);

        capacity_ = new_capacity;
    }

    int64_t try_remove(Id id) {
        if (count_ == 0) {
            return -1;
        }

        assert(id.idx() < capacity_);
        assert(!id.is_empty());
        assert(count_ > 0);
        auto &at = index_[id.idx()];
        if (at.gen != id.gen()) {
            return -1;
        }
        at.gen += Id::EMPTY_GEN;  // set as invalid, same as it's gonna be in Id
        return free_id(at);
    }

    Id push() noexcept {
        assert(has_space());
        auto id = acquire_id();
        index_[id.idx()] = {.gen = id.gen(), .idx = count_ - 1};
        return id;
    }

    Id push_realloc() {
        reserve_at_least(count_ + 1);
        return push();
    }

    const Id *begin() const noexcept {
        return ids_;
    }

    const Id *end() const noexcept {
        return ids_ + count_;
    }

    std::span<const Id> span() const noexcept {
        return {begin(), end()};
    }

    uint32_t get_idx_unchecked(Id id) const noexcept {
        assert(id.idx() < capacity_);
        auto idx = index_[id.idx()];

        assert(idx.gen == id.gen());
        return idx.idx;
    }

    bool try_get_idx(Id id, uint32_t &res) const noexcept {
        assert(id.idx() < capacity_);
        auto at = index_[id.idx()];
        if (at.gen != id.gen()) {
            return false;
        }
        res = at.idx;
        return true;
    }

    std::optional<uint32_t> try_get_idx(Id id) const noexcept {
        assert(id.idx() < capacity_);
        auto at = index_[id.idx()];

        std::optional<uint32_t> res;
        if (at.gen == id.gen()) {
            res = at.idx;
        }
        return res;
    }

    uint32_t count() const noexcept {
        return count_;
    }

    uint32_t capacity() const noexcept {
        return capacity_;
    }

    bool has_space() const noexcept {
        return count_ < capacity_;
    }

    void destroy() noexcept {
        for (uint32_t i = 0; i < count_; ++i) {
            Id &id = ids_[i];
            if (!id.is_empty()) {
                id.make_gen_invalid();
                index_[id.idx()].gen = id.gen();
            }
        }
        count_ = 0;
    }

    void dealloc() noexcept {
        delete[] index_;
        delete[] ids_;
        capacity_ = 0;
        count_ = 0;
        index_ = nullptr;
        ids_ = nullptr;
    }

    Id get_id_by_idx(uint32_t idx) const noexcept {
        assert(idx < count());
        return ids_[idx];
    }

private:
    Id acquire_id() noexcept {
        assert(count_ < capacity_);
        auto &id = ids_[count_++];
        id.make_gen_valid();
        return id;
    }

    uint32_t free_id(GenIdx at) noexcept {
        // make index for last Id point into place where it's going to get swapped into
        //  generation stays the same, thus no need to change
        auto &last_id = ids_[--count_];
        index_[last_id.idx()].idx = at.idx;

        // remove Id by invalidating it and swapping it with the last
        ids_[at.idx].make_gen_invalid();
        std::swap(ids_[at.idx], last_id);
        return at.idx;
    }

    void replace_index(uint32_t new_capacity) {
        assert(new_capacity > capacity_);
        auto new_index = new GenIdx[new_capacity];
        for (uint32_t i = 0; i < capacity_; ++i) {
            new_index[i] = index_[i];
        }
        for (uint32_t i = capacity_; i < new_capacity; ++i) {
            new_index[i] = {
                .gen = Id::EMPTY_GEN,
                .idx = i,
            };
        }
        delete[] index_;
        index_ = new_index;
    }

    void replace_ids(uint32_t new_capacity) {
        assert(new_capacity > capacity_);
        auto new_ids = new Id[new_capacity];
        for (uint32_t i = 0; i < capacity_; ++i) {
            new_ids[i] = ids_[i];
        }
        for (uint32_t i = capacity_; i < new_capacity; ++i) {
            new_ids[i] = Id::make_empty(i);
        }
        delete[] ids_;
        ids_ = new_ids;
    }

private:
    uint32_t capacity_ = 0;
    uint32_t count_ = 0;
    GenIdx *index_ = nullptr;  // point to actual places of elements
    Id *ids_ = nullptr;        // before count_: store Id of element, after count_: store free Ids
};
}  // namespace tablez::dense
