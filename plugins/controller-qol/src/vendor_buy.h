#pragma once
#include <D2RLPlugin/api.h>
#include <array>
namespace QolVendorBuy {
// UI-thread only. Resolve stock through the merchant grid, never by container enum.
bool Check(const D2RL::PluginContext*, const D2RL::Items::ItemInfo&) noexcept;
void Submit(const D2RL::PluginContext*, const D2RL::Items::ItemInfo&) noexcept;
struct TomePurchase {
    std::array<D2RL::Items::ItemInfo,32> tomes{};
    unsigned count{};int32_t before{};bool submitted{};
};
enum class TomeObservation { Waiting, Increased, Changed };
bool SubmitScroll(const D2RL::PluginContext*,const D2RL::Items::ItemInfo&,TomePurchase&) noexcept;
TomeObservation ObserveScroll(const D2RL::PluginContext*,const TomePurchase&) noexcept;
}
