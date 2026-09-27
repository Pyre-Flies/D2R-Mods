#include "vendor_buy.h"
#include "vendor_policy.h"
#include "vendor_compatibility.h"
#include "custom_page_profile.h"
#include "belt_compatibility.h"
#include "native_d2r.h"
namespace QolVendorBuy {
namespace {
bool Validate(const D2RL::PluginContext* ctx) noexcept {
    if (!ctx || !ctx->exeBase) return false;
    for (const auto& s: QolVendor::Sites)
        if (!(s.rva==0x10D160 ? QolVendor::ValidateTransaction(ctx->exeBase) :
            QolBeltCompat::Match(ctx->exeBase+s.rva,s.bytes,s.size))) return false;
    for (const auto& s: QolCustomProfile::Game)
        if (!QolBeltCompat::Match(ctx->exeBase+s.rva,s.bytes,s.size)) return false;
    return true;
}
void* Stock(uintptr_t base, const D2RL::Items::ItemInfo& info, void*& panel) noexcept {
    panel=reinterpret_cast<void*(__fastcall*)(int)>(base+0x846190)(0x0B);
    if (!panel || info.x<0 || info.y<0 || info.x>63 || info.y>63) return nullptr;
    const auto visible=[](void* p) noexcept {
        const auto* b=static_cast<const unsigned char*>(p);
        return b && b[0x50]==1 && b[0x51]==1;
    };
    if (!visible(panel)) return nullptr;
    void* grid=reinterpret_cast<void*(__fastcall*)(void*,const char*)>(base+0x856220)(panel,"grid");
    if (!visible(grid)) return nullptr;
    const int32_t cell[]={info.x,info.y};
    void* item=reinterpret_cast<void*(__fastcall*)(void*,const int32_t*)>(base+0x2C49F0)(grid,cell);
    if (!item || reinterpret_cast<uint32_t(__fastcall*)(void*)>(base+0x34A330)(item)!=info.runtimeId ||
        reinterpret_cast<uint32_t(__fastcall*)(void*)>(base+0x36EF50)(item)!=info.code) return nullptr;
    return item;
}
}
bool Check(const D2RL::PluginContext* ctx, const D2RL::Items::ItemInfo& info) noexcept {
    if (!Validate(ctx)) return false;
    __try { void* panel{}; return Stock(ctx->exeBase,info,panel)!=nullptr; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
void Submit(const D2RL::PluginContext* ctx, const D2RL::Items::ItemInfo& info) noexcept {
    if (!Validate(ctx)) { ctx->LogWarn("[QOL/VendorBelt] Native purchase profile unavailable; no purchase."); return; }
    __try {
        const auto base=ctx->exeBase;
        void* panel{};
        void* item=Stock(base,info,panel);
        void* player=D2R::Native::GetLocalPlayerUnit(base);
        if (!item || !player) { ctx->LogInfo("[QOL/VendorBelt] Stock changed; purchase cancelled."); return; }
        void* inv=reinterpret_cast<D2R::Native::GetUnitInventoryFn>(base+D2R::Native::GetUnitInventoryRva)(player);
        int32_t slot=-1;
        if (!inv || !reinterpret_cast<D2R::Native::GetFreeBeltSlotFn>(base+D2R::Native::GetFreeBeltSlotRva)(inv,item,&slot,true) || slot<0 || slot>=16) {
            ctx->LogInfo("[QOL/VendorBelt] No compatible belt space; no purchase."); return;
        }
        QolVendor::SubmitBuy(reinterpret_cast<QolVendor::SellFn>(base+0x23FED0),panel,player,item);
        ctx->LogInfo("[QOL/VendorBelt] Native Shift-buy submitted once; game owns price, stock and placement. No automatic retry.");
    } __except(EXCEPTION_EXECUTE_HANDLER) { ctx->LogWarn("[QOL/VendorBelt] Native purchase fault; no retry."); }
}
}
