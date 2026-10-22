#pragma once
#include "policy.h"
#include <string>
namespace GemBag::Native {
struct Snapshot {
    Info bag{};
    uint32_t credits{};
    std::optional<Cell> cell;
    unsigned bank{},width{},height{};
    uint64_t definition{};
    unsigned gemMask{};
};
bool Initialize(const D2RL::PluginContext*);
bool Inspect(const D2RL::PluginContext*,const D2RL::ItemService*,D2RL::ItemHandle,const Info&,Snapshot&,std::string&);
bool Execute(const D2RL::PluginContext*,const D2RL::ItemService*,D2RL::PlayerHandle,const Info&,unsigned,Snapshot&,std::string&,bool&);
bool Deposit(const D2RL::PluginContext*,const D2RL::ItemService*,const D2RL::InventoryService*,D2RL::PlayerHandle,const Info&,Snapshot&,std::string&,bool&);
bool Focus(const D2RL::PluginContext*,uint32_t&,uint32_t&) noexcept;
bool Controller(const D2RL::PluginContext*) noexcept;
bool CursorFree(const D2RL::PluginContext*) noexcept;
bool SafeUi(const D2RL::PluginContext*) noexcept;
#ifdef GEMBAG_NATIVE_TESTS
namespace Testing {
using Setter=bool(__fastcall*)(uint8_t,void*,int32_t,int32_t,uint32_t);
using Publisher=bool(*)(const D2RL::PluginContext*,const D2RL::ItemService*,const Info&,bool);
void Configure(uintptr_t,Setter,Publisher);
}
#endif
}
