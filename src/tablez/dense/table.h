#pragma once

#include <ranges>
#include <type_traits>

#include "index.h"
#include "tablez/util.h"
#include "thin_vector.h"

namespace tablez::dense {

template <class... Ts>
class Table {
public:
    static Table with_capacity(uint32_t capacity) {
        Table table;
        table.reserve_at_least(capacity);
        return table;
    }

    constexpr Table() noexcept = default;

    Table(Table &&rhs) noexcept : index_(rhs.index_), columns_(rhs.columns_) {
        rhs.index_ = {};
        rhs.columns_ = {};
    }

    Table &operator=(Table &&rhs) noexcept {
        if (this == &rhs) {
            return *this;
        }
        destroy();
        dealloc();
        index_ = std::exchange(rhs.index_, Index{});
        (..., (raw_column<Ts>() = std::exchange(rhs.raw_column<Ts>(), ThinVector<Ts>{})));
        return *this;
    }

    ~Table() noexcept {
        destroy();
        dealloc();
    }

    template <class... Us>
        requires(std::is_constructible_v<Ts, Us &&> && ...)
    Id insert(Us &&...args) noexcept((std::is_nothrow_constructible_v<Ts, Us> && ...)) {
        reserve_at_least(count() + 1);
        auto last = count();
        Id id = index_.push();
        (..., raw_column<Ts>().insert_at(last, std::forward<Us>(args)));
        return id;
    }

    bool remove(Id id) noexcept(((std::is_nothrow_destructible_v<Ts> && std::is_nothrow_move_assignable_v<Ts>) &&
                                 ...)) {
        int64_t replaced_idx = index_.try_remove(id);
        if (replaced_idx < 0) {
            return false;
        }

        (..., raw_column<Ts>().remove_at(replaced_idx, index_.count()));
        return true;
    }

    void reserve_at_least(uint32_t new_capacity) {
        if (new_capacity < capacity()) {
            return;
        }
        new_capacity = std::max(new_capacity, capacity() * 2);
        uint32_t old_capacity = capacity();
        index_.reserve_at_least(new_capacity);
        (..., raw_column<Ts>().realloc(new_capacity, count()));
    }

    uint32_t count() const noexcept { return index_.count(); }

    uint32_t capacity() const noexcept { return index_.capacity(); }

    template <class T>
        requires(IsUniqueAmong<T, Ts...>)
    auto column() const noexcept {
        return std::ranges::views::iota(uint32_t{0}, count()) | std::ranges::views::transform([this](uint32_t idx) {
                   return std::pair<Id, T &>(index_.get_id_by_idx(idx), raw_column<T>().get_unchecked(idx));
               });
    }

    template <class T, class Func>
        requires(std::is_invocable_r_v<void, Func, Id, T &>)
    void for_each(Func &&func) noexcept(std::is_nothrow_invocable_v<Func, Id, T &>) {
        auto &col = raw_column<T>();
        for (uint32_t i = 0; i < count(); ++i) {
            func(index_.get_id_by_idx(i), col.get_unchecked(i));
        }
    }

private:
    void destroy() {
        (..., raw_column<Ts>().destroy(index_.count()));
        index_.destroy();
    }

    void dealloc() {
        index_.dealloc();
        (..., raw_column<Ts>().dealloc());
    }

    template <class T>
        requires(IsUniqueAmong<T, Ts...>)
    ThinVector<T> &raw_column() noexcept {
        return std::get<ThinVector<T>>(columns_);
    }

    template <class T>
        requires(IsUniqueAmong<T, Ts...>)
    const ThinVector<T> &raw_column() const noexcept {
        return std::get<ThinVector<T>>(columns_);
    }

private:
    Index index_;
    std::tuple<ThinVector<Ts>...> columns_;
};
}  // namespace tablez::dense
