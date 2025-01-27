#pragma once

#include <cassert>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace tablez::sparse {

// owns nothing, doesn't know it's size
template <class T>
class Blob {
    using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;

    explicit Blob(Storage *data) : data_{data} {}

public:
    constexpr Blob() noexcept = default;

    static Blob with_capacity(uint32_t capacity) {
        Storage *ptr = new Storage[capacity];
        return Blob{ptr};
    }

    Storage &raw_at(uint32_t idx) const noexcept { return data_[idx]; }

    T &assume_init_at(uint32_t idx) const noexcept { return reinterpret_cast<T &>(data_[idx]); }

    template <class... Args>
    T &init_at(uint32_t idx, Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args &&...>) {
        return *(new (data_ + idx) T(std::forward<Args>(args)...));
    }

    void destroy_at(uint32_t idx) noexcept(std::is_nothrow_destructible_v<T>) {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            assume_init_at(idx).~T();
        }
    }

    // will make space for at least new_capacity elements,
    // will move existing elements into new storage, preserving their places
    template <class IsInit>
        requires std::is_invocable_r_v<bool, IsInit, uint32_t>
    void grow_for_capacity(uint32_t old_capacity, uint32_t new_capacity,
                           IsInit is_init) noexcept(std::is_nothrow_move_constructible_v<T> &&
                                                    std::is_nothrow_destructible_v<T>) {
        assert(old_capacity <= new_capacity);
        auto dst = new Storage[new_capacity];

        for (uint32_t i = 0; i < old_capacity; ++i) {
            if (is_init(i)) {
                auto &val = assume_init_at(i);
                new (dst + i) T(std::move(val));
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    val.~T();
                }
            }
        }
        dealloc();
        data_ = dst;
    }

    template <class IsInit>
        requires std::is_invocable_r_v<bool, IsInit, uint32_t>
    void destroy(uint32_t capacity, IsInit is_init) noexcept(std::is_nothrow_destructible_v<T>) {
        if constexpr (std::is_trivially_destructible_v<T>) {
            return;
        }
        while (capacity-- != 0) {
            auto i = capacity;
            if (is_init(i)) {
                auto &val = assume_init_at(i);
                val.~T();
            }
        }
    }

    void dealloc() noexcept {
        if (data_) {
            delete[] data_;
            data_ = nullptr;
        }
    }

private:
    Storage *data_ = nullptr;
};
}  // namespace tablez
