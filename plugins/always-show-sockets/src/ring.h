#pragma once
#include "scale.h"
#include <algorithm>
#include <vector>

namespace Sockets {
enum class RingLayer {Rim,Center};
// Separate original SpA1 RGBA8 layers let native per-draw alpha control the rim
// and black disk independently, without replacing cached textures at runtime.
inline std::vector<unsigned char> MakeRingSprite(unsigned side,RingLayer layer=RingLayer::Rim) {
    if(side!=64 && side!=128) return {};
    std::vector<unsigned char> bytes(40+static_cast<std::size_t>(side)*side*4);
    auto word=[&](unsigned offset,unsigned value,unsigned count) {
        for(unsigned n=0;n<count;++n) bytes[offset+n]=static_cast<unsigned char>(value>>(n*8));
    };
    bytes[0]='S';bytes[1]='p';bytes[2]='A';bytes[3]='1';
    word(4,31,2);word(6,side,2);word(8,side,4);word(12,side,4);word(20,1,4);
    const double radius=side*0.44,halfBand=side*0.018;
    constexpr unsigned color[]={164,147,110};
    for(unsigned y=0;y<side;++y) for(unsigned x=0;x<side;++x) {
        const double dx=x+0.5-side*0.5,dy=y+0.5-side*0.5,distance=std::hypot(dx,dy);
        const double rim=std::clamp(halfBand+0.5-std::abs(distance-radius),0.0,1.0);
        const double disk=std::clamp(radius-halfBand+0.5-distance,0.0,1.0);
        const double alpha=layer==RingLayer::Rim?rim:disk;
        const auto offset=40+(static_cast<std::size_t>(y)*side+x)*4;
        for(unsigned channel=0;channel<3;++channel)
            bytes[offset+channel]=layer==RingLayer::Rim?static_cast<unsigned char>(color[channel]):0;
        bytes[offset+3]=static_cast<unsigned char>(std::lround(alpha*255));
    }
    return bytes;
}
inline bool FitRing(Point& anchor,float& scale,const SpriteBounds& original,
                    const SpriteBounds& replacement,unsigned percent) noexcept {
    if(!ValidBounds(original) || !ValidBounds(replacement) || original.width!=original.height ||
       replacement.width!=replacement.height || percent<10 || percent>MaxCircleScalePercent) return false;
    SpriteCenter center{};
    if(!CenterOf(anchor,scale,original,center)) return false;
    float fitted=static_cast<float>(static_cast<double>(scale)*original.width/replacement.width);
    // Owned circle layers can reach 115%; keep the ordinary inserted-art
    // percentage guard unchanged and validate the final native scale as usual.
    fitted*=static_cast<float>(percent)/100;
    auto fittedAnchor=anchor;
    if(!ScaleAtCenter(fittedAnchor,fitted,replacement,100,center)) return false;
    anchor=fittedAnchor;scale=fitted;return true;
}
}
