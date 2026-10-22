#pragma once
#include "compatibility.h"
#include <cmath>

namespace DayNight {
enum class Mode { Auto, Day, Night };
inline const char* ModeName(Mode mode) noexcept {return mode==Mode::Day?"day":mode==Mode::Night?"night":"auto";}

// Calls/mutations are serialized on the SDK UI queue. The singleton is borrowed
// afresh each time; a stale pointer or another owner's override is never adopted.
class Preview {
public:
    struct Observation {
        uintptr_t renderer{};uint64_t pair{};unsigned char overrideTime{};
        bool operator==(const Observation&) const = default;
        int Stage() const noexcept {return static_cast<int>(pair&0xffffffff);}
        float Blend() const noexcept {float value{};const auto bits=static_cast<uint32_t>(pair>>32);std::memcpy(&value,&bits,4);return value;}
    };
    struct Checkpoint {Observation observed;uintptr_t owner{};uint64_t original{},expected{};};
private:
    uintptr_t host_{},core_{},owner_{};uint64_t original_{},expected_{};
    Observation published_{};bool publishedValid_{};
    static constexpr uintptr_t PairOffset=Profile::RendererEnvironmentOffset+Profile::EnvironmentPeriodOffset;
    bool Guards() const noexcept {
        using namespace Profile;
        return host_ && core_ && Bytes(host_+RendererGetterRva,RendererGetter) &&
            Bytes(host_+RendererOverrideControlRva,RendererOverrideControl) &&
            Bytes(host_+ScriptSetterCallRva,ScriptSetterCall) && Bytes(host_+NormalCycleTableRva,NormalCycleTable) &&
            Bytes(host_+NativeSetterRva,NativeSetter);
    }
    static bool Writable(uintptr_t address,size_t size) noexcept {
        MEMORY_BASIC_INFORMATION info{};
        return VirtualQuery(reinterpret_cast<void*>(address),&info,sizeof(info)) && info.State==MEM_COMMIT &&
            !(info.Protect&(PAGE_GUARD|PAGE_NOACCESS)) && (info.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)) &&
            address+size>=address && address+size<=reinterpret_cast<uintptr_t>(info.BaseAddress)+info.RegionSize;
    }
    static bool Flag(uintptr_t renderer,unsigned char before,unsigned char after) noexcept {
        const auto address=renderer+Profile::RendererOverrideOffset;
        return Writable(address,1) && static_cast<unsigned char>(_InterlockedCompareExchange8(reinterpret_cast<volatile char*>(address),static_cast<char>(after),static_cast<char>(before)))==before;
    }
    static bool Pair(uintptr_t renderer,uint64_t before,uint64_t after) noexcept {
        const auto address=renderer+PairOffset;
        return !(address&7) && Writable(address,8) && static_cast<uint64_t>(_InterlockedCompareExchange64(reinterpret_cast<volatile long long*>(address),static_cast<long long>(after),static_cast<long long>(before)))==before;
    }
    void Published(const Observation& value) noexcept {published_=value;publishedValid_=true;}
public:
    bool Start(uintptr_t host,uintptr_t core) noexcept {host_=host;core_=core;return Guards();}
    bool AwaitingRenderer() const noexcept {uintptr_t value{};return Guards() && Read(host_+Profile::RendererSlotRva,&value,8) && !value;}
    bool Inspect(Observation& value) const noexcept {
        if(!Guards() || !Read(host_+Profile::RendererSlotRva,&value.renderer,8) || value.renderer<65536 ||
            value.renderer>UINTPTR_MAX-Profile::RendererOverrideOffset || (value.renderer&7) ||
            !Read(value.renderer+PairOffset,&value.pair,8) || !Read(value.renderer+Profile::RendererOverrideOffset,&value.overrideTime,1)) return false;
        return value.Stage()>=0 && value.Stage()<=5 && std::isfinite(value.Blend()) && value.Blend()>=0 && value.Blend()<=1 && value.overrideTime<=1;
    }
    bool SaveCheckpoint(Checkpoint& value) const noexcept {
        if(!Inspect(value.observed)) return false;
        value.owner=owner_;value.original=original_;value.expected=expected_;return true;
    }
    bool Force(Mode mode) noexcept {
        if(mode==Mode::Auto) return Release();
        Observation before{};unsigned char gate{};
        if(!Inspect(before) || !Read(core_+Profile::CoreSafetyFlagRva,&gate,1) || (gate&1)) return false;
        if(owner_) {
            if(before.renderer!=owner_ || before.overrideTime!=1 || before.pair!=expected_) return false;
        } else {
            if(before.overrideTime || !Flag(before.renderer,0,1)) return false;
            owner_=before.renderer;original_=expected_=before.pair;
        }
        // Same complete leaf and argument contract used by native SetTimeOfDay.
        // Stage 2 is noon and stage 5 is night; interpolation is exactly zero.
        const int stage=mode==Mode::Day?2:5;
        reinterpret_cast<void(__fastcall*)(void*,float,int)>(host_+Profile::NativeSetterRva)(
            reinterpret_cast<void*>(owner_+Profile::RendererEnvironmentOffset),0.0f,stage);
        Observation after{};
        if(!Inspect(after) || after.renderer!=owner_ || after.overrideTime!=1 || after.pair!=static_cast<uint64_t>(stage)) return false;
        expected_=after.pair;Published(after);return true;
    }
    bool Release(bool restore=true) noexcept {
        if(!owner_) return true;
        Observation current{};
        if(!Inspect(current) || current.renderer!=owner_ || current.overrideTime!=1 || current.pair!=expected_) return false;
        const auto pair=restore?original_:expected_;
        if(!Pair(owner_,expected_,pair)) return false;
        if(!Flag(owner_,1,0)) return false;
        Published({owner_,pair,0});owner_=0;return true;
    }
    bool DayForOff() noexcept {return Force(Mode::Day) && Release(false);}
    bool Restore(const Checkpoint& before) noexcept {
        Observation current{};
        if(!Inspect(current)) return false;
        if(current==before.observed) {owner_=before.owner;original_=before.original;expected_=before.expected;return true;}
        if(!publishedValid_ || current!=published_ || current.renderer!=before.observed.renderer) return false;
        if(!current.overrideTime && before.observed.overrideTime && !Flag(current.renderer,0,1)) return false;
        if(!Pair(current.renderer,current.pair,before.observed.pair)) return false;
        if(current.overrideTime && !before.observed.overrideTime && !Flag(current.renderer,1,0)) return false;
        owner_=before.owner;original_=before.original;expected_=before.expected;Published(before.observed);return true;
    }
    bool Stop() noexcept {const auto restored=Release();host_=core_=0;return restored;}
};
}
