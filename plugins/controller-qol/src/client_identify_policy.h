#pragma once
#include <cstdint>
namespace QolClientIdentify {
// Native request builder ABI recovered from 0x2C77D9 and 0x2CAF2C.
// This calls the game's builder; it does not assemble or send packet bytes.
using RequestFn=void(__fastcall*)(uint8_t,uint32_t,int32_t,uint8_t,uint32_t,
    uint32_t,int32_t,uint8_t,uint32_t,const uint32_t*,const uint32_t*,const uint32_t*) noexcept;
inline bool Cell(int32_t x,int32_t y,uint32_t packed) noexcept {
    return x>=0 && x<=255 && y>=0 && y<=255 && packed==(static_cast<uint32_t>(x)|(static_cast<uint32_t>(y)<<16));
}
inline bool Confirmed(bool identified,int32_t before,int32_t after) noexcept {
    return identified && before>0 && after==before-1;
}
inline bool StoragePage(uint8_t page) noexcept {return page==0 || page==4;}
inline bool LooseScrollQuantity(int32_t quantity) noexcept {return quantity>=0 && quantity<=1;}
inline bool ScrollConfirmed(bool identified,bool sdkSourcePresent,bool nativeSourcePresent) noexcept {
    return identified && !sdkSourcePresent && !nativeSourcePresent;
}
inline void Submit(RequestFn fn,uint8_t nativeUseFlag,uint32_t tome,uint32_t tomeCell,uint32_t target,uint32_t targetCell,
                   uint8_t sourcePage=0,uint8_t targetPage=0) noexcept {
    if(!fn || !StoragePage(sourcePage) || !StoragePage(targetPage))return;
    // Stored Inventory/Personal source and target. Owner checks exclude Shared.
    // Equipment auxiliary arrays are read only for source mode 2, which is excluded.
    fn(nativeUseFlag,tome,0,sourcePage,tomeCell,target,4,targetPage,targetCell,nullptr,nullptr,nullptr);
}
}
