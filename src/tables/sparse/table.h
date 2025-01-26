#pragma once

#include <memory>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

#include <tables/id.h>
#include "blob.h"
#include "index.h"

namespace tables::sparse {

namespace detail {

template <class T, class... Ts>
struct IsUniqueAmongImpl {
    // template <class U>
    // constexpr static uint32_t is_same

    constexpr static bool Value = (uint32_t{std::is_same_v<T, Ts>} + ...) == 1;
};

template <class T, class...Ts>
static constexpr bool IsUniqueAmong = IsUniqueAmongImpl<T, Ts...>::Value;
}

template <class T>
class Column {
public:
    Column(const Index &index, const Blob<T> &data)
        : index_{index}
        , data_{data}
    {}

    template <class Func>
    void for_each(Func &&func) noexcept(std::is_nothrow_invocable_r_v<void, Func, Id, T&>) {
        index_.for_each([this, &func](Id id) {
            func(id, data_.assume_init_at(id.idx()));
        });
    }

    auto range() noexcept {
        return index_ | std::ranges::views::transform([data = data_](Id id) {
            return std::pair<Id, T&>(id, data.assume_init_at(id.idx()));
        });
    }

    void destroy() noexcept(std::is_nothrow_destructible_v<T>) {
        for_each([](Id, T &value) {
            value.~T();
        });
    }
private:
    Index index_;
    Blob<T> data_;
};

template <class... Ts>
class Table {
public:
    constexpr Table() noexcept = default;

    Table(Table &&rhs) noexcept
        : index_(rhs.index_)
        , free_{std::move(rhs.free_)}
        , columns_{rhs.columns_}
    {
        rhs.index_ = {};
        rhs.columns_ = {};
    }

    Table &operator=(Table &&rhs) noexcept {
        if (this == &rhs) {
            return *this;
        }
        destroy();
        index_ = std::exchange(rhs.index_, Index{});
        free_ = std::move(rhs.free_);
        (..., (raw_column<Ts>() = std::exchange(rhs.raw_column<Ts>(), Blob<Ts>{})));
    }

    ~Table() noexcept {
        destroy();
    }

    static Table with_capacity(uint32_t capacity) {
        return Table(capacity);
    }

    template <class T> requires(detail::IsUniqueAmong<T, Ts...>)
    Column<T> column() noexcept {
        return Column<T>{index_, raw_column<T>()};
    }

    template <class... Us> requires(std::is_constructible_v<Ts, Us&&> && ...)
    Id insert(Us &&...args) {
        reserve_at_least(index_.count() + 1);

        auto id = pop_free_index();
        (...,raw_column<Ts>().init_at(id.idx(), std::forward<Us>(args)));
        return id;
    }

    Id remove(Id id) noexcept {
        (...,raw_column<Ts>().destroy_at(id.idx()));
        return push_free_index(id);
    }

    template <class Func> requires(std::is_invocable_r_v<void, Func, Id, Ts&...>)
    void for_each_row(Func &&func) noexcept(std::is_nothrow_invocable_v<Func, Id, Ts&...>) {
        for (uint32_t i = 0; i < index_.capacity(); ++i) {
            if (index_.is_set(i)) {
                func(index_.get_unchecked(i), raw_column<Ts>().assume_init_at(i)...);
            }
        }
    }

    void destroy() noexcept {
        (column<Ts>().destroy(),...);
        (raw_column<Ts>().dealloc(),...);
        free_.reset();
        index_.dealloc();
    }

    uint32_t count() const noexcept {
        return index_.count();
    }

    uint32_t capacity() const noexcept {
        return index_.capacity();
    }

    void reserve_at_least(uint32_t new_capacity) {
        if (new_capacity <= capacity()) {
            return;
        }
        new_capacity = std::max(capacity() * 2, new_capacity);

        auto old_capacity = index_.capacity();
        index_.reserve_for(new_capacity);
        (..., raw_column<Ts>().grow_for_capacity(old_capacity, index_.capacity(), [this](uint32_t idx) {
            return index_.is_set(idx);
        }));

        std::unique_ptr<uint32_t[]> new_free{new uint32_t[new_capacity]};
        for (uint32_t i = 0; i < old_capacity; ++i) {
            new_free[i] = free_[i];
        }
        for (uint32_t i = old_capacity; i < new_capacity; ++i) {
            new_free[i] = i;
        }
        free_ = std::move(new_free);
    }
private:
    explicit Table(uint32_t capacity)
        : index_{Index::with_capacity(capacity)}
        , free_{new uint32_t[capacity]}
        , columns_(Blob<Ts>::with_capacity(capacity)...)
    {
        for (uint32_t i = 0; i < capacity; ++i) {
            free_[i] = i;
        }
    }

    Id pop_free_index() noexcept {
        assert(index_.count() < index_.capacity());
        uint32_t stack_top = index_.count();
        uint32_t idx = free_[stack_top];
        return index_.set_unchecked(idx); // increases index_.count(), moves stack_top right
    }

    Id push_free_index(Id id) noexcept {
        uint32_t stack_end = index_.count() - 1;
        free_[stack_end] = id.idx();
        return index_.reset_unchecked(id); // moves stack_end left, thus, stack_top is previous stack_end
    }

    template <class T> requires(detail::IsUniqueAmong<T, Ts...>)
    Blob<T> &raw_column() noexcept {
        return std::get<Blob<T>>(columns_);
    }
private:
    Index index_;
    std::unique_ptr<uint32_t[]> free_; // acts as a stack of free indicies
    std::tuple<Blob<Ts>...> columns_;
};
}  // namespace tables
