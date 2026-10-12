#pragma once
#include <climits>
#include <optional>
namespace BaseRanges {
// Low-quality armor keeps a random base roll, reduced to 75% with truncation.
inline std::optional<int> LowArmor(int base) {
    if (base<0) return {};
    return static_cast<int>(static_cast<long long>(base)*3/4);
}
inline std::optional<int> LowWeapon(int base) {
    const auto reduced=LowArmor(base);
    if (!reduced) return {};
    return *reduced<1?1:*reduced;
}
// Match the intrinsic native base, including its staged ethereal truncation.
inline std::optional<int> Intrinsic(int base,bool ethereal) {
    if (base<0) return {};
    const long long value=ethereal?static_cast<long long>(base)*3/2:base;
    if (value>INT_MAX) return {};
    return static_cast<int>(value);
}
}
