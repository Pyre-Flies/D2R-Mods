#pragma once
#include <cstdint>
namespace QolCubeControls {
inline constexpr uint16_t A=0x1000,B=0x2000;
inline constexpr uint64_t HoldMs=600;
// Exact-build getter observes 7 for the idle controller cursor; 5 is targeting.
// A null item alone does not admit an unknown/special cursor mode.
inline constexpr bool CursorAvailable(bool occupied,int mode) noexcept {return !occupied && (mode==0 || mode==7);}
enum class CursorUse { Idle, TrackHold, CompleteHold, Description };
inline constexpr bool NativeCursorAvailable(bool occupied,int mode,CursorUse use,bool armed) noexcept {
    return CursorAvailable(occupied,mode) || (!occupied && armed &&
        ((mode==9 && (use==CursorUse::TrackHold || use==CursorUse::CompleteHold)) || (mode==8 && use==CursorUse::TrackHold)));
}
// Captions describe the binding during the native A press/hold animation too.
// This permission is read-only; native actions retain their narrower policy.
inline constexpr bool ResolveCursorAvailable(bool occupied,int mode,CursorUse use,bool armed) noexcept {
    return use==CursorUse::Description ? !occupied && (mode==0 || mode==7 || mode==8 || mode==9) :
        NativeCursorAvailable(occupied,mode,use,armed);
}
inline constexpr uint8_t CaptionFlags(bool describedCube,bool swap,bool transmute,bool openView,bool hoverAdmitted) noexcept {
    return static_cast<uint8_t>((describedCube && swap?1:0) |
        (transmute && (openView || (describedCube && hoverAdmitted))?2:0));
}
struct Identity {
    uint64_t session{};uint32_t player{},item{};int32_t x{},y{};
    bool operator==(const Identity&) const = default;
};
// Only an A press observed on this carried Cube can admit the game's transient
// charging/completion modes. Cancellation cannot re-arm until a fresh press.
struct NativeHold {
    bool previousA{},armed{},completed{};Identity captured{};
    void Update(uint16_t buttons,bool eligible,bool focus,const Identity& identity) noexcept {
        const bool a=(buttons&A)!=0;
        if(a && !previousA && eligible && focus && buttons==A) {
            captured=identity;armed=true;completed=false;
        }
        if(!a || !eligible || !focus || !(identity==captured) || buttons!=A)armed=completed=false;
        previousA=a;
    }
    bool Matches(const Identity& identity) const noexcept {return (armed || completed) && captured==identity;}
    void Complete() noexcept {armed=false;completed=true;}
    void Cancel() noexcept {armed=completed=false;}
};
struct HoverRequest {
    static constexpr uint64_t LifetimeMs=5000;
    bool pending{},inFlight{};uint64_t started{};Identity captured{};
    void Arm(uint64_t now,const Identity& identity) noexcept {pending=true;inFlight=false;started=now;captured=identity;}
    bool Matches(uint64_t now,const Identity& identity) const noexcept {
        return pending && !inFlight && now>=started && now-started<=LifetimeMs && identity==captured;
    }
    bool Begin(uint64_t now,const Identity& identity) noexcept {
        if(!Matches(now,identity))return false;
        inFlight=true;return true;
    }
    // A companion may legitimately submit another Convert event to finish its
    // recipe. QOL never generates those events or extends the intent deadline.
    void End() noexcept {inFlight=false;}
    void Cancel() noexcept {pending=inFlight=false;}
};
enum class Action { None, Open, PickUp, Transmute };
inline constexpr bool TransmuteAvailable(bool openView,bool carriedCube,bool commandAdmitted) noexcept {
    return openView || (carriedCube && commandAdmitted);
}
inline constexpr Action NativeAction(bool held,bool eligible,bool selectionMatches,bool modifier,bool busy) noexcept {
    return eligible && selectionMatches && !modifier && !busy ? (held?Action::PickUp:Action::Open):Action::None;
}
struct Frame {
    uint64_t now{};uint16_t buttons{};
    bool modifier{},swap{},transmute{},ready{},focus{},cubeView{},busy{};
    Identity identity{};
    uint16_t modifierButtons=0x100;
};
struct Decision {uint16_t consumed{};Action action{};};
struct Gesture {
    bool previousA{},previousB{},ownedA{},ownedB{},finished{},cancelled{};
    uint64_t started{};Identity captured{};
    Decision Update(const Frame& f) noexcept {
        Decision out{};const bool a=(f.buttons&A)!=0,b=(f.buttons&B)!=0;
        const bool newA=a&&!previousA,newB=b&&!previousB;
        if(newA && f.ready && f.swap && f.focus && !f.modifier && !f.busy && !(f.buttons&~A)) {
            ownedA=true;finished=cancelled=false;started=f.now;captured=f.identity;
        }
        if(ownedA) {
            out.consumed|=A;
            if(!f.ready || !f.swap || !f.focus || f.modifier || f.busy ||
               !(captured==f.identity) || f.now<started || (f.buttons&~A))cancelled=true;
            if(!cancelled && !finished && (f.now-started>=HoldMs || !a)) {
                out.action=f.now-started>=HoldMs?Action::PickUp:Action::Open;finished=true;
            }
        }
        if(newB && f.ready && f.transmute && f.cubeView && f.modifier && !f.busy && !ownedA &&
           !(f.buttons&static_cast<uint16_t>(~(B|f.modifierButtons)))) {
            ownedB=true;out.action=Action::Transmute;
        }
        if(ownedB)out.consumed|=B;
        if(!a){ownedA=false;finished=cancelled=false;}
        if(!b)ownedB=false;
        previousA=a;previousB=b;
        return out;
    }
    void Cancel() noexcept {if(ownedA)cancelled=true;}
};
}
