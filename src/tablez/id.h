#pragma once

#include <cassert>
#include <cstdint>

namespace tablez {

class Id {
public:
    Id() noexcept = default;

    Id(uint32_t gen, uint32_t idx) noexcept
        : value_{(uint64_t{gen} << 32) | idx}
    {}

    uint32_t idx() const noexcept {
        return static_cast<uint32_t>(value_);
    }

    uint32_t gen() const noexcept {
        return static_cast<uint32_t>(value_ >> 32);
    }

    bool is_empty() const noexcept {
        constexpr uint64_t EMPTY_MASK = uint64_t{1} << 32;
        return value_ & EMPTY_MASK;
    }
private:
    uint64_t value_;
};

template <class T>
class TaggedId : Id {};

}
