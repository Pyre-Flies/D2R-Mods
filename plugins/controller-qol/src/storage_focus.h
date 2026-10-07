#pragma once
#include "client_transfer_policy.h"
#include "storage_focus_profile.h"
#include "filter_focus_signatures.h"
#include "native_identify_profile.h"
#include "materials_signatures.h"
#include "vendor_signatures.h"
#include <windows.h>
namespace QolStorageFocus {
// UI thread only. Never substitute a tooltip, remembered grid or mouse hover
// for the controller's actual widget and selected cell.
inline bool Matches(const D2RL::PluginContext* ctx,const QolClientTransfer::Info& info) noexcept {
    if(!ctx)return false;
    __try {
        using namespace QolNativeIdentifyProfile;
        if(!ctx->CheckExpectedBytes(0x846020,QolFilterFocus::FocusGetter,sizeof(QolFilterFocus::FocusGetter)) ||
           !ctx->CheckExpectedBytes(0x876160,QolStorageProfile::ControllerFocus,sizeof(QolStorageProfile::ControllerFocus)) ||
           !ctx->CheckExpectedBytes(0x2a89c0,QolStorageProfile::ControllerItem,sizeof(QolStorageProfile::ControllerItem)) ||
           !ctx->CheckExpectedBytes(0x36ef50,Code,sizeof(Code)))return false;
        const auto base=ctx->exeBase;
        auto manager=*reinterpret_cast<const unsigned char**>(base+0x3440170);
        auto focus=manager?*reinterpret_cast<const unsigned char* const*>(manager+0xd0):nullptr;
        auto widget=focus?*reinterpret_cast<unsigned char* const*>(focus+0x190):nullptr;
        if(!widget || !widget[0x50] || !widget[0x51])return false;
        auto table=*reinterpret_cast<const uintptr_t* const*>(widget);
        if(!table)return false;
        const auto code=reinterpret_cast<uint32_t(__fastcall*)(const void*)>(base+0x36ef50);
        if(table[0xc8/8]==base+0x2ce900) {
            using namespace QolMaterials::Signatures;
            if(!ctx->CheckExpectedBytes(0x2ce900,GetBoundItem,sizeof(GetBoundItem)) ||
               !ctx->CheckExpectedBytes(0x2cf4a0,DisplayItemWitness,sizeof(DisplayItemWitness)))return false;
            for(const auto offset:{0x608,0x600}) {
                auto item=*reinterpret_cast<const uint32_t* const*>(widget+offset);
                if(item && item[0]==4 && item[2]==info.runtimeId && code(item)==info.code)return true;
            }
            return false;
        }
        if(table[0xc8/8]!=base+0x2c49f0 ||
           !ctx->CheckExpectedBytes(0x2c49f0,Lookup,sizeof(Lookup)) ||
           !ctx->CheckExpectedBytes(0x2a7810,Owner,sizeof(Owner)) ||
           !ctx->CheckExpectedBytes(0x36cfe0,QolVendor::ItemPageBytes,sizeof(QolVendor::ItemPageBytes)))return false;
        struct Cell {int32_t x,y;};
        const auto cell=*reinterpret_cast<const Cell*>(widget+0x544);
        if(cell.x<0 || cell.y<0 || cell.x>255 || cell.y>255)return false;
        auto item=static_cast<const uint32_t*>(reinterpret_cast<void*(__fastcall*)(void*,const Cell*)>(base+0x2c49f0)(widget,&cell));
        return item && item[0]==4 && item[2]==info.runtimeId && code(item)==info.code &&
            reinterpret_cast<uint8_t(__fastcall*)(const void*)>(base+0x36cfe0)(item)==QolClientTransfer::NativePage(info.container);
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
}
