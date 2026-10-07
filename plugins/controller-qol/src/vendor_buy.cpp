#include "plugin_coexistence.h"
#include "vendor_buy.h"
#include "vendor_policy.h"
#include "vendor_compatibility.h"
#include "custom_page_profile.h"
#include "belt_compatibility.h"
#include "native_d2r.h"
#include "vendor_tome_profile.h"
#include "identify_stat_profile.h"
#include "client_transfer_policy.h"
#include "core_compatibility.h"
#include "native_input_profile.h"
#include <cstdio>
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
bool TomeGuard(const D2RL::PluginContext* ctx) noexcept {
    if(!Validate(ctx))return false;
    __try {
        const auto core=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"D2RCore.dll"));
        using namespace QolIdentifyStatProfile;
        return core && QolCore::VerifyFileHash(reinterpret_cast<HMODULE>(core),QolNativeProfile::CoreHash) &&
            ctx->CheckExpectedBytes(0x3719e0,QolVendorTomeProfile::Maximum,sizeof(QolVendorTomeProfile::Maximum)) &&
            ctx->CheckExpectedBytes(0x2f5020,Entry,sizeof(Entry)) &&
            QolCoexistence::StatSlot(ctx->exeBase,core) &&
            QolBeltCompat::Match(core+0x831de0,Wrapper,sizeof(Wrapper)) &&
            QolBeltCompat::Match(core+0x3d8bd0,Body,sizeof(Body));
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool TomeQuantity(const D2RL::PluginContext* ctx,const D2RL::Items::ItemInfo& expected,int32_t& quantity,int32_t& maximum) noexcept {
    const D2RL::ItemService* items{};
    D2RL::Items::ItemInfo live{.structSize=D2RL::Items::ItemInfoSize};
    if(ctx->QueryService(&items)!=D2RL::ServiceQueryResult::Success || !items || !items->getItemInfo ||
       items->getItemInfo(ctx,expected.handle,&live)!=D2RL::Items::Result::Success || !QolClientTransfer::Source(expected,live) ||
       live.container!=D2RL::Items::ItemContainer::Inventory || live.inventoryPage!=0)return false;
    __try {
        const auto base=ctx->exeBase;
        auto unit=static_cast<uint32_t*>(reinterpret_cast<void*(__fastcall*)(uint32_t,uint32_t)>(base+0x9a5d0)(live.runtimeId,4));
        if(!unit || unit[0]!=4 || unit[1]!=live.classId || unit[2]!=live.runtimeId || unit[3]!=0 ||
           reinterpret_cast<uint32_t(__fastcall*)(void*)>(base+0x36ef50)(unit)!=live.code ||
           reinterpret_cast<uint8_t(__fastcall*)(void*)>(base+0x36cfe0)(unit)!=0)return false;
        quantity=reinterpret_cast<int32_t(__fastcall*)(void*,int32_t,uint16_t)>(base+0x2f5020)(unit,70,0);
        maximum=reinterpret_cast<int32_t(__fastcall*)(void*)>(base+0x3719e0)(unit);
        return quantity>=0 && maximum>0 && maximum<=511 && quantity<=maximum;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
}
bool SubmitScroll(const D2RL::PluginContext* ctx,const D2RL::Items::ItemInfo& stock,TomePurchase& purchase) noexcept {
    purchase={};
    const auto code=QolVendor::TomeForScroll(stock.code);
    if(!code || !TomeGuard(ctx)){ctx->LogWarn("[QOL/VendorTome] Scroll or native tome profile unavailable; no purchase.");return false;}
    const D2RL::InventoryService* inventory{};D2RL::PlayerHandle player{};
    if(ctx->QueryService(&inventory)!=D2RL::ServiceQueryResult::Success || !inventory || !inventory->getLocalPlayer || !inventory->forEachInventoryItem ||
       inventory->getLocalPlayer(ctx,&player)!=D2RL::Inventory::Result::Success || !player)return false;
    struct Scan {uint32_t code;TomePurchase* purchase;bool valid{true},room{};} scan{code,&purchase};
    const D2RL::Inventory::ItemFilter filter{D2RL::Inventory::ItemFilterSize,0,D2RL::Items::ContainerBit(D2RL::Items::ItemContainer::Inventory),0};
    if(inventory->forEachInventoryItem(ctx,player,&filter,[](const D2RL::PluginContext* ctx,const D2RL::Items::ItemInfo* info,void* user) noexcept {
        auto& s=*static_cast<Scan*>(user);
        if(info && info->code==s.code) {
            int32_t quantity{},maximum{};
            if(s.purchase->count==s.purchase->tomes.size() || !TomeQuantity(ctx,*info,quantity,maximum))s.valid=false;
            else {s.purchase->tomes[s.purchase->count++]=*info;s.purchase->before+=quantity;s.room|=QolVendor::TomeRoom(quantity,maximum);}
        }
        return D2RL::Inventory::IterationAction::Continue;
    },&scan)!=D2RL::Inventory::Result::Success || !scan.valid || !scan.room) {
        ctx->LogInfo("[QOL/VendorTome] No matching non-full Inventory tome; no loose-scroll purchase.");return false;
    }
    __try {
        void* panel{};auto item=Stock(ctx->exeBase,stock,panel);
        auto nativePlayer=D2R::Native::GetLocalPlayerUnit(ctx->exeBase);
        if(!item || !nativePlayer)return false;
        purchase.submitted=true; // Even a native fault must never authorize a retry.
        QolVendor::SubmitBuy(reinterpret_cast<QolVendor::SellFn>(ctx->exeBase+0x23fed0),panel,nativePlayer,item);
        ctx->LogInfo("[QOL/VendorTome] Native scroll Shift-buy submitted once; awaiting matching Inventory tome charges.");
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {ctx->LogWarn("[QOL/VendorTome] Native purchase fault; no retry.");return false;}
}
TomeObservation ObserveScroll(const D2RL::PluginContext* ctx,const TomePurchase& purchase) noexcept {
    if(!purchase.submitted || !TomeGuard(ctx))return TomeObservation::Changed;
    int32_t total{};
    for(unsigned i=0;i<purchase.count;++i) {
        int32_t quantity{},maximum{};
        if(!TomeQuantity(ctx,purchase.tomes[i],quantity,maximum))return TomeObservation::Changed;
        total+=quantity;
    }
    if(total>purchase.before) {
        char line[176];std::snprintf(line,sizeof(line),"[QOL/VendorTome] Matching Inventory tome charges observed %d -> %d; native purchase, no local edits.",purchase.before,total);ctx->LogInfo(line);
        return TomeObservation::Increased;
    }
    return total<purchase.before?TomeObservation::Changed:TomeObservation::Waiting;
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
