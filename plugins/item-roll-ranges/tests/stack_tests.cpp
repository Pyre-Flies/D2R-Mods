#include <intrin.h>
#include <cstring>
#include <cstdint>
#include <cstdio>
extern "C" int RangeStackFixture(char*);
extern "C" unsigned char RangeStackReturn;
extern "C" __declspec(noinline) int __fastcall ProbeRangeEntry(void* context,void* list,int stat,int layer,
    int minimum,int maximum,char* out,int mode) noexcept {
    if (_ReturnAddress()!=&RangeStackReturn || reinterpret_cast<std::uintptr_t>(context)!=0x1234 ||
        reinterpret_cast<std::uintptr_t>(list)!=0x5678 || stat!=31 || layer!=9 || minimum!=80 || maximum!=120 || mode!=4) return -1;
    int actual{};
    std::memcpy(&actual,static_cast<const unsigned char*>(_AddressOfReturnAddress())+0x4c,sizeof(actual));
    std::snprintf(out,256,"[%+d - %+d] %+d Defense",minimum,maximum,actual);
    return actual;
}
int main() {
    char line[256]{};
    if (RangeStackFixture(line)!=118 || std::strcmp(line,"[+80 - +120] +118 Defense")) return 1;
    std::puts("Verified actual-value stack offset and eight-argument ABI through entry JMP detour.");
}
