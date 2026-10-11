#pragma once

#include <D2RLPlugin/widgets.h>
#include <cstdint>
#include <limits>

namespace InventoryDepositGoldAnchor {

// All child rectangles use the same inventory-parent coordinate system.
// Parent x/y belong to its own parent and must not be added a second time.
// The button's current x/y do not constrain where it can be moved.
inline bool Compute(const D2RL::Widgets::Rect& gold,
    const D2RL::Widgets::Rect& button,const D2RL::Widgets::Rect& parent,
    int gap,int offsetY,D2RL::Widgets::Rect& output) noexcept {
    if(parent.width<=0 || parent.height<=0 || gold.width<=0 || gold.height<=0 ||
        button.width<=0 || button.height<=0 || gap<0 || gap>4096 ||
        offsetY<-4096 || offsetY>4096 || gold.x<0 || gold.y<0)return false;

    const int64_t goldRight=static_cast<int64_t>(gold.x)+gold.width;
    const int64_t goldBottom=static_cast<int64_t>(gold.y)+gold.height;
    if(goldRight>parent.width || goldBottom>parent.height)return false;

    const int64_t x=goldRight+gap;
    const int64_t y=static_cast<int64_t>(gold.y)+
        (static_cast<int64_t>(gold.height)-button.height)/2+offsetY;
    constexpr auto max=(std::numeric_limits<int32_t>::max)();
    if(x<0 || y<0 || x>max || y>max ||
        x+button.width>parent.width || y+button.height>parent.height)return false;

    output={static_cast<int32_t>(x),static_cast<int32_t>(y),button.width,button.height};
    return true;
}

}
