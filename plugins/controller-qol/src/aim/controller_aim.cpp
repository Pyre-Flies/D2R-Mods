#include <D2RLPlugin/api.h>
#include <windows.h>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "aim_policy.h"
#include "aim_config.h"
#include "snap_policy.h"
#include "native_profile.h"
#include "coexistence_policy.h"
#include "core_compatibility.h"
#include "controller_aim.h"
#include "portal_priority.h"

namespace {
using LookupFn = void(__fastcall*)(void*,void*,bool,float*,float*) noexcept;
using UnitTestFn = bool(__fastcall*)(void*,void*,void*) noexcept;
using PointScoreFn = float(__fastcall*)(void*,void*,const float*,int,int) noexcept;
using CastFn = int(__fastcall*)(void*,void*,unsigned,unsigned,unsigned,void*) noexcept;
using GetObjectFn = void*(__fastcall*)();
using ReadAimFn = void(__fastcall*)(void*,void*,float*,float*);
using ActiveSkillFn = int(__fastcall*)(void*,unsigned);
using GetPlayerFn = void*(__fastcall*)(unsigned);
using AxisFn = void*(__fastcall*)(void*,Aim::Point*,unsigned);
using SelectedFn = void*(__fastcall*)(void*,int) noexcept;

using InGameFn = bool(__cdecl*)() noexcept;

const D2RL::PluginContext* ctx{};
const D2RL::OverlayService* overlay{};
const D2RL::ThreadService* threads{};
InGameFn inGame{};
LookupFn originalLookup{};
UnitTestFn originalUnitTest{};
PointScoreFn originalPointScore{};
CastFn originalCast{};
SelectedFn originalSelected{};
std::atomic<unsigned> selectedSuppressionCount{};
std::atomic<bool> ready{}, installed{}, enabled{false}, inSession{}, queued{}, reverseY{};
std::atomic<float> requestedDistance{Aim::DefaultDistance};
std::atomic<unsigned> lookupCount{}, scoringCount{}, castCount{};
std::atomic<int> previewSkill{-1};
std::atomic<bool> rendererAdmitted{};
std::atomic<bool> clipCorrectionAdmitted{};
std::atomic<unsigned> overlayReports{};
std::atomic<unsigned> projectionReports{}, lastProjectionStatus{99};
SRWLOCK stateLock=SRWLOCK_INIT;
Aim::Point retainedOffset{};
bool retainedOffsetValid{};
Aim::CursorMotion cursorMotion{};
Aim::MotionSettings motionSettings{}; // immutable after plugin load
Aim::SnapBook snapBook{}; // under stateLock
bool whirlLockActive{}; // same lock; cannot inherit another skill's retained target

struct View {
    bool valid{}, target{}, projectionValid{};
    int skill{-1}, activeSkill{-1};
    unsigned playerId{}, targetId{}, projectionStatus{};
    float axis{}, distance{Aim::DefaultDistance};
    Aim::Point player{}, facing{}, center{}, destination{}, targetPosition{}, stick{};
    Aim::Projection projection{};
    std::uint64_t tick{}, targetTick{}, projectionTick{};
    Aim::SnapDiagnostics snapDiagnostics{};
};
View published{}, lastLookup{};
struct CastMarker { bool valid{}; unsigned playerId{}; Aim::Point position{}; std::uint64_t tick{}; };
CastMarker lastCastMarker{};
std::atomic<unsigned> candidateCount{}, previewCandidateReports{};
struct PendingTeleport {
    bool active{};
    unsigned sequence{}, playerId{};
    Aim::Point start{}, requested{};
    std::uint64_t tick{};
};
PendingTeleport pending{};
float observedDisplacement{-1};
std::atomic<std::uint64_t> lastUiTick{};
struct ScoringWindow { bool active{}, monster{}; void* player{}; Aim::Point center{}; };
thread_local ScoringWindow scoringWindow{};

template<class T> T NativeFunction(std::uintptr_t rva) noexcept {
    return reinterpret_cast<T>(ctx->exeBase+rva);
}
template<class T> T Field(const void* object,std::size_t offset) noexcept {
    T value{};
    std::memcpy(&value,static_cast<const unsigned char*>(object)+offset,sizeof(value));
    return value;
}
bool Foreground() noexcept {
    DWORD process{};
    GetWindowThreadProcessId(GetForegroundWindow(),&process);
    return process==GetCurrentProcessId();
}
// All native calls are on their natural caller or the SDK UI thread. No unit
// pointer is published to the render callback or retained between frames.
bool ReadView(void* expectedPlayer, View& result, bool allowPreview=false) noexcept {
    if (!installed.load() || !enabled.load() || !inSession.load() || !inGame || !inGame() || !Foreground()) return false;
    __try {
        const auto index=NativeFunction<unsigned(__fastcall*)()>(Native::IndexRva)();
        if(index>=8 || !NativeFunction<bool(__fastcall*)()>(Native::ControllerModeRva)()) return false;
        void* const player=NativeFunction<GetPlayerFn>(Native::LocalPlayerRva)(index);
        if(!player || (expectedPlayer && player!=expectedPlayer) || Field<unsigned>(player,0)!=0) return false;
        void* const aim=NativeFunction<GetObjectFn>(Native::GetAimRva)();
        if(!aim) return false;
        int skill=NativeFunction<ActiveSkillFn>(Native::ActiveSkillRva)(aim,index);
        result.activeSkill=skill;
        // Active skill is -1 after release. UI-only distance adjustment may
        // retain the last test skill. Passive candidate observation may also use
        // it, but native scoring overrides and Lookup still require an active skill.
        if(skill==-1 && allowPreview) skill=previewSkill.load();
        if(!Aim::Supported(skill)) return false;
        result.skill=skill;
        result.playerId=Field<unsigned>(player,8);
        NativeFunction<ReadAimFn>(Native::ReadAimRva)(aim,player,&result.player.x,&result.facing.x);
        Aim::Point offset{};
        bool haveOffset{};
        AcquireSRWLockShared(&stateLock);
        offset=retainedOffset; haveOffset=retainedOffsetValid;
        ReleaseSRWLockShared(&stateLock);
        if(!haveOffset) {
            Aim::Point initial{};
            if(!Aim::Center(result.player,result.facing,Aim::DefaultDistance,initial)) return false;
            offset={initial.x-result.player.x,initial.y-result.player.y};
            AcquireSRWLockExclusive(&stateLock);
            if(!retainedOffsetValid) { retainedOffset=offset; retainedOffsetValid=true; }
            offset=retainedOffset;
            ReleaseSRWLockExclusive(&stateLock);
        }
        result.center={result.player.x+offset.x,result.player.y+offset.y};
        result.distance=std::hypot(offset.x,offset.y);
        if(!Aim::ValidWorld(result.center)) return false;
        Aim::Point axis{};
        void* const input=NativeFunction<GetObjectFn>(Native::InputRva)();
        if(!input) return false;
        NativeFunction<AxisFn>(Native::SecondaryAxisRva)(input,&axis,index);
        if(!Aim::Finite(axis) || std::abs(axis.x)>1.05f || std::abs(axis.y)>1.05f) return false;
        result.axis=axis.y;
        result.stick=axis;
        result.destination=result.center;
        result.tick=GetTickCount64();
        result.valid=true;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}

bool ReadMonster(void* unit, Aim::Point& position, unsigned& id) noexcept {
    __try {
        if(!unit || Field<unsigned>(unit,0)!=1) return false;
        const auto path=Field<const unsigned char*>(unit,0x38);
        if(!path) return false;
        // Native LookupSkillTarget uses the same 16.16 dynamic-path values.
        position={static_cast<float>(Field<unsigned>(path,0))/65536.0f,
            static_cast<float>(Field<unsigned>(path,4))/65536.0f};
        id=Field<unsigned>(unit,8);
        return Aim::ValidWorld(position);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}

// Called with stateLock held. Facing and native selected-target caches are not
// inputs: normal turn-to-cast cannot move the cursor or re-rank the same point.
void ResolveSelection(View& view,bool allowPreview=false) noexcept {
    const bool groundPreview=Aim::GroundSnapPreview(view.activeSkill,view.skill,allowPreview);
    if(!motionSettings.snapping || (!Aim::SnapSkill(view.skill) && !groundPreview)) { whirlLockActive=false; return; }
    const bool whirl=view.skill==Aim::Whirlwind;
    if(whirl && (!whirlLockActive || std::hypot(view.stick.x,view.stick.y)>motionSettings.deadzone)) snapBook.retained={};
    whirlLockActive=whirl;
    const auto chosen=snapBook.Choose(view.playerId,view.center,view.player,view.tick,&view.snapDiagnostics,motionSettings.snapRadius,motionSettings.switchAdvantage,whirl);
    if(chosen.valid) {
        Aim::Point endpoint=chosen.position;
        if(whirl && !Aim::WhirlwindEndpoint(view.player,chosen.position,endpoint,motionSettings.whirlwindPassThrough?motionSettings.whirlwindPassThroughDistance:0)) return;
        view.target=true; view.targetTick=chosen.tick;
        view.targetId=chosen.id; view.destination=endpoint; view.targetPosition=chosen.position;
    }
}

void* __fastcall Selected(void* controller,int skill) noexcept {
    void* const nativeTarget=originalSelected(controller,skill);
    View view{};
    // Active local controller skill only. Never use idle preview to suppress
    // native interactions, mouse targeting, or another skill's selected unit.
    if(!Aim::Supported(skill) || !ReadView(nullptr,view) || !Aim::UseCoordinateTarget(skill,view.activeSkill)) return nativeTarget;
    if(nativeTarget) {
        const auto count=selectedSuppressionCount.fetch_add(1)+1;
        if(count<=32) {
            char message[160];
            std::snprintf(message,sizeof(message),"[QOL/Aim] coordinate route #%u skill=%d: native selected unit suppressed; Lookup supplies cursor/snap coordinates.",count,skill);
            ctx->LogInfo(message);
        }
    }
    return nullptr;
}

void __fastcall Lookup(void* controller,void* player,bool adjust,float* x,float* y) noexcept {
    originalLookup(controller,player,adjust,x,y);
    View view{};
    if(!x || !y || !ReadView(player,view)) return;
    previewSkill.store(view.skill);
    AcquireSRWLockExclusive(&stateLock);
    ResolveSelection(view);
    lastLookup=view;
    ReleaseSRWLockExclusive(&stateLock);
    __try { *x=view.destination.x; *y=view.destination.y; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return; }
    const auto count=lookupCount.fetch_add(1)+1;
    if(count<=128) {
        char message[256];
        std::snprintf(message,sizeof(message),"[QOL/Aim] lookup #%u skill=%d range=%.2f center=(%.2f,%.2f) destination=(%.2f,%.2f) target=%s id=%u.",
            count,view.skill,view.distance,view.center.x,view.center.y,view.destination.x,view.destination.y,view.target?"monster":"ground",view.targetId);
        ctx->LogInfo(message);
        if(Aim::SnapSkill(view.skill)) {
            const auto& d=view.snapDiagnostics;
            char detail[280];
            std::snprintf(detail,sizeof(detail),"[QOL/Aim] snap decision lookup=%u fresh=%u expired=%u outsideCircle=%u outsideRange=%u lastRejected=%u rejectedAge=%llu targetAge=%llu centerDistance=%.2f.",
                count,d.fresh,d.expired,d.outsideCircle,d.outsideRange,d.rejectedId,
                static_cast<unsigned long long>(d.rejectedTick && view.tick>=d.rejectedTick?view.tick-d.rejectedTick:999999),
                static_cast<unsigned long long>(view.target && view.tick>=view.targetTick?view.tick-view.targetTick:999999),
                view.target?std::sqrt(Aim::DistanceSquared(view.center,view.destination)):-1.0f);
            ctx->LogInfo(detail);
        }
    }
    AcquireSRWLockExclusive(&stateLock);
    // Keep the UI-thread projection if it is still recent.
    if(Aim::Fresh(view.tick,published.tick,150)) {
        view.projection=published.projection; view.projectionValid=published.projectionValid;
        view.projectionTick=published.projectionTick;
    }
    published=view;
    ReleaseSRWLockExclusive(&stateLock);
}

bool __fastcall UnitTest(void* controller,void* player,void* candidate) noexcept {
    const ScoringWindow previous=scoringWindow;
    scoringWindow={};
    View view{};
    Aim::Point position{}; unsigned id{};
    const bool haveView=ReadView(player,view,true);
    const auto mode=haveView && motionSettings.snapping?Aim::MeteorObservation(view.activeSkill,view.skill):Aim::ObservationMode::None;
    const bool observe=mode!=Aim::ObservationMode::None && ReadMonster(candidate,position,id);
    // Prepare the shared cursor while idle after either test skill.
    // Teleport never resolves a snap; this prepares the next Meteor cast.
    // Other active skills are excluded by MeteorObservation before this scope.
    if(observe) {
        Aim::Point scoringCenter=view.center;
        // Retain only the same freshly observed enemy, not every nearby unit.
        AcquireSRWLockShared(&stateLock);
        const auto held=snapBook.retained;
        const bool whirlHeld=whirlLockActive;
        ReleaseSRWLockShared(&stateLock);
        if(whirlHeld && view.skill==Aim::Whirlwind && std::hypot(view.stick.x,view.stick.y)<=motionSettings.deadzone &&
            held.valid && held.playerId==view.playerId && held.id==id &&
            Aim::Fresh(view.tick,held.tick,Aim::SnapBook::Lifetime) &&
            Aim::DistanceSquared(view.player,position)<=Aim::MaximumDistance*Aim::MaximumDistance)
            scoringCenter=position;
        scoringWindow={true,true,player,scoringCenter};
    }
    const bool result=originalUnitTest(controller,player,candidate);
    scoringWindow=previous;
    if(observe) {
        unsigned currentId{};
        const bool stillReadable=ReadMonster(candidate,position,currentId) && currentId==id;
        const bool eligible=result && stillReadable;
        AcquireSRWLockExclusive(&stateLock);
        snapBook.Observe(view.playerId,id,position,GetTickCount64(),eligible);
        ReleaseSRWLockExclusive(&stateLock);
        const auto count=candidateCount.fetch_add(1)+1;
        const bool previewNear=mode==Aim::ObservationMode::MeteorPreviewCircle && Aim::Score(view.center,position,motionSettings.snapRadius)>=0;
        const bool reportPreview=previewNear && previewCandidateReports.fetch_add(1)<24;
        if(reportPreview || (count<=64 && (eligible || count<=8))) {
            char message[192];
            std::snprintf(message,sizeof(message),"[QOL/Aim] candidate #%u id=%u eligible=%u source=%s center=(%.2f,%.2f) position=(%.2f,%.2f).",
                count,id,eligible,mode==Aim::ObservationMode::MeteorPreviewCircle?"preview-circle":"active-snap",view.center.x,view.center.y,position.x,position.y);
            ctx->LogInfo(message);
        }
    }
    return result;
}

float __fastcall PointScore(void* controller,void* player,const float* position,int category,int profile) noexcept {
    const float nativeScore=originalPointScore(controller,player,position,category,profile);
    if(!enabled.load() || !scoringWindow.active || scoringWindow.player!=player) return nativeScore;
    // Replace geometry only in native Meteor candidate enumeration. Native
    // visibility, hostility/category and skill validation still execute.
    float score=-1;
    __try {
        if(scoringWindow.monster && position) score=Aim::Score(scoringWindow.center,{position[0],position[1]},motionSettings.snapRadius);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return nativeScore; }
    const auto count=scoringCount.fetch_add(1)+1;
    if(count<=8) {
        char message[160];
        std::snprintf(message,sizeof(message),"[QOL/Aim] Aim circle score #%u category=%d profile=%d native=%.3f circle=%.3f.",count,category,profile,nativeScore,score);
        ctx->LogInfo(message);
    }
    return score;
}

bool CaptureCast(void* player,void* selectedSkill,unsigned x,unsigned y,int& skill,PendingTeleport& sample) noexcept {
    __try {
        const void* record=selectedSkill?Field<const void*>(selectedSkill,0):nullptr;
        if(!record || !player) return false;
        skill=Field<unsigned short>(record,0);
        if(!Aim::Supported(skill)) return false;
        const auto path=Field<const unsigned char*>(player,0x38);
        if(!path || x>65534 || y>65534 || !x || !y) return false;
        sample.playerId=Field<unsigned>(player,8);
        sample.start={static_cast<float>(Field<unsigned short>(path,2)),static_cast<float>(Field<unsigned short>(path,6))};
        sample.requested={static_cast<float>(x),static_cast<float>(y)};
        sample.tick=GetTickCount64();
        sample.active=skill==Aim::Teleport;
        return Aim::ValidWorld(sample.start);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}

int __fastcall Cast(void* game,void* player,unsigned x,unsigned y,unsigned flag,void* selectedSkill) noexcept {
    PendingTeleport sample{};
    int skill{};
    const bool observed=installed.load() && enabled.load() && CaptureCast(player,selectedSkill,x,y,skill,sample);
    // Pass-through observer: never rewrites packets, coordinates or server rules.
    const int result=originalCast(game,player,x,y,flag,selectedSkill);
    if(observed) {
        sample.sequence=castCount.fetch_add(1)+1;
        char message[224];
        std::snprintf(message,sizeof(message),"[QOL/Aim] cast #%u skill=%d start=(%.0f,%.0f) request=(%u,%u) distance=%.2f result=%d (source unclassified).",
            sample.sequence,skill,sample.start.x,sample.start.y,x,y,std::sqrt(Aim::DistanceSquared(sample.start,sample.requested)),result);
        if(sample.sequence<=128) ctx->LogInfo(message);
        View lookup{};
        AcquireSRWLockShared(&stateLock); lookup=lastLookup; ReleaseSRWLockShared(&stateLock);
        if(lookup.valid && lookup.playerId==sample.playerId && lookup.skill==skill && Aim::Fresh(sample.tick,lookup.tick,200)) {
            AcquireSRWLockExclusive(&stateLock);
            lastCastMarker={true,sample.playerId,sample.requested,sample.tick};
            ReleaseSRWLockExclusive(&stateLock);
            char alignment[224];
            std::snprintf(alignment,sizeof(alignment),"[QOL/Aim] recent lookup comparison cast=%u target=%u snapped=%u age=%llu ms coordinateError=%.2f (temporal match, not transaction proof).",
                sample.sequence,lookup.targetId,lookup.target,static_cast<unsigned long long>(sample.tick-lookup.tick),
                std::sqrt(Aim::DistanceSquared(sample.requested,lookup.destination)));
            if(sample.sequence<=128) ctx->LogInfo(alignment);
        }
        if(sample.active) {
            AcquireSRWLockExclusive(&stateLock); pending=sample; observedDisplacement=-1; ReleaseSRWLockExclusive(&stateLock);
        }
    }
    return result;
}

// Use the complete native XY wrapper. It owns its collision-result buffer;
// the plugin supplies only the two floats written explicitly by the wrapper.
bool ProbeProjection(View& view,float width,float height) noexcept {
    view.projectionStatus=1;
    if(!rendererAdmitted.load() || width<640 || height<360) return false;
    __try {
        void* renderer=NativeFunction<GetObjectFn>(Native::RendererRva)();
        if(!renderer) return false;
        const float step=std::min(width,height)*0.15f;
        const Aim::Point origin{width*0.5f,height*0.5f};
        const Aim::Point screens[]={origin,{origin.x+step,origin.y},{origin.x,origin.y+step},{origin.x+step,origin.y+step}};
        Aim::Point worlds[4]{};
        for(unsigned i=0;i<4;++i) {
            std::uint64_t packed{}; std::memcpy(&packed,&screens[i],sizeof(packed));
            view.projectionStatus=2;
            if(!NativeFunction<bool(__fastcall*)(void*,float*,float*,std::uint64_t)>(Native::PickWitnessRva)(
                renderer,&worlds[i].x,&worlds[i].y,packed)) return false;
            view.projectionStatus=3;
            if(!Aim::ValidWorld(worlds[i])) return false;
        }
        const Aim::Point predicted{worlds[1].x+worlds[2].x-worlds[0].x,worlds[1].y+worlds[2].y-worlds[0].y};
        view.projectionStatus=4;
        if(Aim::DistanceSquared(predicted,worlds[3])>0.25f || Aim::DistanceSquared(worlds[0],view.player)>400 ||
            Aim::DistanceSquared(worlds[0],worlds[1])<1 || Aim::DistanceSquared(worlds[0],worlds[1])>2500) return false;
        view.projection={origin,worlds[0],worlds[1],worlds[2],step};
        Aim::Point test{};
        const bool valid=view.projection.Project(view.center,test);
        view.projectionStatus=valid?6:5;
        return valid;
    } __except(EXCEPTION_EXECUTE_HANDLER) { view.projectionStatus=7; return false; }
}

void __cdecl UiTick(const D2RL::PluginContext*,void*) noexcept {
    queued.store(false);
    if(!ready.load()) return;
    View view{};
    const auto now=GetTickCount64();
    const auto previousTick=lastUiTick.exchange(now);
    const float seconds=previousTick && now>=previousTick ? static_cast<float>(now-previousTick)/1000.0f : 0;
    if(ReadView(nullptr,view,true)) {
        requestedDistance.store(view.distance);
        view.destination=view.center;
        // The targeting-controller pointer is deliberately not retained here.
        // Lookup publishes actual native selections; this callback shows aim.
        AcquireSRWLockShared(&stateLock);
        view.projection=published.projection; view.projectionTick=published.projectionTick; view.projectionValid=published.projectionValid;
        ReleaseSRWLockShared(&stateLock);
        if(!Aim::Fresh(now,view.projectionTick,static_cast<std::uint64_t>(1000.0f/motionSettings.projectionHz))) {
            view.projectionValid=false; view.projectionTick=now;
            D2RL::Overlay::Metrics metrics{}; metrics.structSize=sizeof(metrics);
            if(overlay->getMetrics(ctx,&metrics)==D2RL::Overlay::Result::Success)
                view.projectionValid=ProbeProjection(view,metrics.screenWidth,metrics.screenHeight);
            if(lastProjectionStatus.exchange(view.projectionStatus)!=view.projectionStatus && projectionReports.load()<8) {
                projectionReports.fetch_add(1);
                char message[192];
                std::snprintf(message,sizeof(message),"[QOL/Aim] projection status=%u (0=metrics unavailable,1=renderer unavailable,2=ray miss,3=world bounds,4=geometry check,5=singular,6=ready,7=exception,8=disabled).",view.projectionStatus);
                ctx->LogInfo(message);
            }
        }
        AcquireSRWLockExclusive(&stateLock);
        Aim::Point offset=retainedOffset;
        const bool moved=view.projectionValid && cursorMotion.Advance(view.stick,view.projection,reverseY.load(),seconds,motionSettings,offset);
        const Aim::Point next{view.player.x+offset.x,view.player.y+offset.y};
        if(moved && Aim::ValidWorld(next)) {
            retainedOffset=offset;
            view.center=next; view.destination=next;
            view.distance=std::hypot(offset.x,offset.y); requestedDistance.store(view.distance);
        } else if(!view.projectionValid || !Aim::ValidWorld(next)) cursorMotion={};
        ReleaseSRWLockExclusive(&stateLock);
    } else {
        AcquireSRWLockExclusive(&stateLock); cursorMotion={}; ReleaseSRWLockExclusive(&stateLock);
    }
    PendingTeleport completed{};
    float displacement=-1;
    AcquireSRWLockExclusive(&stateLock);
    if(view.valid) ResolveSelection(view,true);
    else { snapBook={}; whirlLockActive=false; lastLookup={}; lastCastMarker={}; }
    if(pending.active && view.valid && view.playerId==pending.playerId && now>=pending.tick) {
        displacement=std::sqrt(Aim::DistanceSquared(pending.start,view.player));
        if(displacement>0.75f || now-pending.tick>1200) {
            completed=pending; pending={}; observedDisplacement=displacement;
        }
    }
    published=view;
    ReleaseSRWLockExclusive(&stateLock);
    if(completed.active) {
        char message[200];
        std::snprintf(message,sizeof(message),"[QOL/Aim] Teleport sample #%u displacement=%.2f requested=%.2f elapsed=%llu ms (single-cast test; movement can affect measurement).",
            completed.sequence,displacement,std::sqrt(Aim::DistanceSquared(completed.start,completed.requested)),static_cast<unsigned long long>(now-completed.tick));
        if(completed.sequence<=128) ctx->LogInfo(message);
    }
}

void Line(D2RL::Overlay::CanvasHandle canvas,Aim::Point a,Aim::Point b,D2RL::Overlay::Color color) noexcept {
    D2RL::Overlay::LineRequest request{sizeof(request),0,canvas,{a.x,a.y},{b.x,b.y},color,2,0};
    overlay->drawLine(ctx,&request);
}
void Cross(D2RL::Overlay::CanvasHandle canvas,Aim::Point p,D2RL::Overlay::Color color) noexcept {
    Line(canvas,{p.x-5,p.y},{p.x+5,p.y},color); Line(canvas,{p.x,p.y-5},{p.x,p.y+5},color);
}
// Muted bone/brass strokes with a dark under-stroke for terrain contrast.
void ReticleStroke(D2RL::Overlay::CanvasHandle canvas,Aim::Point a,Aim::Point b,D2RL::Overlay::Color color,float scale) noexcept {
    D2RL::Overlay::LineRequest request{sizeof(request),0,canvas,{a.x,a.y},{b.x,b.y},{0.035f,0.025f,0.015f,color.alpha*0.8f},3.2f*scale,0};
    overlay->drawLine(ctx,&request);
    request.color=color; request.thickness=1.25f*scale;
    overlay->drawLine(ctx,&request);
}
void GroundReticle(D2RL::Overlay::CanvasHandle canvas,const Aim::Projection& projection,Aim::Point world,float scale,float alpha) noexcept {
    const D2RL::Overlay::Color bone{0.76f,0.71f,0.59f,alpha};
    // Four broken ground-plane arcs: an aiming mark, not a skill-area outline.
    for(unsigned quadrant=0;quadrant<4;++quadrant) {
        Aim::Point previous{}; bool havePrevious=false;
        for(unsigned segment=0;segment<=8;++segment) {
            const float angle=static_cast<float>(quadrant)*1.5707963f+0.20f+static_cast<float>(segment)*1.17f/8;
            Aim::Point point{};
            const bool valid=projection.Project({world.x+0.65f*std::cos(angle),world.y+0.65f*std::sin(angle)},point);
            if(valid && havePrevious) ReticleStroke(canvas,previous,point,bone,scale);
            previous=point; havePrevious=valid;
        }
    }
}
void EnemyReticle(D2RL::Overlay::CanvasHandle canvas,Aim::Point point,float scale) noexcept {
    const D2RL::Overlay::Color brass{0.80f,0.61f,0.32f,0.95f};
    // Four open diamond corners around the enemy's ground position.
    for(unsigned corner=0;corner<4;++corner) {
        const float angle=static_cast<float>(corner)*1.5707963f;
        const Aim::Point tip{point.x+14*scale*std::cos(angle),point.y+9*scale*std::sin(angle)};
        const float previous=angle-1.5707963f,next=angle+1.5707963f;
        const Aim::Point a{point.x+14*scale*std::cos(previous),point.y+9*scale*std::sin(previous)};
        const Aim::Point b{point.x+14*scale*std::cos(next),point.y+9*scale*std::sin(next)};
        ReticleStroke(canvas,{tip.x+(a.x-tip.x)*0.34f,tip.y+(a.y-tip.y)*0.34f},tip,brass,scale);
        ReticleStroke(canvas,tip,{tip.x+(b.x-tip.x)*0.34f,tip.y+(b.y-tip.y)*0.34f},brass,scale);
    }
}
void Label(D2RL::Overlay::CanvasHandle canvas,Aim::Point p,const char* text,D2RL::Overlay::Color color) noexcept {
    const D2RL::Overlay::TextRequest request{sizeof(request),0,canvas,{p.x,p.y},color,18,0,text,
        static_cast<unsigned>(std::strlen(text)),0};
    overlay->drawText(ctx,&request);
}
// The reviewed Core bridge passes pointers to native PushClipRect's by-value
// float2 arguments. Override that clip only during our SDK frame callback.
// Keep the bridge's own stack entry intact; balance our extra push directly.
void* BeginDrawingClip(float width,float height) noexcept {
    if(!clipCorrectionAdmitted.load() || !Aim::Finite({width,height}) || width<=0 || height<=0) return nullptr;
    __try {
        void* draw=NativeFunction<GetObjectFn>(Native::DrawListRva)();
        if(!draw) return nullptr;
        const Aim::Point extent{width,height};
        std::uint64_t packed{}; std::memcpy(&packed,&extent,sizeof(packed));
        NativeFunction<void(__fastcall*)(void*,std::uint64_t,std::uint64_t,bool)>(Native::PushClipRva)(draw,0,packed,false);
        return draw;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}
void EndDrawingClip(void* draw) noexcept {
    if(!draw) return;
    __try { NativeFunction<void(__fastcall*)(void*)>(Native::PopClipRva)(draw); }
    __except(EXCEPTION_EXECUTE_HANDLER) {}
}
struct DrawingClip {
    void* draw{};
    ~DrawingClip() { EndDrawingClip(draw); }
};
void __cdecl Frame(const D2RL::PluginContext*,const D2RL::Overlay::Frame* frame,void*) noexcept {
    if(!frame || frame->structSize<D2RL::Overlay::FrameRequiredSize || !installed.load() || !inSession.load()) return;
    if(!queued.exchange(true) && threads->runOnUiThread(ctx,&UiTick,nullptr)!=D2RL::Threads::Result::Success) queued.store(false);
    View view{}; float actual{}; CastMarker castMarker{};
    AcquireSRWLockShared(&stateLock); view=published; actual=observedDisplacement; castMarker=lastCastMarker; ReleaseSRWLockShared(&stateLock);
    if(!Foreground() || !motionSettings.overlay) return;
    const DrawingClip clip{BeginDrawingClip(frame->screenWidth,frame->screenHeight)};
    const bool fresh=view.valid && Aim::Fresh(GetTickCount64(),view.tick);
    // Status must not depend on a successfully resolved aim or projection.
    // Keep OFF visible too, so a missing marker cannot be mistaken for OFF.
    const bool active=enabled.load();
    if(motionSettings.debugOverlay) {
    char status[144];
    std::snprintf(status,sizeof(status),"AIM TEST %s | %.1f tiles | F8 toggle | %s",active?"ON":"OFF",
        requestedDistance.load(),fresh?"controller preview":"cast a supported skill with controller");
    const D2RL::Overlay::TextRequest statusRequest{sizeof(statusRequest),0,frame->canvas,{24,70},
        {1,0.9f,0.3f,1},18,0,status,static_cast<unsigned>(std::strlen(status)),0};
    const auto textResult=overlay->drawText(ctx,&statusRequest);
    const float barStart=24,barEnd=294,barY=190;
    const float marker=barStart+(requestedDistance.load()-Aim::MinimumDistance)/(Aim::MaximumDistance-Aim::MinimumDistance)*(barEnd-barStart);
    const D2RL::Overlay::LineRequest bar{sizeof(bar),0,frame->canvas,{barStart,barY},{barEnd,barY},{1,1,1,1},3,0};
    const auto lineResult=overlay->drawLine(ctx,&bar);
    // One diagnostic per session plus explicit failures (capped) makes missing
    // presentation distinguishable from missing callbacks or invalid aim.
    if(overlayReports.load()==0 || ((textResult!=D2RL::Overlay::Result::Success || lineResult!=D2RL::Overlay::Result::Success) && overlayReports.load()<4)) {
        overlayReports.fetch_add(1);
        char message[224];
        std::snprintf(message,sizeof(message),"[QOL/Aim] overlay frame=%llu size=%.0fx%.0f textResult=%u lineResult=%u fresh=%u projection=%u rendererGuard=%u clipCorrection=%u (0=success).",
            static_cast<unsigned long long>(frame->frameNumber),frame->screenWidth,frame->screenHeight,
            static_cast<unsigned>(textResult),static_cast<unsigned>(lineResult),fresh,view.projectionValid,rendererAdmitted.load(),clip.draw!=nullptr);
        ctx->LogInfo(message);
    }
    Cross(frame->canvas,{marker,barY},{0.2f,0.8f,1,1});
    Label(frame->canvas,{barStart,barY+8},"0",{1,1,1,1});
    Label(frame->canvas,{barEnd-18,barY+8},"30",{1,1,1,1});
    }
    if(!active || !fresh) return;
    if(motionSettings.debugOverlay) {
    char text[320];
    const int length=std::snprintf(text,sizeof(text),"AIM CURSOR | %s | range %.1f / 30 | stick %+.2f,%+.2f%s\nSnap radius %.1f | %s | Teleport sample %.1f | F8 toggle  F9 recenter  F10 invert Y\nGround marker: %s",
        Aim::SkillName(view.skill),view.distance,view.stick.x,view.stick.y,reverseY.load()?" (inverted)":"",
        motionSettings.snapRadius,view.target?"TARGET LOCK":"GROUND",actual,view.projectionValid?"native XY wrapper":"unavailable (see projection status in log)");
    D2RL::Overlay::TextRequest request{sizeof(request),0,frame->canvas,{24,100},{1,0.9f,0.3f,1},18,0,text,
        static_cast<unsigned>(std::min(length,static_cast<int>(sizeof(text)-1))),0};
    overlay->drawText(ctx,&request);
    }
    if(!view.projectionValid) return;
    static Aim::DisplayProjection displayProjection{};
    view.projection=displayProjection.Update(view.projection,GetTickCount64(),motionSettings.overlaySmoothingMs);
    if(motionSettings.debugOverlay && castMarker.valid && castMarker.playerId==view.playerId && Aim::Fresh(GetTickCount64(),castMarker.tick,2500)) {
        Aim::Point castScreen{};
        if(view.projection.Project(castMarker.position,castScreen)) {
            Cross(frame->canvas,castScreen,{1,0.3f,1,1});
            Label(frame->canvas,{castScreen.x+10,castScreen.y-40},"LAST CAST",{1,0.3f,1,1});
        }
    }
    if(!motionSettings.debugOverlay) {
        const float scale=std::clamp(frame->screenHeight/1080.0f,0.75f,2.0f);
        GroundReticle(frame->canvas,view.projection,view.center,scale,view.target?0.38f:0.82f);
        if(view.target) {
            Aim::Point enemy{};
            if(view.projection.Project(view.targetPosition,enemy)) EnemyReticle(frame->canvas,enemy,scale);
            // Whirlwind's landing point is separate from the enemy lock.
            if(Aim::DistanceSquared(view.destination,view.targetPosition)>0.01f)
                GroundReticle(frame->canvas,view.projection,view.destination,scale,0.9f);
        }
        return;
    }
    const D2RL::Overlay::Color color{0.2f,0.8f,1,0.85f};
    Aim::Point playerScreen{},aimScreen{};
    if(view.projection.Project(view.player,playerScreen) && view.projection.Project(view.center,aimScreen)) {
        Line(frame->canvas,playerScreen,aimScreen,color);
        Cross(frame->canvas,aimScreen,color);
        char distanceLabel[48];
        std::snprintf(distanceLabel,sizeof(distanceLabel),"%.1f tiles",view.distance);
        Label(frame->canvas,{aimScreen.x+10,aimScreen.y-20},distanceLabel,color);
        for(float distance=5;motionSettings.debugOverlay && distance<view.distance;distance+=5) {
            const float fraction=distance/view.distance;
            Cross(frame->canvas,{playerScreen.x+(aimScreen.x-playerScreen.x)*fraction,
                playerScreen.y+(aimScreen.y-playerScreen.y)*fraction},color);
        }
    }
    if(motionSettings.debugOverlay) {
    Aim::Point previous{};
    bool havePrevious=false;
    for(unsigned i=0;i<=48;++i) {
        const float angle=static_cast<float>(i)*(6.28318530718f/48);
        const Aim::Point world{view.player.x+Aim::MaximumDistance*std::cos(angle),view.player.y+Aim::MaximumDistance*std::sin(angle)};
        Aim::Point screen{};
        if(!view.projection.Project(world,screen) || screen.x < -frame->screenWidth || screen.x > 2*frame->screenWidth ||
            screen.y < -frame->screenHeight || screen.y > 2*frame->screenHeight) { havePrevious=false; continue; }
        if(havePrevious) Line(frame->canvas,previous,screen,color);
        previous=screen; havePrevious=true;
    }
    }
    if(motionSettings.snapping && Aim::SnapSkill(view.skill)) {
        Aim::Point prior{}; bool priorValid=false;
        for(unsigned i=0;i<=48;++i) {
            const float angle=static_cast<float>(i)*(6.28318530718f/48);
            Aim::Point point{};
            const bool valid=view.projection.Project({view.center.x+motionSettings.snapRadius*std::cos(angle),view.center.y+motionSettings.snapRadius*std::sin(angle)},point);
            if(valid && priorValid) Line(frame->canvas,prior,point,{1,0.8f,0.2f,0.8f});
            prior=point; priorValid=valid;
        }
        Aim::Point snapped{};
        if(view.target && view.projection.Project(view.center,aimScreen) && view.projection.Project(view.destination,snapped)) {
            Line(frame->canvas,aimScreen,snapped,{0.3f,1,0.3f,1});
            Label(frame->canvas,{snapped.x+10,snapped.y+10},view.skill==Aim::Whirlwind && motionSettings.whirlwindPassThrough && motionSettings.whirlwindPassThroughDistance>0?"PASS THROUGH":"SNAP",{0.3f,1,0.3f,1});
        }
    }
    Aim::Point center{};
    if(view.projection.Project(view.center,center)) Cross(frame->canvas,center,color);
    if(view.target && view.projection.Project(view.destination,center)) Cross(frame->canvas,center,{0.3f,1,0.3f,1});
}

D2RL::Input::ActionResult __cdecl Control(const D2RL::PluginContext*,const D2RL::Input::ActionEvent* event,void* user) noexcept {
    if(!ready.load() || !event || event->kind!=D2RL::Input::ActionEventKind::Pressed) return D2RL::Input::ActionResult::Ignored;
    switch(reinterpret_cast<std::uintptr_t>(user)) {
        case 1: enabled.store(!enabled.load());
            AcquireSRWLockExclusive(&stateLock); snapBook={}; whirlLockActive=false; lastLookup={}; lastCastMarker={}; ReleaseSRWLockExclusive(&stateLock); ctx->LogInfo(enabled.load()?"[QOL/Aim] Enabled.":"[QOL/Aim] Disabled; native aim restored."); break;
        case 2:
            AcquireSRWLockExclusive(&stateLock); retainedOffsetValid=false; cursorMotion={}; snapBook={}; whirlLockActive=false; lastLookup={}; lastCastMarker={}; ReleaseSRWLockExclusive(&stateLock);
            requestedDistance.store(Aim::DefaultDistance); ctx->LogInfo("[QOL/Aim] Recenter pending: 20 tiles along current facing."); break;
        case 3: reverseY.store(!reverseY.load()); ctx->LogInfo(reverseY.load()?"[QOL/Aim] Y inverted.":"[QOL/Aim] Y normal."); break;
    }
    return D2RL::Input::ActionResult::Handled;
}

bool ReadBytes(std::uintptr_t address,void* data,std::size_t size) noexcept {
    SIZE_T read{};
    return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),data,size,&read) && read==size;
}
bool AdmitContact(std::uintptr_t callRva) noexcept {
    unsigned char call[5]{};
    if(!ReadBytes(ctx->exeBase+callRva,call,sizeof(call)) || call[0]!=0xe8) return false;
    std::int32_t displacement{};std::memcpy(&displacement,call+1,4);
    const auto target=static_cast<std::uintptr_t>(static_cast<std::int64_t>(ctx->exeBase+callRva+5)+displacement);
    if(target==ctx->exeBase+0x34bc90) return true;
    // Internal ownership is exact function identity, not a self-referential DLL hash.
    unsigned char relay[14]{};
    constexpr unsigned char prefix[]={0xff,0x25,0,0,0,0};
    if(!ReadBytes(target,relay,sizeof(relay)) || std::memcmp(relay,prefix,sizeof(prefix))) return false;
    std::uintptr_t destination{};std::memcpy(&destination,relay+6,8);
    return QolPortal::OwnsContactDestination(destination);
}
bool AdmitEnumeration() noexcept {
    std::array<unsigned char,sizeof(Native::UnitTestBytes)> actual{};
    return ReadBytes(ctx->exeBase+Native::UnitTestRva,actual.data(),actual.size()) &&
        Aim::SameEnumeration(actual,Native::UnitTestBytes) && AdmitContact(0x1922cd) && AdmitContact(0x192378);
}

bool Install() noexcept {
    for(const auto& site:Native::Sites) {
        if(site.rva==Native::UnitTestRva) continue; // separately admits known QOL contact relays
        if(!ctx->CheckExpectedBytes(site.rva,site.bytes,site.size)) {
            char message[128]; std::snprintf(message,sizeof(message),"[QOL/Aim] Guard mismatch: %s at +0x%llX; aim module inert.",site.name,static_cast<unsigned long long>(site.rva));
            ctx->LogWarn(message); return false;
        }
    }
    if(!AdmitEnumeration()) {ctx->LogWarn("[QOL/Aim] UnitTestSelect changed beyond reviewed QOL contact relays; aim module inert."); return false;}
    rendererAdmitted.store(ctx->CheckExpectedBytes(Native::RendererRva,Native::RendererBytes,sizeof(Native::RendererBytes)) &&
        ctx->CheckExpectedBytes(Native::UnprojectRva,Native::UnprojectBytes,sizeof(Native::UnprojectBytes)) &&
        ctx->CheckExpectedBytes(Native::PickWitnessRva,Native::PickWitnessBytes,sizeof(Native::PickWitnessBytes)));
    clipCorrectionAdmitted.store(ctx->CheckExpectedBytes(Native::DrawListRva,Native::DrawListBytes,sizeof(Native::DrawListBytes)) &&
        ctx->CheckExpectedBytes(Native::PushClipRva,Native::PushClipBytes,sizeof(Native::PushClipBytes)) &&
        ctx->CheckExpectedBytes(Native::PopClipRva,Native::PopClipBytes,sizeof(Native::PopClipBytes)));
    // The stable QOL hooks own action handlers and ScoreUnit. The aim module owns
    // different entry points and never accepts somebody else's patched prefix.
    if(!ctx->InstallInlineHook(Native::LookupRva,Native::LookupBytes,15,&Lookup,&originalLookup) ||
        !ctx->InstallInlineHook(Native::UnitTestRva,Native::UnitTestBytes,14,&UnitTest,&originalUnitTest) ||
        !ctx->InstallInlineHook(Native::PointScoreRva,Native::PointScoreBytes,14,&PointScore,&originalPointScore) ||
        !ctx->InstallInlineHook(Native::CastRva,Native::CastBytes,15,&Cast,&originalCast) ||
        !ctx->InstallInlineHook(Native::SelectedRva,Native::SelectedBytes,14,&Selected,&originalSelected)) {
        ctx->LogWarn("[QOL/Aim] Hook admission failed; installed wrappers remain pass-through."); return false;
    }
    installed.store(true);
    ctx->LogInfo("[QOL/Aim] Integrated aim installed: 10 allowlisted skills; configurable snapping; Teleport/Leap ground-only; Whirlwind snap/pass-through; projection sampled at configured rate.");
    return true;
}
void __cdecl Lifecycle(const D2RL::PluginContext*,const D2RL::Lifecycle::GameplayEvent* event,void*) noexcept {
    if(!event || !ready.load()) return;
    inSession.store(event->kind!=D2RL::Lifecycle::GameplayEventKind::GameLeft);
    requestedDistance.store(Aim::DefaultDistance); previewSkill.store(-1); lastUiTick.store(0); queued.store(false); overlayReports.store(0);
    projectionReports.store(0); previewCandidateReports.store(0); lastProjectionStatus.store(99);
    AcquireSRWLockExclusive(&stateLock); published={}; pending={}; observedDisplacement=-1; retainedOffsetValid=false; cursorMotion={}; snapBook={}; whirlLockActive=false; lastLookup={}; lastCastMarker={}; ReleaseSRWLockExclusive(&stateLock);
    if(inSession.load() && !installed.load()) Install();
}
}

bool QolAim::Initialize(const D2RL::PluginContext* context,bool qolEnabled) noexcept {
    if(!context || !context->GetApi() || !context->exeBase) return false;
    ctx=context;
    if(!qolEnabled) return true;
    std::array<char,16384> config{}; bool configured=false;
    if(!ctx->ReadConfig(config.data(),static_cast<unsigned>(config.size()-1)) ||
        !Aim::ParseQolSettings(config.data(),motionSettings,configured)) {
        ctx->LogWarn("[QOL/Aim] Invalid/unavailable aim configuration; aim disabled.");
        return false;
    }
    if(!configured) { ctx->LogInfo("[QOL/Aim] Disabled by configuration."); return true; }
    if(GetModuleHandleW(L"Controller Aim Test.dll")) {
        ctx->LogWarn("[QOL/Aim] Standalone aim module loaded; integrated aim disabled to avoid duplicate hooks.");
        return false;
    }
    char configMessage[224];
    std::snprintf(configMessage,sizeof(configMessage),"[QOL/Aim] Cursor settings: deadzone=%.2f initial_speed=%.2f maximum_speed=%.2f acceleration_seconds=%.2f (restart to reload).",
        motionSettings.deadzone,motionSettings.initialSpeed,motionSettings.maximumSpeed,motionSettings.accelerationSeconds);
    ctx->LogInfo(configMessage);
    char featureMessage[256];
    std::snprintf(featureMessage,sizeof(featureMessage),"[QOL/Aim] Features: snapping=%u radius=%.2f switch_advantage=%.2f overlay=%u debug=%u projection_hz=%.1f smoothing_ms=%.1f.",
        motionSettings.snapping,motionSettings.snapRadius,motionSettings.switchAdvantage,motionSettings.overlay,motionSettings.debugOverlay,motionSettings.projectionHz,motionSettings.overlaySmoothingMs);
    ctx->LogInfo(featureMessage);
    std::snprintf(featureMessage,sizeof(featureMessage),"[QOL/Aim] Whirlwind: pass_through_enabled=%u pass_through_distance=%.2f (restart to reload).",motionSettings.whirlwindPassThrough,motionSettings.whirlwindPassThroughDistance);
    ctx->LogInfo(featureMessage);
    constexpr unsigned char hash[]={0x2a,0x86,0x8d,0x01,0x3d,0x2e,0x08,0x30,0xbd,0x2d,0x9e,0x04,0xb9,0x18,0xb1,0x9e,0x46,0xa7,0x3c,0xf7,0x26,0xc8,0x33,0xe7,0x0d,0x08,0x9b,0x94,0x8f,0xde,0xb5,0xa2};
    const auto core=GetModuleHandleW(L"D2RCore.dll");
    if(!QolCore::VerifyFileHash(core,hash)) {ctx->LogWarn("[QOL/Aim] Unreviewed core; refusing aim module."); return false;}
    inGame=reinterpret_cast<InGameFn>(GetProcAddress(core,"IsInGame"));
    const D2RL::LifecycleService* lifecycle{};
    const D2RL::InputService* input{};
    if(!inGame || ctx->QueryService(&overlay)!=D2RL::ServiceQueryResult::Success || !D2RL::HasOverlayServiceField(overlay,D2RL::OverlayServiceRequiredSize) ||
        ctx->QueryService(&threads)!=D2RL::ServiceQueryResult::Success || !D2RL::HasThreadServiceField(threads,D2RL::ThreadServiceRequiredSize) ||
        ctx->QueryService(&lifecycle)!=D2RL::ServiceQueryResult::Success || !D2RL::HasLifecycleServiceField(lifecycle,D2RL::LifecycleServiceRequiredSize) ||
        ctx->QueryService(&input)!=D2RL::ServiceQueryResult::Success || !D2RL::HasInputServiceField(input,D2RL::InputServiceRequiredSize) ||
        !overlay->registerFrameCallback || !overlay->getMetrics || !overlay->drawLine || !overlay->drawText || !threads->runOnUiThread || !lifecycle->registerGameplayEventListener || !input->registerAction) return false;
    for(const auto kind:{D2RL::Lifecycle::GameplayEventKind::GameJoined,D2RL::Lifecycle::GameplayEventKind::LocalPlayerReady,D2RL::Lifecycle::GameplayEventKind::GameLeft}) {
        const D2RL::Lifecycle::GameplayEventListener listener{sizeof(listener),0,kind,0,&Lifecycle,nullptr};
        D2RL::Lifecycle::ListenerHandle handle{};
        if(lifecycle->registerGameplayEventListener(ctx,&listener,&handle)!=D2RL::Lifecycle::Result::Success || !handle) return false;
    }
    const D2RL::Overlay::CallbackRegistration registration{sizeof(registration),0,D2RL::Overlay::Phase::AfterGameUi,0,&Frame,nullptr};
    D2RL::Overlay::CallbackHandle frameHandle{};
    if(overlay->registerFrameCallback(ctx,&registration,&frameHandle)!=D2RL::Overlay::Result::Success || !frameHandle) return false;
    const D2RL::Input::Key keys[]={D2RL::Input::Key::F8,D2RL::Input::Key::F9,D2RL::Input::Key::F10};
    const char* ids[]={"aim-toggle","aim-recenter","aim-invert-y"};
    const char* names[]={"Toggle controller aim aim module","Recenter free aim at 20","Invert aim-distance stick Y"};
    for(unsigned i=0;i<3;++i) {
        const D2RL::Input::ActionRegistration action{sizeof(action),0,ids[i],names[i],"Controller QOL Aim",
            {keys[i],D2RL::Input::Modifier::None},{D2RL::Input::Key::None,D2RL::Input::Modifier::None},&Control,reinterpret_cast<void*>(static_cast<std::uintptr_t>(i+1))};
        D2RL::Input::ActionHandle handle{};
        if(input->registerAction(ctx,&action,&handle)!=D2RL::Input::Result::Success || !handle) return false;
    }
    enabled.store(true); ready.store(true);
    inSession.store(inGame());
    if(inSession.load()) Install();
    ctx->LogInfo("[QOL/Aim] Ready; F8 toggle, F9 recenter, F10 invert. Settings load on restart.");
    return true;
}
void QolAim::Shutdown() noexcept {
    ready.store(false);
    enabled.store(false); installed.store(false); inSession.store(false);
    // SDK removes owned callbacks/hooks. Keep context and original pointers
    // valid until that removal completes; every remaining wrapper passes through.
}

// Internal ownership query used by QOL's existing Guided Arrow correction.
// Atomic state only: safe from QOL's controller action thread, no native reads.
bool QolAim::OwnsGuidedArrow() noexcept {
    return installed.load() && enabled.load() && inSession.load();
}
