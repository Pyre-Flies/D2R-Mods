#pragma once
#include <climits>
#include <optional>
namespace BaseRanges {
// Match the intrinsic native base, including its staged ethereal truncation.
inline std::optional<int> Intrinsic(int base,bool ethereal) {
    if (base<0) return {};
    const long long value=ethereal?static_cast<long long>(base)*3/2:base;
    if (value>INT_MAX) return {};
    return static_cast<int>(value);
}
}
