#pragma once
#include <D2RLPlugin/api.h>
#include <cstdint>

namespace InventoryDepositStashFeedback {
bool Initialize(const D2RL::PluginContext*) noexcept;
void BeginBatch() noexcept;
void ItemRemoved(const D2RL::Items::ItemInfo&) noexcept;
void EndBatch() noexcept;
void Refresh() noexcept;
void Shutdown() noexcept;

// Narrow native boundary also used by the scheduler/lifecycle fixture. Frame
// pointers exist only on the UI callback's stack and are never retained.
namespace Detail {
struct Frame {
    void* bank{};
    void* tabs{};
    unsigned nativeTab{5};
    std::uint32_t fadeMs{330}, cycles{3};
    bool open{};
};
bool Admit(const D2RL::PluginContext*) noexcept;
bool ReadFrame(const D2RL::PluginContext*,const D2RL::WidgetService*,Frame&) noexcept;
std::uint8_t Category(const D2RL::PluginContext*,std::uint32_t) noexcept;
void Enable(const D2RL::PluginContext*,void*,unsigned,bool) noexcept;
void Alpha(const D2RL::PluginContext*,void*,unsigned,float) noexcept;
std::uint64_t Clock() noexcept;
}
}
