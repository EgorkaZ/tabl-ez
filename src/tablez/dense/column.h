#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

namespace tablez::dense {

template <class T>
class ThinVector {
public:
    template <class U>
        requires(std::is_constructible_v<T, U &&>)
    void insert_at(uint32_t idx, U&& arg) noexcept(std::is_nothrow_constructible_v<T, U&&>) {
        new (data_ + idx) T(std::forward<U>(arg));
    }

    void remove_at(uint32_t idx,
                   uint32_t last) noexcept(std::is_nothrow_destructible_v<T> && std::is_nothrow_move_assignable_v<T>) {
        data_[idx] = std::move(data_[last]);
        if constexpr (!std::is_trivially_destructible_v<T>) {
            data_[last].~T();
        }
    }

private:
    T* data_;
};
}  // namespace tablez::dense
