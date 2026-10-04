#pragma once
#include "aim_policy.h"
#include <array>
#include <span>
namespace Aim {
struct LayoutNode { float x{},y{},width{},height{},anchorX{},anchorY{},scale{1}; bool fill{}; };
struct ScreenRect { float x{},y{},width{},height{}; };
inline bool LayoutBounds(std::span<const LayoutNode> chain,ScreenRect& output) noexcept {
    if(chain.empty() || chain.size()>16)return false;
    auto effective=[&](std::size_t i) noexcept -> const LayoutNode* {
        while(i<chain.size() && chain[i].fill)++i;
        return i<chain.size()?&chain[i]:nullptr;
    };
    float x=0,y=0,scale=chain[0].scale;
    for(std::size_t i=0;i<chain.size();++i) {
        const auto& n=chain[i];
        if(!Finite({n.x,n.y}) || !Finite({n.width,n.height}) || !Finite({n.anchorX,n.anchorY}) ||
           !std::isfinite(n.scale) || n.scale<=0 || n.scale>8 || std::abs(n.x)>100000 || std::abs(n.y)>100000 ||
           n.width<0 || n.height<0 || n.width>100000 || n.height>100000 || std::abs(n.anchorX)>4 || std::abs(n.anchorY)>4)return false;
        if(!n.fill){x+=n.x;y+=n.y;}
        if(i+1<chain.size()) {
            const auto* parent=effective(i+1);if(!parent)return false;
            x=(x+parent->width*n.anchorX)*chain[i+1].scale;
            y=(y+parent->height*n.anchorY)*chain[i+1].scale;
            scale*=chain[i+1].scale;
        }
    }
    const auto* rect=effective(0);if(!rect)return false;
    output={x,y,rect->width*scale,rect->height*scale};
    return Finite({output.x,output.y}) && Finite({output.width,output.height}) && output.width>0 && output.height>0;
}
inline bool IconBadge(const ScreenRect& rect,Point& center,float& radius) noexcept {
    if(!Finite({rect.x,rect.y}) || !Finite({rect.width,rect.height}) || rect.width<20 || rect.height<20 || rect.width>300 || rect.height>300)return false;
    const float size=std::min(rect.width,rect.height);
    radius=std::clamp(size*0.075f,3.0f,7.0f);
    const float inset=radius+size*0.075f;
    center={rect.x+rect.width-inset,rect.y+inset};
    return true;
}
}
