#pragma once
#include "aim_policy.h"

namespace Aim {
// Estimate only from copied positions on the existing UI observation path.
// Timing/jump limits are conservative policy, not native missile constants.
struct MotionTrack {
    Point anchor{}, velocity{};
    std::uint64_t anchorTick{}, lastTick{};
    unsigned consistent{};
    void Reset(Point position,std::uint64_t tick) noexcept {
        anchor=position; anchorTick=lastTick=tick; velocity={}; consistent=0;
    }
    void Observe(Point position,std::uint64_t tick) noexcept {
        if(!anchorTick || tick<lastTick || tick-lastTick>150 || !ValidWorld(position)) {
            Reset(position,tick); return;
        }
        lastTick=tick;
        const auto elapsed=tick-anchorTick;
        if(elapsed<50) return; // Duplicate/closely spaced samples are not velocity.
        if(elapsed>150) { Reset(position,tick); return; }
        const float seconds=static_cast<float>(elapsed)/1000;
        const Point next{(position.x-anchor.x)/seconds,(position.y-anchor.y)/seconds};
        anchor=position; anchorTick=tick;
        const float speed=std::hypot(next.x,next.y), oldSpeed=std::hypot(velocity.x,velocity.y);
        if(!Finite(next) || speed>40 || speed<0.25f) { velocity={}; consistent=0; return; }
        const bool agrees=consistent && oldSpeed>0 &&
            next.x*velocity.x+next.y*velocity.y>=0.75f*speed*oldSpeed &&
            speed>=oldSpeed*0.5f && speed<=oldSpeed*2;
        velocity=agrees?Point{(next.x+velocity.x)*0.5f,(next.y+velocity.y)*0.5f}:next;
        consistent=agrees?2u:1u; // Two agreeing windows before any prediction.
    }
    Point Destination(Point player,Point position,std::uint64_t now,unsigned millisecondsPerTile,unsigned maxMilliseconds=600,unsigned maxTiles=3) const noexcept {
        if(!millisecondsPerTile || millisecondsPerTile>200 || maxMilliseconds<100 || maxMilliseconds>1500 ||
            maxTiles<1 || maxTiles>8 || consistent<2 ||
            !Fresh(now,lastTick,75) || !Finite(velocity) || !ValidWorld(player) || !ValidWorld(position)) return position;
        const float seconds=std::min(maxMilliseconds/1000.0f,std::sqrt(DistanceSquared(player,position))*millisecondsPerTile/1000);
        Point offset{velocity.x*seconds,velocity.y*seconds};
        const float length=std::hypot(offset.x,offset.y);
        if(length>maxTiles) { offset.x*=maxTiles/length; offset.y*=maxTiles/length; }
        const Point predicted{position.x+offset.x,position.y+offset.y};
        return ValidWorld(predicted) && DistanceSquared(player,predicted)<=MaximumDistance*MaximumDistance?predicted:position;
    }
};
}
