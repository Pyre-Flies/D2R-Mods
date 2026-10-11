#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <span>
#include "elements.h"

namespace Damage {
struct Point { float x{}, y{}; };
inline bool Valid(Point p) noexcept { return std::isfinite(p.x) && std::isfinite(p.y) && p.x>=1 && p.y>=1 && p.x<65535 && p.y<65535; }
struct Projection {
    Point origin{}, world{}, worldX{}, worldY{};
    float step{};
    bool Project(Point p, Point& out) const noexcept {
        const Point a{worldX.x-world.x,worldX.y-world.y}, b{worldY.x-world.x,worldY.y-world.y}, d{p.x-world.x,p.y-world.y};
        const float determinant=a.x*b.y-a.y*b.x;
        if (!Valid(p) || !std::isfinite(determinant) || std::abs(determinant)<0.01f || !std::isfinite(step) || step<=0) return false;
        out={origin.x+step*(d.x*b.y-d.y*b.x)/determinant,origin.y+step*(a.x*d.y-a.y*d.x)/determinant};
        return std::isfinite(out.x) && std::isfinite(out.y);
    }
};
struct PoisonStamp {
    std::int64_t rate{};
    std::uint32_t frame{}, expires{};
    bool valid{};
};
struct Monster {
    std::uintptr_t identity{};
    std::uint32_t id{}, classId{};
    std::int64_t hp{}, maximum{};
    Point position{};
    PoisonStamp poison{};
};
struct Number {
    Monster target{};
    std::int64_t raw{};
    std::uint64_t born{}, last{};
    unsigned lane{};
    std::int64_t fromHp{};
    std::uint64_t since{};
    unsigned elements{};
    bool colorEligible=true;
    PoisonStamp fromPoison{};
    bool critical{};
};
struct Settings {
    bool enabled=true, debug=false, fallingMotion=true, shadow=true, elementColors=true, backing=true, backingPoisonOnly=true, criticalHits=true;
    float textSize=26, rise=64, offset=46, laneSpacing=22;
    std::array<float,3> textColor{213.0f/255,218.0f/255,226.0f/255}, outlineColor{16.0f/255,18.0f/255,21.0f/255}, shadowColor{0,0,0};
    std::array<float,3> physicalColor{241.0f/255,227.0f/255,202.0f/255}, fireColor{1,139.0f/255,104.0f/255},
        coldColor{141.0f/255,216.0f/255,1}, lightningColor{246.0f/255,220.0f/255,120.0f/255},
        magicColor{182.0f/255,161.0f/255,245.0f/255}, poisonColor{198.0f/255,246.0f/255,155.0f/255},
        mixedColor{245.0f/255,140.0f/255,182.0f/255}, backingColor{10.0f/255,13.0f/255,16.0f/255};
    float fadeStart=0.8f;
    float side=48, fall=96, shadowX=4, shadowY=4, shadowOpacity=0.75f;
    float backingOpacity=0.68f, backingPaddingX=4, backingPaddingY=1, backingRounding=3;
    float criticalScale=1.35f;
    unsigned outlineWidth=2, shadowSoftness=1;
    unsigned lifetime=1000, mergeMs=120, precision=0, maximum=128;
};
class Model {
    std::array<Monster,1024> previous{};
    struct LastSeen { Monster target{}; std::uint64_t tick{}; bool occupied{}; };
    std::array<LastSeen,1024> lastSeen{};
    std::size_t count{};
    std::uintptr_t game{};
    std::uint64_t generation{};
    std::uint64_t sampleTick{};
    struct Hit {Monster target{}; std::int64_t after{}; std::uint64_t started{}, completed{}; unsigned elements{}; PoisonStamp afterPoison{};bool critical{};};
    std::array<Hit,256> hits{};
    std::size_t hitNext{}, hitCount{};
    static constexpr std::uint64_t DeathRetentionMs=2000;
    static constexpr std::uint64_t PendingDeathMs=500;
    static bool SameTarget(const Monster& a,const Monster& b) noexcept {
        return a.identity==b.identity && a.id==b.id && a.classId==b.classId;
    }
    static bool PoisonDecrease(const Monster& before,const Monster& after) noexcept {
        const auto& a=before.poison; const auto& b=after.poison;
        if(!SameTarget(before,after) || !a.valid || a.rate<=0 || a.rate>INT32_MAX || before.hp<=after.hp || b.frame<=a.frame ||
            b.frame-a.frame>8 || a.expires<=a.frame || b.frame>=a.expires) return false;
        if(after.hp>0 && (!b.valid || b.rate!=a.rate || b.expires!=a.expires)) return false;
        const auto predicted=a.rate*static_cast<std::int64_t>(b.frame-a.frame);
        return after.hp==0 ? predicted>=before.hp : predicted==before.hp-after.hp;
    }
    void AppendHit(const Hit& hit) noexcept {
        hits[hitNext]=hit;hitNext=(hitNext+1)%hits.size();hitCount=std::min(hitCount+1,hits.size());
    }
    void ExtendPoisonHistory(const Monster& before,const Monster& after,std::uint64_t since,std::uint64_t now) noexcept {
        auto cursor=before;auto cursorTick=since;
        for(std::size_t i=0;i<hitCount;++i) {
            const auto& hit=hits[(hitNext+hits.size()-hitCount+i)%hits.size()];
            if(!SameTarget(hit.target,after) || hit.started<since || hit.started>now || hit.completed>now ||
                now-hit.completed>DeathRetentionMs || hit.after<0 || hit.after>=cursor.hp) continue;
            if(hit.target.hp!=cursor.hp && !PoisonDecrease(cursor,hit.target)) return;
            if(hit.after<after.hp) return;
            cursor=hit.target;cursor.hp=hit.after;cursor.poison=hit.afterPoison;cursorTick=hit.completed;
        }
        if(PoisonDecrease(cursor,after)) AppendHit({cursor,after.hp,cursorTick,now,Poison,after.poison});
    }
    void ConfirmTerminal(const Monster& target,std::uint64_t since,std::uint64_t now) noexcept {
        for(auto& hit:hits) if(hit.after<0 && SameTarget(hit.target,target) && hit.started>=since &&
            now>=hit.completed && now-hit.completed<=PendingDeathMs) hit.after=0;
    }
    void Remember(const Monster& m,std::uint64_t now) noexcept {
        auto slot=std::find_if(lastSeen.begin(),lastSeen.end(),[&](const LastSeen& x){return x.occupied && x.target.id==m.id;});
        if(slot==lastSeen.end()) slot=std::find_if(lastSeen.begin(),lastSeen.end(),[&](const LastSeen& x){return !x.occupied || now<x.tick || now-x.tick>DeathRetentionMs;});
        if(slot==lastSeen.end()) slot=std::min_element(lastSeen.begin(),lastSeen.end(),[](const LastSeen& a,const LastSeen& b){return a.tick<b.tick;});
        *slot={m,now,true};
    }
    struct Classification {unsigned elements{};bool critical{};};
    Classification Classify(const Number& n,std::uint64_t now) const noexcept {
        if(!n.colorEligible) return {};
        auto cursor=n.target;cursor.hp=n.fromHp;cursor.poison=n.fromPoison;unsigned mask{};bool critical{};
        for(std::size_t i=0;i<hitCount;++i) {
            const auto& hit=hits[(hitNext+hits.size()-hitCount+i)%hits.size()];
            if(hit.target.identity!=n.target.identity || hit.target.id!=n.target.id || hit.target.classId!=n.target.classId ||
                hit.started<n.since || hit.started>n.last || hit.completed>now || now<hit.completed || now-hit.completed>DeathRetentionMs) continue;
            if(hit.after<0) continue; // Unreadable HP is pending, never a zero.
            if(hit.after>=cursor.hp) continue;
            if(hit.target.hp!=cursor.hp) {
                if(!PoisonDecrease(cursor,hit.target)) return {};
                mask|=Poison;
            }
            if(hit.after<n.target.hp) return {};
            cursor=hit.target;cursor.hp=hit.after;cursor.poison=hit.afterPoison;mask|=hit.elements;critical=critical || hit.critical;
            if(cursor.hp==n.target.hp) return {mask,critical};
        }
        if(PoisonDecrease(cursor,n.target)) return {mask|Poison,critical};
        return {}; // Missing a decrease means hit metadata is ambiguous.
    }
    void ClassifyNumber(Number& number,std::uint64_t now) const noexcept {
        const auto result=Classify(number,now);number.elements=result.elements;number.critical=result.critical;
    }
    void Add(const Monster& m,std::int64_t delta,std::uint64_t since,std::uint64_t now,const Settings& config,bool allowMerge,PoisonStamp fromPoison={}) noexcept {
        Number* slot=nullptr;
        if(allowMerge && config.mergeMs && delta<=512) {
            for(auto& n:numbers) if(n.raw && n.target.hp>0 && n.target.identity==m.identity && n.target.id==m.id &&
                now>=n.last && now-n.last<=config.mergeMs && now-n.born<config.lifetime/2) {
                if(!slot || n.last>slot->last) slot=&n;
            }
        }
        if(slot) {
            slot->colorEligible=slot->colorEligible && slot->target.hp==m.hp+delta;
            slot->raw+=delta; slot->last=now; slot->target.hp=m.hp;slot->target.poison=m.poison;
            ClassifyNumber(*slot,now); return;
        }
        unsigned lane=0;
        for(;lane<8;++lane) {
            bool used=false;
            for(const auto& n:numbers) if(n.raw && n.target.identity==m.identity && n.target.id==m.id && n.lane==lane) used=true;
            if(!used) break;
        }
        const auto limit=std::min<std::size_t>(config.maximum,numbers.size());
        for(std::size_t i=0;i<limit;++i) if(!numbers[i].raw) { slot=&numbers[i]; break; }
        if(!slot && limit) slot=&*std::min_element(numbers.begin(),numbers.begin()+static_cast<std::ptrdiff_t>(limit),[](const Number& a,const Number& b){return a.born<b.born;});
        if(slot) { *slot={m,delta,now,now,lane%8,m.hp+delta,since,Unclassified,true,fromPoison}; ClassifyNumber(*slot,now); }
    }
public:
    enum class HitResult {Ignored,NotDecrease,PendingDeath,Accepted};
    static const char* ResultName(HitResult result) noexcept {
        return result==HitResult::Accepted?"accepted":result==HitResult::PendingDeath?"pending-death":
            result==HitResult::NotDecrease?"not-decrease":"ignored-state";
    }
    std::array<Number,128> numbers{};
    void Clear() noexcept { count=0; game=0; generation=0; sampleTick=0; numbers={}; lastSeen={}; hits={}; hitNext=0; hitCount=0; }
    HitResult RecordHit(const Monster& before,std::int64_t after,bool afterValid,unsigned elements,
        std::uintptr_t owner,std::uint64_t session,std::uint64_t started,std::uint64_t now,bool lethalCandidate=false,PoisonStamp afterPoison={},bool critical=false) noexcept {
        if(owner!=game || session!=generation || now<started || now-started>DeathRetentionMs || before.hp<=0) return HitResult::Ignored;
        // Another readable pre-hit HP proves any older unresolved record was
        // not the terminal hit. Never carry its element into a later death.
        for(auto& hit:hits) if(hit.after<0 && SameTarget(hit.target,before) && hit.completed<=started) hit={};
        if(!afterValid) {
            // A destroyed stat list is not a zero-HP read. Only the supported
            // death event (or a valid zero sample) can establish the endpoint.
            const auto dead=std::find_if(lastSeen.begin(),lastSeen.end(),[&](const LastSeen& x){return x.occupied &&
                x.target.identity==before.identity && x.target.id==before.id && x.target.classId==before.classId &&
                x.target.hp==0 && x.tick>=started && x.tick<=now;});
            if(dead==lastSeen.end()) {
                if(!lethalCandidate) return HitResult::NotDecrease;
                AppendHit({before,-1,started,now,elements,{},critical});
                return HitResult::PendingDeath;
            }
            after=0;
        }
        if(after<0 || after>=before.hp) return HitResult::NotDecrease;
        AppendHit({before,after,started,now,elements,afterPoison,critical});
        // Death callbacks can run inside the original combat call, before its
        // final component record can be copied. Update only that fresh target.
        for(auto& n:numbers) if(n.raw && n.target.identity==before.identity && n.target.id==before.id &&
            n.target.classId==before.classId && n.last>=started && n.last<=now && now-n.last<=250)
            ClassifyNumber(n,now);
        return HitResult::Accepted;
    }
    void Expire(std::uint64_t now,const Settings& config) noexcept {
        for(auto& n:numbers) if(n.raw && (now<n.born || now-n.born>=config.lifetime)) n={};
    }
    void Sample(std::span<const Monster> monsters,std::uintptr_t owner,std::uint64_t session,std::uint64_t now,const Settings& config) noexcept {
        if(owner!=game || session!=generation) { Clear(); game=owner; generation=session; }
        Expire(now,config);
        for(const auto& m:monsters) {
            for(auto& n:numbers) if(n.raw && n.target.id==m.id && n.target.identity==m.identity) n.target.position=m.position;
            const auto end=previous.begin()+static_cast<std::ptrdiff_t>(count);
            const auto old=std::find_if(previous.begin(),end,[&](const Monster& x){return x.id==m.id && x.identity==m.identity && x.classId==m.classId;});
            if(m.hp<0 || m.maximum<=0 || m.hp>m.maximum) continue;
            if(m.hp==0) ConfirmTerminal(m,sampleTick,now);
            if(old!=end && old->hp>m.hp) {
                ExtendPoisonHistory(*old,m,sampleTick,now);
                Add(m,old->hp-m.hp,sampleTick,now,config,m.hp>0,old->poison);
            }
            Remember(m,now);
        }
        count=std::min(monsters.size(),previous.size());
        std::copy_n(monsters.begin(),count,previous.begin());
        sampleTick=now;
    }
    std::int64_t Death(std::uint32_t id,std::uint32_t classId,std::uintptr_t owner,std::uint64_t session,std::uint64_t now,const Settings& config,
        std::uint32_t frame=0,bool frameValid=false) noexcept {
        if(owner!=game || session!=generation) return 0;
        const auto cached=std::find_if(lastSeen.begin(),lastSeen.end(),[&](const LastSeen& x){return x.occupied && x.target.id==id && x.target.classId==classId;});
        if(cached==lastSeen.end() || cached->target.hp<=0 || now<cached->tick || now-cached->tick>DeathRetentionMs) return 0;
        Expire(now,config);
        const auto remaining=cached->target.hp;
        auto dead=cached->target; dead.hp=0;dead.poison.frame=frame;dead.poison.valid=frameValid;
        ConfirmTerminal(dead,cached->tick,now);
        if(frameValid) ExtendPoisonHistory(cached->target,dead,cached->tick,now);
        Add(dead,remaining,cached->tick,now,config,false,cached->target.poison);
        // Retire the baseline so neither another death event nor a later zero
        // HP sample repeats the terminal decrease.
        cached->target.hp=0; cached->tick=now;
        for(std::size_t i=0;i<count;++i) if(previous[i].id==id && previous[i].identity==dead.identity && previous[i].classId==classId) previous[i].hp=0;
        return remaining;
    }
};
} // namespace Damage
