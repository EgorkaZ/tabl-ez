#pragma once

#include <cstdint>
#include <type_traits>

namespace tablez {

template <class T, class... Ts>
struct IsUniqueAmongImpl {
    constexpr static bool Value = (uint32_t{std::is_same_v<T, Ts>} + ...) == 1;
};

template <class T, class... Ts>
static constexpr bool IsUniqueAmong = IsUniqueAmongImpl<T, Ts...>::Value;
}  // namespace tablez