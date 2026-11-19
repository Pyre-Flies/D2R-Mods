#pragma once
// Native drop sound replacement (0x1C8130 -> D2RLoader+0x1A0C00) and
// Mouse pickup guard (0xFABE0) with keyboard Ctrl (VK_LCONTROL / VK_RCONTROL) override.

#include "unit_reader.h"
#include <D2RLPlugin/api.h>
#include <windows.h>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace PyreLootFilter::AudioAndPickup {

namespace Rva {
inline constexpr std::uintptr_t NativeDropPlayer = 0x1C8130;
inline constexpr std::uintptr_t DropCallerReturn1 = 0x1C36A0;
inline constexpr std::uintptr_t DropCallerReturn2 = 0x1C6C9E;
inline constexpr std::uintptr_t LoaderPlayNamedSound = 0x1A0C00;
inline constexpr std::uintptr_t SetSoundHandleVolume = 0x19F900;
inline constexpr std::uintptr_t SoundChannelsArray = 0x246CA30;
inline constexpr std::uintptr_t SoundChannelsCount = 0x246CA38;
inline constexpr std::size_t SoundChannelStride = 0x98U;
inline constexpr std::size_t SoundChannelHashOffset = 0x80U;
inline constexpr std::size_t SoundChannelSliderOffset = 0x94U;
inline constexpr std::uint32_t HashChannelSfx = 0xD01EC7FCU;      // FNV1a32("sfx")
inline constexpr std::uint32_t HashChannelItemsHd = 0x86ED69F0U;  // FNV1a32("sfx/items_hd")

inline constexpr std::uintptr_t NativeActionDispatch = 0xFABE0;
inline constexpr std::uintptr_t NativeItemLookup = 0x9A5D0;
inline constexpr std::uintptr_t UiManagerSlot = 0x3440170;

inline constexpr std::uint32_t PickupActionId = 22U;
} // namespace Rva

namespace Witnesses {
inline constexpr std::array<std::uint8_t, 20> NativeDropPlayerEntry{{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C, 0x24, 0x10,
    0x48, 0x89, 0x74, 0x24, 0x18, 0x48, 0x89, 0x7C, 0x24, 0x20,
}};
inline constexpr std::array<std::uint8_t, 11> LoaderPlayNamedEntry{{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74, 0x24, 0x10, 0x57,
}};
inline constexpr std::array<std::uint8_t, 16> SetSoundHandleVolumeEntry{{
    0x44, 0x8B, 0xD2, 0x48, 0x85, 0xC9, 0x74, 0x2F,
    0x48, 0x8B, 0x05, 0x91, 0x67, 0x8F, 0x02, 0x4C,
}};
inline constexpr std::array<std::uint8_t, 21> NativeActionDispatchEntry{{
    0x40, 0x55, 0x53, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57,
    0x48, 0x8D, 0x6C, 0x24, 0xC9, 0x48, 0x81, 0xEC, 0xA0, 0x00, 0x00, 0x00,
}};
} // namespace Witnesses

using NativeDropPlayerFn = void(__fastcall*)(void*) noexcept;
using PlayNamedSoundFn = void*(__fastcall*)(const char*, void*, int, int) noexcept;
using SetSoundHandleVolumeFn = void(__fastcall*)(void*, std::uint8_t) noexcept;
using NativeActionDispatchFn = void(__fastcall*)(std::uint32_t, void*, std::uint32_t,
                                                 std::uint32_t) noexcept;
using NativeItemLookupFn = void*(__fastcall*)(std::uint32_t, std::uint32_t) noexcept;

// Reads the user's Audio Options "Sound Effects" ("sfx" +0x94) and "Items" ("sfx/items_hd" +0x94)
// slider multipliers (0.0f..1.0f) from the runtime soundchannels.json table so custom drop sounds
// routed through the unattenuated "master" bus still honor the Items and Sound Effects sliders.
[[nodiscard]] inline std::uint8_t ReadItemsSliderVolumeByte(std::uintptr_t exeBase) noexcept {
    if (!exeBase) return 0xFFU;
    std::uintptr_t arrayPtr = 0;
    std::uint64_t count = 0;
    if (!Reader::CopyReadable(reinterpret_cast<const void*>(exeBase + Rva::SoundChannelsArray),
                              &arrayPtr, sizeof(arrayPtr)) ||
        !Reader::CopyReadable(reinterpret_cast<const void*>(exeBase + Rva::SoundChannelsCount),
                              &count, sizeof(count)) ||
        !arrayPtr || count == 0 || count > 256U) {
        return 0xFFU;
    }

    float sfxSlider = 1.0f;
    float itemsSlider = 1.0f;
    bool foundSfx = false;
    bool foundItems = false;
    for (std::uint64_t i = 0; i < count && (!foundSfx || !foundItems); ++i) {
        const std::uintptr_t entry = arrayPtr + static_cast<std::uintptr_t>(i * Rva::SoundChannelStride);
        std::uint32_t idHash = 0;
        float slider = 1.0f;
        if (!Reader::CopyReadable(reinterpret_cast<const void*>(entry + Rva::SoundChannelHashOffset),
                                  &idHash, sizeof(idHash)) ||
            !Reader::CopyReadable(reinterpret_cast<const void*>(entry + Rva::SoundChannelSliderOffset),
                                  &slider, sizeof(slider)) ||
            !std::isfinite(slider)) {
            continue;
        }
        const float clamped = slider < 0.0f ? 0.0f : (slider > 1.0f ? 1.0f : slider);
        if (idHash == Rva::HashChannelSfx) {
            sfxSlider = clamped;
            foundSfx = true;
        } else if (idHash == Rva::HashChannelItemsHd) {
            itemsSlider = clamped;
            foundItems = true;
        }
    }

    const float combined = sfxSlider * itemsSlider;
    const long scaled = std::lround(combined * 255.0f);
    if (scaled <= 0L) return 0U;
    if (scaled >= 255L) return 0xFFU;
    return static_cast<std::uint8_t>(scaled);
}

[[nodiscard]] inline SetSoundHandleVolumeFn QualifySetSoundHandleVolume(
    std::uintptr_t exeBase) noexcept {
    if (!exeBase) return nullptr;
    std::array<std::uint8_t, Witnesses::SetSoundHandleVolumeEntry.size()> entry{};
    if (!Reader::CopyReadable(reinterpret_cast<const void*>(exeBase + Rva::SetSoundHandleVolume),
                              entry.data(), entry.size()) ||
        entry != Witnesses::SetSoundHandleVolumeEntry) {
        return nullptr;
    }
    return reinterpret_cast<SetSoundHandleVolumeFn>(exeBase + Rva::SetSoundHandleVolume);
}

[[nodiscard]] constexpr bool IsKeyboardCtrlHeld(short leftState, short rightState) noexcept {
    return ((static_cast<std::uint16_t>(leftState) | static_cast<std::uint16_t>(rightState)) &
            0x8000U) != 0U;
}

[[nodiscard]] constexpr bool IsKeyboardAltHeld(short leftState, short rightState) noexcept {
    return IsKeyboardCtrlHeld(leftState, rightState);
}

[[nodiscard]] inline bool IsKeyboardCtrlHeldNow() noexcept {
    return IsKeyboardCtrlHeld(GetAsyncKeyState(VK_LCONTROL), GetAsyncKeyState(VK_RCONTROL));
}

[[nodiscard]] inline bool IsKeyboardAltHeldNow() noexcept {
    return IsKeyboardCtrlHeldNow();
}

// Per-millisecond thread-local cached check for high-density label paint loops.
[[nodiscard]] inline bool IsKeyboardCtrlHeldCached(std::uint64_t nowMs) noexcept {
    thread_local std::uint64_t s_lastTick = 0;
    thread_local bool s_cachedHeld = false;
    if (s_lastTick != nowMs || nowMs == 0) {
        s_lastTick = nowMs;
        s_cachedHeld = IsKeyboardCtrlHeldNow();
    }
    return s_cachedHeld;
}

[[nodiscard]] inline bool IsKeyboardPickupMode(std::uintptr_t exeBase) noexcept {
    if (!exeBase) return false;
    std::uintptr_t manager{}, managerAfter{};
    std::uint32_t mode = ~0U;
    return Reader::CopyReadable(reinterpret_cast<const void*>(exeBase + Rva::UiManagerSlot),
                                &manager, sizeof(manager)) &&
           manager != 0 && manager <= UINTPTR_MAX - 0xE0U &&
           Reader::CopyReadable(reinterpret_cast<const void*>(manager + 0xDCU), &mode,
                                sizeof(mode)) &&
           mode == 0U &&
           Reader::CopyReadable(reinterpret_cast<const void*>(exeBase + Rva::UiManagerSlot),
                                &managerAfter, sizeof(managerAfter)) &&
           managerAfter == manager;
}

[[nodiscard]] inline std::uintptr_t QualifyNamedSoundLoaderBase(
    std::uintptr_t exeBase = 0) noexcept {
    std::uintptr_t base = exeBase;
    if (!base) {
        const HMODULE mod = GetModuleHandleW(L"D2RLoader.exe");
        if (!mod) return 0;
        base = reinterpret_cast<std::uintptr_t>(mod);
    }
    std::array<std::uint8_t, Witnesses::LoaderPlayNamedEntry.size()> entry{};
    if (!Reader::CopyReadable(reinterpret_cast<const void*>(base + Rva::LoaderPlayNamedSound),
                              entry.data(), entry.size()) ||
        entry != Witnesses::LoaderPlayNamedEntry) {
        return 0;
    }
    return base;
}

enum class DropSoundAction : std::uint8_t {
    PlayVanilla = 0,
    SuppressHidden = 1,
    PlayedCustom = 2,
};

[[nodiscard]] inline DropSoundAction EvaluateDropSound(
    void* unit, std::uintptr_t callerRva, std::uintptr_t loaderBase,
    bool dropSoundsEnabled, const Schema::FilterRuleTable* table,
    const Reader::NativeReaders& readers,
    Cache::UnitEvaluationCache& cache,
    bool dropSound3dAttenuation = false,
    bool* outMissingCustomSound = nullptr) noexcept {
    if (!unit || !table || (table->soundRules == 0 && table->hiddenRules == 0)) {
        return DropSoundAction::PlayVanilla;
    }
    if (callerRva != Rva::DropCallerReturn1 && callerRva != Rva::DropCallerReturn2) {
        return DropSoundAction::PlayVanilla;
    }

    Reader::UnitHeader before{};
    if (!Reader::ReadUnitHeader(unit, before) ||
        !Reader::AllowsMode(before.mode, Reader::Purpose::VerifiedLabel)) {
        return DropSoundAction::PlayVanilla;
    }

    Schema::ItemFacts facts{};
    Schema::EvaluatedDecision decision{};
    if (!readers.EvaluateUnit(unit, 0, table, cache, Reader::Purpose::VerifiedLabel, facts,
                              decision) ||
        !decision.matched) {
        return DropSoundAction::PlayVanilla;
    }

    Reader::UnitHeader after{};
    if (!Reader::ReadUnitHeader(unit, after) || after != before) {
        return DropSoundAction::PlayVanilla;
    }

    // Suppress drop sound (both custom and vanilla) when the item is hidden by the filter.
    if (!decision.show) {
        return DropSoundAction::SuppressHidden;
    }

    if (dropSoundsEnabled && loaderBase && decision.hasDropSound && decision.dropSound[0]) {
        const std::uint8_t sliderVolByte = ReadItemsSliderVolumeByte(loaderBase);
        if (sliderVolByte == 0U) {
            return DropSoundAction::PlayedCustom;
        }
        auto* playNamed = reinterpret_cast<PlayNamedSoundFn>(loaderBase + Rva::LoaderPlayNamedSound);
        auto* setHandleVol = sliderVolByte < 0xFFU ? QualifySetSoundHandleVolume(loaderBase) : nullptr;
        void* const soundUnit = dropSound3dAttenuation ? unit : nullptr;
        __try {
            if (void* const handle = playNamed(decision.dropSound.data(), soundUnit, 0, 0);
                handle != nullptr) {
                if (setHandleVol) {
                    setHandleVol(handle, sliderVolByte);
                }
                return DropSoundAction::PlayedCustom;
            } else if (outMissingCustomSound &&
                       std::strncmp(decision.dropSound.data(), "Filter", 6) == 0) {
                *outMissingCustomSound = true;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    return DropSoundAction::PlayVanilla;
}

[[nodiscard]] inline bool TryPlayRuleDropSound(
    void* unit, std::uintptr_t callerRva, std::uintptr_t loaderBase,
    const Schema::FilterRuleTable* table,
    const Reader::NativeReaders& readers,
    Cache::UnitEvaluationCache& cache,
    bool dropSound3dAttenuation = false) noexcept {
    return EvaluateDropSound(unit, callerRva, loaderBase, true, table, readers, cache,
                             dropSound3dAttenuation) ==
           DropSoundAction::PlayedCustom;
}

[[nodiscard]] constexpr bool ShouldBlockEvaluatedGroundPickup(
    const Schema::ItemFacts& facts,
    const Schema::EvaluatedDecision& decision) noexcept {
    return decision.matched && !decision.show && !Tables::IsGoldCode(facts.code);
}

[[nodiscard]] inline bool ShouldBlockMouseGroundPickup(
    std::uint32_t action, void* player, std::uint32_t type, std::uint32_t unitId,
    bool altPickupOverrideEnabled, std::uintptr_t exeBase,
    const Schema::FilterRuleTable* table,
    const Reader::NativeReaders& readers,
    Cache::UnitEvaluationCache& cache) noexcept {
    if (action != Rva::PickupActionId || type != Reader::UnitTypeItem || !unitId || !player ||
        !exeBase || !table || table->hiddenRules == 0) {
        return false;
    }
    if (altPickupOverrideEnabled && IsKeyboardCtrlHeldNow()) return false;
    if (!IsKeyboardPickupMode(exeBase)) return false;

    std::uint32_t playerType = 0xFFFFFFFFU;
    if (!Reader::CopyReadable(player, &playerType, sizeof(playerType)) || playerType != 0U) {
        return false;
    }

    auto* lookupFn = reinterpret_cast<NativeItemLookupFn>(exeBase + Rva::NativeItemLookup);
    void* unit = nullptr;
    __try {
        unit = lookupFn(unitId, Reader::UnitTypeItem);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (!unit) return false;

    Reader::UnitHeader before{};
    if (!Reader::ReadUnitHeader(unit, before) || before.unitId != unitId ||
        before.mode != Reader::ItemModeOnGround) {
        return false;
    }

    Schema::ItemFacts facts{};
    Schema::EvaluatedDecision decision{};
    if (!readers.EvaluateUnit(unit, 0, table, cache, Reader::Purpose::StrictGround, facts,
                              decision) ||
        !ShouldBlockEvaluatedGroundPickup(facts, decision)) {
        return false;
    }

    Reader::UnitHeader after{};
    if (!Reader::ReadUnitHeader(unit, after) || after != before) return false;
    if (altPickupOverrideEnabled && IsKeyboardCtrlHeldNow()) return false;
    if (!IsKeyboardPickupMode(exeBase)) return false;
    return true;
}

} // namespace PyreLootFilter::AudioAndPickup
