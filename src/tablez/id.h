#pragma once

#include <cassert>
#include <cstdint>

namespace tablez {

class Id {
    static constexpr uint64_t EMPTY_MASK = uint64_t{1} << 32;
public:
    static constexpr uint32_t EMPTY_GEN = 1;

    Id() noexcept = default;

    Id(uint32_t gen, uint32_t idx) noexcept
        : value_{(uint64_t{gen} << 32) | idx}
    {}

    static Id make_empty(uint32_t idx) noexcept {
        return Id{EMPTY_MASK >> 32, idx};
    }

    uint32_t idx() const noexcept {
        return static_cast<uint32_t>(value_);
    }

    uint32_t gen() const noexcept {
        return static_cast<uint32_t>(value_ >> 32);
    }

    Id &make_gen_valid() noexcept {
        assert(is_empty());
        value_ += EMPTY_MASK;
        return *this;
    }

    Id &make_gen_invalid() noexcept {
        assert(!is_empty());
        value_ += EMPTY_MASK;
        return *this;
    }

    bool is_empty() const noexcept {
        return value_ & EMPTY_MASK;
    }
private:
    uint64_t value_;
};

template <class T>
class TaggedId : Id {};

}
