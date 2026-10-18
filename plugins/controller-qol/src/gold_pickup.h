#include "item_code_compatibility.h"
#pragma once
#include "gold_pickup_profile.h"
#include "client_loot_policy.h"
#include <D2RLPlugin/api.h>
#include <windows.h>
namespace QolGoldPickup {
// Called only inside the native server pickup callback with its real actor.
// Follow that entry's adjacent-room enumeration; never borrow client/UI units.
inline bool IsGold(const D2RL::PluginContext* context,void* player,uint32_t guid) noexcept {
    if(!context || !player || !guid)return false;
    for(const auto& s:Sites)if(!QolItemCode::Check(context,s.rva,s.bytes,s.size))return false;
    __try {
        const auto base=context->exeBase;
        auto room=reinterpret_cast<void*(__fastcall*)(void*)>(base+0x34b440)(player);
        if(!room)return false;
        void** rooms{};unsigned count{};
        reinterpret_cast<void(__fastcall*)(void*,void***,unsigned*)>(base+0x2efde0)(room,&rooms,&count);
        if(!rooms || !count || count>256)return false;
        unsigned budget=8192;
        for(unsigned i=0;i<count && budget;++i) {
            if(!rooms[i])continue;
            auto unit=reinterpret_cast<void*(__fastcall*)(void*)>(base+0x2efd90)(rooms[i]);
            while(unit && budget) {
                --budget;
                const auto words=static_cast<const uint32_t*>(unit);
                if(words[0]==4 && words[2]==guid && words[3]==3) {
                    const auto code=reinterpret_cast<uint32_t(__fastcall*)(void*)>(base+0x36ef50)(unit);
                    return QolClientLoot::IsGoldCode(code);
                }
                unit=reinterpret_cast<void*(__fastcall*)(void*)>(base+0x34b4a0)(unit);
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
    return false;
}
}
