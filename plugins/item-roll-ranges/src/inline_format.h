#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>

namespace InlineRanges {
inline constexpr std::size_t Capacity = 256;
// Properties.uiRangeType: zero is the ordinary table default; four is the native fallback.
inline constexpr bool ScalarMode(int mode) noexcept { return mode==0 || mode==4; }
struct OptionalValue { std::int32_t value{}; bool present{}; unsigned char pad[3]{}; };
static_assert(sizeof(OptionalValue) == 8);
using BankFn = unsigned char(__fastcall*)(void*);
using RowFn = const unsigned char*(__fastcall*)(unsigned char, int);
using NormalizeFn = int(__fastcall*)(void*, int, int, const void*, OptionalValue*, bool);
using GroupFn = int(__fastcall*)(void*, void*, int, int, const void*, int*);
using SingleFn = bool(__fastcall*)(void*, const void*, int, int, bool, char*, int);
struct Functions { BankFn bank{}; RowFn row{}; NormalizeFn normalize{}; GroupFn group{}; SingleFn single{}; };

// Numeric transformations witnessed in the native descfunc dispatch. The
// localized actual line is always produced by the native single-value formatter.
inline bool DisplayBounds(unsigned function, int& lo, int& hi) noexcept {
    std::int64_t a=lo, b=hi;
    switch (function) {
    case 1: case 2: case 3: case 4: case 6: case 7: case 8: case 9: case 12: case 19: break;
    case 5: case 10: a=a*100/128; b=b*100/128; break;
    case 20: case 21: a=-static_cast<std::int64_t>(hi); b=-static_cast<std::int64_t>(lo); break;
    case 29:
        a=lo<0?-static_cast<std::int64_t>(lo):lo;
        b=hi<0?-static_cast<std::int64_t>(hi):hi;
        if (a>b) { auto tmp=a; a=b; b=tmp; }
        if (lo<=0 && hi>=0) a=0;
        break;
    default: return false;
    }
    if (a<std::numeric_limits<int>::min() || b>std::numeric_limits<int>::max() || a>=b) return false;
    lo=static_cast<int>(a); hi=static_cast<int>(b); return true;
}
inline bool Prefix(char* line, int lo, int hi) noexcept {
    const auto length = strnlen(line, Capacity);
    if (!length || length==Capacity) return false;
    char prefix[64]{};
    const int n=std::snprintf(prefix,sizeof(prefix),"[%+d - %+d] ",lo,hi);
    if (n<=0 || static_cast<std::size_t>(n)+length>=Capacity) return false;
    std::memmove(line+n,line,length+1);
    std::memcpy(line,prefix,static_cast<std::size_t>(n));
    return true;
}
// Returns -1 for an unusable row so the adapter can delegate; 0 preserves
// native suppression, and 1 supplies a native actual line (with a safe prefix
// when supported). No item/definition memory is mutated.
inline int Format(const Functions& f, void* context, void* statList, int stat, int layer,
                  int minimum, int maximum, int actualRaw, char* output, int mode) noexcept {
    const auto row=f.row(f.bank(context),stat);
    if (!row) return -1;
    OptionalValue actualOption{};
    const int actual=f.normalize(context,stat,actualRaw,row,&actualOption,false);
    int emit=1;
    const bool grouped=f.group(context,statList,stat,actual,row,&emit)!=0;
    if (grouped && !emit) { output[0]=0; return 0; }
    char line[Capacity]{};
    if (!f.single(context,row,actual,layer,grouped,line,mode)) { output[0]=0; return 0; }
    if (strnlen(line,Capacity)==Capacity) return -1;
    // Grouped bounds can represent multiple source stats. Keep the actual
    // grouped line until each constituent range is independently verified.
    if (!grouped && ScalarMode(mode) && minimum<maximum) {
        OptionalValue lowOption{}, highOption{};
        int lo=f.normalize(context,stat,minimum,row,&lowOption,true);
        int hi=f.normalize(context,stat,maximum,row,&highOption,true);
        if (DisplayBounds(row[0x32],lo,hi)) Prefix(line,lo,hi);
    }
    std::memcpy(output,line,std::strlen(line)+1);
    return 1;
}
}
