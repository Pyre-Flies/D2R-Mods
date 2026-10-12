#pragma once
#include <D2RLPlugin/api.h>
#include "identify_action.h"
namespace QolCubeShortcut {
using C=D2RL::Items::ItemContainer;
inline bool IsCube(uint32_t code) noexcept {
    return QolIdentify::Code(code,D2RL::Items::MakeItemCode("box"));
}
// All routes, including LB+X, must check the live item before submitting a move.
// A Cube already in Cube storage may still be withdrawn to Inventory.
inline bool Allows(uint32_t code,C destination) noexcept {
    return destination!=C::Cube || !IsCube(code);
}
inline bool Context(C source,bool inventory,bool stash,bool cube) noexcept {
    switch(source) {
    case C::Inventory:return inventory || stash || cube;
    case C::PersonalStash:case C::SharedStash:return stash;
    case C::Cube:return cube || stash;
    default:return false;
    }
}
inline bool Current(uint64_t capturedGeneration,uint64_t generation,uint64_t capturedAt,uint64_t now) noexcept {
    return capturedGeneration==generation && now>=capturedAt && now-capturedAt<=1000;
}
}
