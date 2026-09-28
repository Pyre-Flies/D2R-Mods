#pragma once
#include <D2RLPlugin/item.h>
#include <cstdint>
namespace QolMaterials {
constexpr bool AdvancedProxyCandidate(D2RL::Items::ItemContainer source) noexcept {
    using C=D2RL::Items::ItemContainer;
    return source==C::Cursor || source==C::SharedStash || source==C::CustomPage;
}
constexpr bool UseMaterialsRoute(uint32_t tab, D2RL::Items::ItemContainer source) noexcept {
    return (tab==2 || tab==3 || tab==4) &&
        source==D2RL::Items::ItemContainer::Inventory;
}
constexpr bool UseStashDeposit(D2RL::Items::ItemContainer source, D2RL::Items::ItemContainer destination, bool stashOpen) noexcept {
    using C=D2RL::Items::ItemContainer;
    return stashOpen && source==C::Inventory && (destination==C::PersonalStash || destination==C::SharedStash);
}
enum class Destination : uint8_t { Inventory = 1, Belt = 3 };
constexpr const char* Category(uint32_t tab) noexcept {
    return tab == 2 ? "advancedstash_gems" : tab == 3 ? "advancedstash_materials" :
           tab == 4 ? "advancedstash_runes" : nullptr;
}
constexpr uint32_t RejuvenationCode(unsigned index) noexcept {
    return index == 0 ? D2RL::Items::MakeItemCode("rvl") : D2RL::Items::MakeItemCode("rvs");
}
constexpr uint32_t RefillCode(uint32_t selected, unsigned family) noexcept {
    return selected ? selected : RejuvenationCode(family);
}
constexpr bool RefillFallback(uint32_t selected, unsigned family) noexcept {
    return selected==0 && family==0;
}
constexpr bool Confirmed(int64_t before, int64_t after) noexcept { return after > before; }
constexpr bool CanContinue(unsigned confirmed) noexcept { return confirmed < 16; }
inline bool WidgetName(uint32_t code, char (&name)[5]) noexcept {
    unsigned length=0;
    for (unsigned i=0; i<4; ++i) {
        const auto c=static_cast<unsigned char>(code >> (8*i));
        if (c == ' ' || c == 0) break;
        if (!((c>='a' && c<='z') || (c>='0' && c<='9'))) return false;
        name[length++]=static_cast<char>(c);
    }
    name[length]=0;
    return length!=0;
}
}
