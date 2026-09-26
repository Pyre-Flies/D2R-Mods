#include "custom_page_actions.h"
#include "custom_page_profile.h"
#include "core_compatibility.h"
#include "native_input_profile.h"
#include "native_d2r.h"
#include <atomic>
#include <cstdio>

namespace QolCustomPage {
namespace {
std::atomic<uintptr_t> core{};
struct Binding {
    uintptr_t owner[3]{};
    uint64_t page{};
    void* grid{};
    bool usable{};
};
struct Snapshot { void* player{}; uint32_t width{}, height{}; };
struct Cell { int32_t x, y; };
static_assert(offsetof(Binding,page)==0x18 && offsetof(Binding,usable)==0x28 && sizeof(Binding)==0x30);
static_assert(sizeof(Snapshot)==0x10);
template<class T> T At(uintptr_t base, uintptr_t rva) noexcept { return reinterpret_cast<T>(base+rva); }
bool Check(uintptr_t base,const QolCustomProfile::Site* sites,size_t count) noexcept {
    for(size_t i=0;i<count;++i)
        if(std::memcmp(reinterpret_cast<void*>(base+sites[i].rva),sites[i].bytes,sites[i].size)) return false;
    return true;
}
bool Guard(uintptr_t base, uintptr_t game) noexcept {
    __try {
        return Check(base,QolCustomProfile::Core,std::size(QolCustomProfile::Core)) &&
            Check(game,QolCustomProfile::Game,std::size(QolCustomProfile::Game)) &&
            *reinterpret_cast<uintptr_t*>(base+0x70E968)==game+0x2C49F0;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
void* InventoryGrid(const D2RL::PluginContext* ctx) noexcept {
    const auto panel=At<D2R::Native::FindTopLevelPanelByNameFn>(ctx->exeBase,0x846170);
    void* root=panel("PlayerInventoryExpansionLayout");
    if(!root) root=panel("PlayerInventoryOriginalLayout");
    if(!root) return nullptr;
    auto* bytes=static_cast<unsigned char*>(root);
    if(!(bytes[0x50]&bytes[0x51])) return nullptr;
    return At<D2R::Native::FindChildWidgetByNameFn>(ctx->exeBase,0x856220)(root,"grid");
}
bool Resolve(const D2RL::PluginContext* ctx,uintptr_t base,Binding& out,void*& inventory) noexcept {
    inventory=InventoryGrid(ctx);
    if(!inventory) return false;
    const auto kind=At<uint8_t(__fastcall*)(void*,Binding*)>(base,0x45C140)(inventory,&out);
    return kind==2 && out.usable && out.grid && out.page;
}
}
bool Initialize(const D2RL::PluginContext* ctx) noexcept {
    core.store(0);
    const auto module=GetModuleHandleW(L"D2RCore.dll");
    const auto base=reinterpret_cast<uintptr_t>(module);
    if(!QolCore::VerifyFileHash(module,QolNativeProfile::CoreHash) || !Guard(base,ctx->exeBase)) {
        ctx->LogWarn("[QOL/CustomPage] Native quick-transfer contract unavailable; custom-page shortcut disabled.");
        return false;
    }
    core.store(base);
    ctx->LogInfo("[QOL/CustomPage] Guarded loader quick-transfer bridge ready; no additional hooks or patches.");
    return true;
}
void Shutdown() noexcept { core.store(0); }
bool Visible(const D2RL::PluginContext* ctx) noexcept {
    const auto base=core.load();
    if(!base || !ctx) return false;
    __try {
        Binding binding{}; void* inventory{};
        return Resolve(ctx,base,binding,inventory);
    } __except(EXCEPTION_EXECUTE_HANDLER) { core.store(0); return false; }
}
bool TryTransfer(const D2RL::PluginContext* ctx,const D2RL::Items::ItemInfo& info) noexcept {
    const auto base=core.load();
    const bool fromCustom=info.container==D2RL::Items::ItemContainer::CustomPage;
    if(!base || !ctx) return fromCustom;
    bool claimed=fromCustom;
    __try {
        // Recheck before a mutation in case another plugin patched a callee after load.
        if(!Guard(base,ctx->exeBase)) {
            core.store(0);
            ctx->LogWarn("[QOL/CustomPage] Live contract changed; transfer refused and bridge disabled.");
            return true;
        }
        Binding binding{}; void* inventory{};
        if(!Resolve(ctx,base,binding,inventory)) return fromCustom;
        claimed=true;
        if(!fromCustom && info.container!=D2RL::Items::ItemContainer::Inventory) return true;
        if(info.x<0 || info.y<0 || info.inventoryPage!=(fromCustom?6:0)) return true;
        Snapshot snapshot{};
        if(At<uint32_t(__fastcall*)(const Binding*,uint64_t,Snapshot*)>(base,0x41F6C0)(&binding,binding.page,&snapshot)) return true;
        if(!At<bool(__fastcall*)(void*,void*)>(base,0x461B10)(inventory,snapshot.player)) return true;
        if(At<uint32_t(__fastcall*)(void*,const Snapshot*)>(base,0x45FA00)(binding.grid,&snapshot)) return true;
        void* source=fromCustom?binding.grid:inventory;
        const auto* bytes=static_cast<unsigned char*>(source);
        const auto width=fromCustom?snapshot.width:*reinterpret_cast<const uint32_t*>(bytes+0x628);
        const auto height=fromCustom?snapshot.height:*reinterpret_cast<const uint32_t*>(bytes+0x62C);
        if(static_cast<uint32_t>(info.x)>=width || static_cast<uint32_t>(info.y)>=height) return true;
        const Cell cell{info.x,info.y};
        void* item=At<void*(__fastcall*)(void*,const Cell*)>(ctx->exeBase,0x2C49F0)(source,&cell);
        if(!item || At<uint32_t(__fastcall*)(void*)>(ctx->exeBase,0x34A330)(item)!=info.runtimeId ||
            At<uint32_t(__fastcall*)(void*)>(ctx->exeBase,0x36EF50)(item)!=info.code) {
            ctx->LogWarn("[QOL/CustomPage] Focus identity no longer matches the source cell; transfer refused.");
            return true;
        }
        // Identical operation to D2RCore's Ctrl+click branch. Its registry ownership,
        // policy, free-space search and local/network routing remain authoritative.
        const auto result=At<uint32_t(__fastcall*)(const Binding*,uint64_t,bool,int32_t,int32_t)>(base,0x41FF40)
            (&binding,binding.page,fromCustom,info.x,info.y);
        char message[192];
        std::snprintf(message,sizeof(message),"[QOL/CustomPage] LB+X direction=%s page=%llu item=%u cell=%d,%d result=%u (request result, not persistence confirmation).",
            fromCustom?"to-inventory":"to-custom",static_cast<unsigned long long>(binding.page),info.runtimeId,info.x,info.y,result);
        ctx->LogInfo(message);
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        core.store(0);
        ctx->LogWarn("[QOL/CustomPage] Native access fault; bridge disabled for this session.");
        return claimed;
    }
}
}
