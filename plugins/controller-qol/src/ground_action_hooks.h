#pragma once
#include <D2RLPlugin/api.h>
#include <array>
#include <atomic>
#include <utility>
#include "compatibility_signatures.h"

namespace QolCompat {
// Observe the native handlers without owning the packet table. A table wrapper
// installed by another plugin still calls these same native entry addresses.
class ActionHooks {
public:
    using Handler = int64_t(__fastcall*)(void*, void*, void*, int32_t);
    using Observer = void(*)(uint8_t, void*, void*, void*, int32_t) noexcept;
    using Projector = bool(*)(void*, const void*, int32_t,
        std::array<uint8_t,5>&) noexcept;
private:
    inline static Handler originals[18]{};
    inline static Observer observer{};
    inline static Projector projector{};
    inline static std::atomic<bool> active{false};
    template<size_t I>
    static int64_t __fastcall Hook(void* game, void* player, void* packet, int32_t size) noexcept {
        void* forwardedPacket = packet;
        std::array<uint8_t,5> projectedPacket{};
        if constexpr (I == 4) {
            if (active.load(std::memory_order_acquire) && projector &&
                projector(player, packet, size, projectedPacket))
                forwardedPacket = projectedPacket.data();
        }
        const auto result = originals[I](game, player, forwardedPacket, size);
        if (active.load(std::memory_order_acquire) && observer)
            observer(static_cast<uint8_t>(I+1), game, player, packet, size);
        return result;
    }
    template<size_t... I>
    static bool InstallAll(const D2RL::PluginContext* ctx, std::index_sequence<I...>) noexcept {
        return (ctx->InstallInlineHook(Native::Actions[I].rva, Native::Actions[I].bytes, 32,
            reinterpret_cast<void*>(&Hook<I>), reinterpret_cast<void**>(&originals[I])) && ...);
    }
public:
    static bool Install(const D2RL::PluginContext* ctx, Observer callback,
        Projector packetProjector = nullptr) noexcept {
        active.store(false);
        for (const auto& site : Native::Actions)
            if (!ctx->CheckExpectedBytes(site.rva, site.bytes, 32)) return false;
        observer = callback;
        projector = packetProjector;
        if (!InstallAll(ctx,std::make_index_sequence<18>{})) return false;
        for (auto fn : originals) if (!fn) return false;
        active.store(true,std::memory_order_release);
        return true;
    }
    static void Shutdown() noexcept {
        // Keep trampolines valid until D2RLoader removes the owned hooks.
        active.store(false,std::memory_order_release);
        projector = nullptr;
    }
};
}
