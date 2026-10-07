#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Aim {
inline constexpr int MultiShot=12, GuidedArrow=22, FireWall=51, Teleport=54, Meteor=56, Blizzard=59, Hydra=62, Teeth=67, Leap=132, Whirlwind=151;
inline constexpr float DefaultDistance = 20, MinimumDistance = 0, MaximumDistance = 30;
inline constexpr float SnapRadius = 6, Deadzone = 0.22f;
inline constexpr bool EnemySnapping = true; // Per-skill policy below; independent cursor
struct Point { float x{}, y{}; };
inline bool Finite(Point p) noexcept { return std::isfinite(p.x) && std::isfinite(p.y); }
inline float DistanceSquared(Point a, Point b) noexcept {
    const auto x = a.x-b.x, y = a.y-b.y;
    return x*x+y*y;
}
inline const char* SkillName(int skill) noexcept {
    switch(skill) {
        case MultiShot: return "Multi Shot"; case GuidedArrow: return "Guided Arrow";
        case FireWall: return "Fire Wall"; case Teleport: return "Teleport";
        case Meteor: return "Meteor"; case Blizzard: return "Blizzard";
        case Hydra: return "Hydra"; case Teeth: return "Teeth";
        case Leap: return "Leap"; case Whirlwind: return "Whirlwind";
        default: return nullptr;
    }
}
inline bool Supported(int skill) noexcept { return SkillName(skill)!=nullptr; }
inline bool SnapSkill(int skill) noexcept { return Supported(skill) && skill!=Teleport && skill!=Leap; }
inline bool ValidWorld(Point p) noexcept {
    return Finite(p) && p.x >= 1 && p.y >= 1 && p.x <= 65534 && p.y <= 65534;
}
inline bool Center(Point player, Point facing, float distance, Point& out) noexcept {
    if (!ValidWorld(player) || !Finite(facing) || !std::isfinite(distance) ||
        distance < MinimumDistance || distance > MaximumDistance) return false;
    const float length = std::hypot(facing.x,facing.y);
    if (!std::isfinite(length) || length < 0.001f) return false;
    out = {player.x + facing.x/length*distance, player.y + facing.y/length*distance};
    return ValidWorld(out);
}
// Resolve one ground endpoint; the native cast owns subsequent movement.
inline bool WhirlwindEndpoint(Point player,Point enemy,Point& out,float passThrough=1.5f) noexcept {
    if(!ValidWorld(player) || !ValidWorld(enemy) || !std::isfinite(passThrough) || passThrough<0 || passThrough>15) return false;
    const float distance=std::sqrt(DistanceSquared(player,enemy));
    if(!std::isfinite(distance) || distance>MaximumDistance) return false;
    if(passThrough==0) { out=enemy; return true; }
    if(distance<0.1f) return false;
    const float length=std::min(distance+passThrough,MaximumDistance);
    out={player.x+(enemy.x-player.x)*length/distance,player.y+(enemy.y-player.y)*length/distance};
    return ValidWorld(out);
}
inline float Score(Point center, Point candidate,float radius=SnapRadius) noexcept {
    if (!ValidWorld(center) || !ValidWorld(candidate)) return -1;
    const float d2 = DistanceSquared(center,candidate);
    // Larger scores win natively; -1 is the native rejection sentinel.
    return d2 <= radius*radius ? 1 + radius*radius - d2 : -1;
}
inline bool Fresh(std::uint64_t now, std::uint64_t then, std::uint64_t limit=250) noexcept {
    return then && now >= then && now-then <= limit;
}
// Measured screen/world basis. aim.6 also uses its directions to interpret
// stick input in screen space; distance remains bounded in world tiles.
struct Projection {
    Point screenOrigin{}, worldOrigin{}, worldX{}, worldY{};
    float step{};
    bool Project(Point world, Point& screen) const noexcept {
        const Point a{worldX.x-worldOrigin.x, worldX.y-worldOrigin.y};
        const Point b{worldY.x-worldOrigin.x, worldY.y-worldOrigin.y};
        const Point d{world.x-worldOrigin.x, world.y-worldOrigin.y};
        const float determinant = a.x*b.y-a.y*b.x;
        if (!Finite(world) || !Finite(a) || !Finite(b) || !Finite(d) ||
            !std::isfinite(step) || step <= 0 || std::abs(determinant) < 0.01f) return false;
        screen = {screenOrigin.x+step*(d.x*b.y-d.y*b.x)/determinant,
            screenOrigin.y+step*(a.x*d.y-a.y*d.x)/determinant};
        return Finite(screen);
    }
};
struct ReticleColor { float red{},green{},blue{},alpha{1}; };
struct MotionSettings {
    float deadzone=0.22f, initialSpeed=4, maximumSpeed=28, accelerationSeconds=0.65f;
    bool snapping=true, overlay=true, debugOverlay=false, whirlwindPassThrough=true;
    bool castObserver=true;
    bool skillTreeToggle=true;
    ReticleColor groundReticleColor{0.76f,0.71f,0.59f,1},lockReticleColor{0.80f,0.61f,0.32f,0.95f};
    float reticleThickness=1.0f;
    float whirlwindPassThroughDistance=1.5f;
    float snapRadius=6, switchAdvantage=1.5f, projectionHz=60, overlaySmoothingMs=35;
    bool Valid() const noexcept {
        return std::isfinite(reticleThickness) && reticleThickness>=0.5f && reticleThickness<=4 &&
            std::isfinite(whirlwindPassThroughDistance) && whirlwindPassThroughDistance>=0 && whirlwindPassThroughDistance<=15 &&
            std::isfinite(deadzone) && deadzone>=0 && deadzone<=0.9f &&
            std::isfinite(initialSpeed) && initialSpeed>=0.1f && initialSpeed<=100 &&
            std::isfinite(maximumSpeed) && maximumSpeed>=initialSpeed && maximumSpeed<=100 &&
            std::isfinite(accelerationSeconds) && accelerationSeconds>=0 && accelerationSeconds<=5 &&
            std::isfinite(snapRadius) && snapRadius>=0.5f && snapRadius<=15 &&
            std::isfinite(switchAdvantage) && switchAdvantage>=0 && switchAdvantage<=15 &&
            std::isfinite(projectionHz) && projectionHz>=10 && projectionHz<=120 &&
            std::isfinite(overlaySmoothingMs) && overlaySmoothingMs>=0 && overlaySmoothingMs<=150;
    }
};
inline bool StickDirection(Point stick,const Projection& projection,bool invertY,float deadzone,Point& directionOut) noexcept {
    if(!Finite(stick) || std::abs(stick.x)>1.05f || std::abs(stick.y)>1.05f) return false;
    const float magnitude=std::hypot(stick.x,stick.y);
    if(magnitude<=deadzone) return false; // retain last offset on release
    Point check{};
    if(!projection.Project(projection.worldOrigin,check)) return false;
    const float screenY=invertY?stick.y:-stick.y;
    const Point a{projection.worldX.x-projection.worldOrigin.x,projection.worldX.y-projection.worldOrigin.y};
    const Point b{projection.worldY.x-projection.worldOrigin.x,projection.worldY.y-projection.worldOrigin.y};
    const Point direction{a.x*stick.x+b.x*screenY,a.y*stick.x+b.y*screenY};
    const float length=std::hypot(direction.x,direction.y);
    if(!std::isfinite(length) || length<0.001f) return false;
    directionOut={direction.x/length,direction.y/length};
    return Finite(directionOut);
}
// Velocity-controlled cursor: even full-scale input starts slowly. Release stops
// immediately; no momentum or spring-return distance mapping is retained.
struct CursorMotion {
    float heldSeconds{};
    Point previousDirection{};
    static float RampIntegral(float t,float ramp) noexcept {
        return ramp>0 && t<ramp ? t*t/(2*ramp) : t-ramp/2;
    }
    bool Advance(Point stick,const Projection& projection,bool invertY,float seconds,const MotionSettings& settings,Point& offset) noexcept {
        Point direction{};
        if(!settings.Valid() || !Finite(offset) || !std::isfinite(seconds) || seconds<0 || seconds>0.1f ||
            !StickDirection(stick,projection,invertY,settings.deadzone,direction)) { *this={}; return false; }
        // Large direction changes restart fine control instead of carrying fast motion.
        if(direction.x*previousDirection.x+direction.y*previousDirection.y<0.5f) heldSeconds=0;
        previousDirection=direction;
        // GetTickCount64 can repeat across frames. No elapsed time means no
        // movement, not a released stick; preserve the accumulated ramp.
        if(seconds==0) return false;
        const float nextTime=heldSeconds+seconds;
        const float strength=(std::min(std::hypot(stick.x,stick.y),1.0f)-settings.deadzone)/(1-settings.deadzone);
        const float travel=strength*strength*(settings.initialSpeed*seconds+(settings.maximumSpeed-settings.initialSpeed)*
            (RampIntegral(nextTime,settings.accelerationSeconds)-RampIntegral(heldSeconds,settings.accelerationSeconds)));
        heldSeconds=std::min(nextTime,settings.accelerationSeconds);
        Point next{offset.x+direction.x*travel,offset.y+direction.y*travel};
        const float radius=std::hypot(next.x,next.y);
        if(radius>MaximumDistance) { next.x*=MaximumDistance/radius; next.y*=MaximumDistance/radius; }
        offset=next;
        return true;
    }
};
}

namespace Aim {
// Render-only filtering: raw projection remains authoritative for stick movement.
struct DisplayProjection {
    Projection value{}; std::uint64_t tick{};
    Projection Update(const Projection& next,std::uint64_t now,float smoothingMs) noexcept {
        Point test{};
        if(!tick || !Fresh(now,tick,250) || smoothingMs<=0 ||
            DistanceSquared(value.worldOrigin,next.worldOrigin)>64 ||
            DistanceSquared(value.screenOrigin,next.screenOrigin)>1 || value.step!=next.step ||
            !value.Project(value.worldOrigin,test)) { value=next; tick=now; return value; }
        const float alpha=1-std::exp(-static_cast<float>(now-tick)/smoothingMs);
        auto mix=[&](Point a,Point b) { return Point{a.x+(b.x-a.x)*alpha,a.y+(b.y-a.y)*alpha}; };
        value.worldOrigin=mix(value.worldOrigin,next.worldOrigin);
        value.worldX=mix(value.worldX,next.worldX); value.worldY=mix(value.worldY,next.worldY);
        if(!value.Project(value.worldOrigin,test)) value=next;
        tick=now; return value;
    }
};
}
