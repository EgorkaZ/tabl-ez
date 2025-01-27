#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>
#include <utility>

namespace tablez::dense {

template <class T>
class ThinVector {
    using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;
public:
    template <class U>
        requires(std::is_constructible_v<T, U &&>)
    void insert_at(uint32_t idx, U&& arg) noexcept(std::is_nothrow_constructible_v<T, U&&>) {
        new (data_ + idx) T(std::forward<U>(arg));
    }

    void remove_at(uint32_t idx,
                   uint32_t last) noexcept(std::is_nothrow_destructible_v<T> && std::is_nothrow_move_assignable_v<T>) {
        assert(idx <= last);

        data_[idx] = std::move(data_[last]);
        if constexpr (!std::is_trivially_destructible_v<T>) {
            get_unchecked(last).~T();
        }
    }

    void realloc(uint32_t new_capacity, uint32_t count) {
        assert(count < new_capacity);

        Storage * new_data = new Storage[new_capacity];
        if constexpr (std::is_trivially_copyable_v<T>) {
            static_assert(std::is_trivially_destructible_v<T>);
            memcpy(new_data, data_, sizeof(Storage) * count);
        } else {
            for (uint32_t i = 0; i < count; ++i) {
                auto &elem = get_unchecked(i);
                new (new_data + i) T(std::move(elem));
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    elem.~T();
                }
            }
        }
        delete[] data_;
        data_ = new_data;
    }

    void destroy(uint32_t count) noexcept {
        if constexpr (std::is_trivially_destructible_v<T>) {
            return;
        } else {
            for (uint32_t i = 0; i < count; ++i) {
                get_unchecked(i).~T();
            }
        }
    }

    void dealloc() noexcept {
        delete[] data_;
        data_ = nullptr;
    }

    std::span<T> span(uint32_t count) const noexcept {
        return std::span{data_, count};
    }

    T &get_unchecked(uint32_t idx) const noexcept {
        return reinterpret_cast<T&>(data_[idx]);
    }

private:
    Storage* data_ = nullptr;
};
}  // namespace tablez::dense
