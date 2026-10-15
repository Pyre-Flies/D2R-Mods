#include <D2RLPlugin/api.h>
#include "controls.h"
#include "../../shared/chat_config.h"
#include "style.h"
#include "native_reader.h"
#include "combat.h"
#include "critical_probe.h"
#include <D2RLChat/native_commands.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <span>

namespace {
const D2RL::PluginContext* ctx{};
const D2RL::OverlayService* overlay{};
const D2RL::ThreadService* threads{};
const D2RL::LifecycleService* lifecycle{};
D2RL::Overlay::CallbackHandle frameHandle{};
std::array<D2RL::Lifecycle::ListenerHandle,3> lifeHandles{};
D2RL::Lifecycle::ListenerHandle deathHandle{};
Damage::Settings settings{};
Damage::Model model{};
std::mutex stateMutex;
Damage::Point player{};
bool playerValid{};
std::size_t enemyCount{};
std::atomic<bool> ready{}, enabled{}, active{}, fallingMotion{};
std::atomic<std::uint64_t> generation{1}, queued{};
std::atomic<std::uint32_t> playerId{};
std::atomic<unsigned> reports{}, failures{};
std::uint64_t lastQueue{};
D2RLChat::NativeCommands chatCommands;
std::atomic<bool> combatReady{};
std::atomic<bool> poisonReady{};
std::atomic<bool> criticalReady{};
void* combatOriginal{};
std::atomic<bool> capture{};
std::atomic<unsigned> captureRecords{};
bool TraceReady() noexcept {return capture.load() && captureRecords.load()<128;}
void Trace(const char* message) noexcept {
    if(!capture.load()) return;
    const auto record=captureRecords.fetch_add(1);
    if(record<128) ctx->LogInfo(message);
    if(record==127) {capture.store(false);ctx->LogInfo("Floating Damage capture: stopped at 128 log records.");}
}
void TracePacket(const Damage::CombatPacket& packet,const char* phase,std::uint32_t id) noexcept {
    if(!TraceReady()) return;
    char message[640]{};
    std::snprintf(message,sizeof(message),"DamageCapture packet %s enemy=%u physical=%d fire=%d cold=%d lightning=%d magic=%d poison=%d poison_length=%d total=%d vector_count=%llu header0=%08X result4=%04X extra6=%04X crit_guard=%u critical_candidate=%u",
        phase,id,packet.Value(0x18),packet.Value(0x20),packet.Value(0x34),packet.Value(0x2c),packet.Value(0x30),packet.Value(0x38),packet.Value(0x3c),packet.Value(0x134),
        static_cast<unsigned long long>(packet.Wide(0x48)),static_cast<unsigned>(packet.Value(0)),static_cast<unsigned>(packet.ResultFlags()),
        static_cast<unsigned>(static_cast<std::uint32_t>(packet.Value(4))>>16),criticalReady.load()?1U:0U,
        criticalReady.load() && packet.CriticalMarker()?1U:0U);
    Trace(message);
    struct Entry {std::int32_t tag,rate,length;};std::array<Entry,4> entries{};
    const auto count=packet.Wide(0x48),address=packet.Wide(0x40);
    // A 12-byte stride is witnessed at 0x44D1F0..0x44D226. Copy at most four
    // entries, synchronously; record tags remain unnamed candidates.
    if(count>0 && count<=4 && Damage::Native::Bytes(static_cast<std::uintptr_t>(address),entries.data(),static_cast<std::size_t>(count)*sizeof(Entry))) {
        for(std::uint64_t i=0;i<count;++i) {
            std::snprintf(message,sizeof(message),"DamageCapture vector %s enemy=%u index=%llu tag=%d rate_candidate=%d length_candidate=%d",phase,id,
                static_cast<unsigned long long>(i),entries[static_cast<std::size_t>(i)].tag,entries[static_cast<std::size_t>(i)].rate,entries[static_cast<std::size_t>(i)].length);
            Trace(message);
        }
    }
}
void TracePoison(std::uintptr_t unit,std::uint32_t id,std::uintptr_t owner) noexcept {
    if(!TraceReady() || !poisonReady.load()) return;
    const auto state=Damage::Native::ReadPoisonState(unit);
    if(!state.found) return;
    Damage::Native::Stats stats{};std::uint32_t frame{};
    const bool hpValid=Damage::Native::ReadStats(unit,stats),frameValid=Damage::Native::ReadGameFrame(owner,frame);
    std::int32_t base{};const bool baseValid=Damage::Native::ReadBaseRegen(unit,base);
    const auto stamp=Damage::Native::ReadPoison(unit,stats,frame,frameValid);char message[384]{};
    std::snprintf(message,sizeof(message),"DamageCapture poison enemy=%u readable=%u state2_found=%u stat74=%lld composite74=%lld base74=%d base_valid=%u hp_valid=%u frame=%u frame_valid=%u expires=%.0f rate=%lld valid=%u",
        id,state.readable?1U:0U,state.found?1U:0U,static_cast<long long>(state.rate),static_cast<long long>(stats.regen),base,baseValid?1U:0U,hpValid?1U:0U,
        frame,frameValid?1U:0U,static_cast<double>(state.expiration),static_cast<long long>(stamp.rate),stamp.valid?1U:0U);Trace(message);
}

std::uintptr_t __fastcall CombatHook(void* game,void* attacker,void* defender,std::uint32_t missile,void* damage) noexcept {
    const auto epoch=generation.load(); const auto started=GetTickCount64();
    const bool tracing=TraceReady();
    const auto before=combatReady.load() && active.load() && ((enabled.load() && (settings.elementColors || settings.criticalHits)) || tracing)?
        Damage::Combat::Before(ctx->exeBase,game,defender,poisonReady.load()):Damage::Combat::Observation{};
    if(tracing && before.valid) {
        Damage::CombatPacket packet{};
        if(Damage::Native::Bytes(reinterpret_cast<std::uintptr_t>(damage),packet.bytes.data(),packet.bytes.size())) TracePacket(packet,"before",before.target.id);
    }
    // Preserve the proven native call: all arguments, exactly once, same RAX.
    const auto result=reinterpret_cast<Damage::Combat::Apply>(combatOriginal)(game,attacker,defender,missile,damage);
    if(before.valid && combatReady.load() && active.load() && generation.load()==epoch) {
        Damage::CombatPacket packet{}; Damage::Native::Stats hp{};
        const bool packetValid=Damage::Native::Bytes(reinterpret_cast<std::uintptr_t>(damage),packet.bytes.data(),packet.bytes.size());
        const auto mask=packetValid?
            packet.Elements():Damage::Unsupported;
        const bool critical=packetValid && criticalReady.load() && packet.CriticalMarker();
        const bool hpValid=Damage::Combat::After(before,hp);
        std::uint32_t frame{};const bool frameValid=poisonReady.load() && Damage::Native::ReadGameFrame(reinterpret_cast<std::uintptr_t>(game),frame);
        const auto poison=hpValid?Damage::Native::ReadPoison(before.target.identity,hp,frame,frameValid):Damage::PoisonStamp{};
        auto observed=Damage::Model::HitResult::Ignored;
        try {
            std::lock_guard lock(stateMutex);
            if(enabled.load() && active.load() && generation.load()==epoch)
                observed=model.RecordHit(before.target,hp.hp,hpValid,mask,reinterpret_cast<std::uintptr_t>(game),epoch,started,GetTickCount64(),
                    packetValid && packet.Value(0x134)>0 && static_cast<std::int64_t>(packet.Value(0x134))>=before.target.hp-255,poison,critical);
        } catch(...) {} // Observation failures cannot alter combat.
        if(tracing) {
            char message[640]{};
            std::snprintf(message,sizeof(message),"DamageCapture hit start=%llu end=%llu thread=%lu enemy=%u class=%u hp_before=%lld hp_after=%lld hp_after_valid=%u packet_valid=%u mask=%u critical=%u outcome=%s",
                static_cast<unsigned long long>(started),static_cast<unsigned long long>(GetTickCount64()),GetCurrentThreadId(),before.target.id,before.target.classId,
                static_cast<long long>(before.target.hp),static_cast<long long>(hp.hp),hpValid?1U:0U,packetValid?1U:0U,mask,critical?1U:0U,Damage::Model::ResultName(observed));
            Trace(message);if(packetValid) TracePacket(packet,"after",before.target.id);
            TracePoison(before.target.identity,before.target.id,reinterpret_cast<std::uintptr_t>(game));
        }
    }
    return result;
}

bool Projection(float width,float height,Damage::Point anchor,Damage::Projection& projection) noexcept {
    if(width<640 || height<360 || !Damage::Valid(anchor)) return false;
    __try {
        const auto base=ctx->exeBase;
        auto renderer=reinterpret_cast<void*(__fastcall*)()>(base+Damage::Native::RendererRva)();
        if(!renderer) return false;
        const float step=std::min(width,height)*0.15f;
        const Damage::Point origin{width*0.5f,height*0.5f};
        const Damage::Point screens[]={origin,{origin.x+step,origin.y},{origin.x,origin.y+step},{origin.x+step,origin.y+step}};
        Damage::Point worlds[4]{};
        for(unsigned i=0;i<4;++i) {
            std::uint64_t packed{}; std::memcpy(&packed,&screens[i],sizeof(packed));
            if(!reinterpret_cast<bool(__fastcall*)(void*,float*,float*,std::uint64_t)>(base+Damage::Native::PickWitnessRva)(renderer,&worlds[i].x,&worlds[i].y,packed) || !Damage::Valid(worlds[i])) return false;
        }
        const Damage::Point predicted{worlds[1].x+worlds[2].x-worlds[0].x,worlds[1].y+worlds[2].y-worlds[0].y};
        const auto distance=[](Damage::Point a,Damage::Point b){return (a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y);};
        if(distance(predicted,worlds[3])>0.25f || distance(worlds[0],anchor)>400 || distance(worlds[0],worlds[1])<1 || distance(worlds[0],worlds[1])>2500) return false;
        projection={origin,worlds[0],worlds[1],worlds[2],step};
        Damage::Point test{}; return projection.Project(anchor,test);
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
void* BeginClip(float width,float height) noexcept {
    __try {
        const auto base=ctx->exeBase;
        void* list=reinterpret_cast<void*(__fastcall*)()>(base+Damage::Native::DrawListRva)();
        if(!list) return nullptr;
        const Damage::Point extent{width,height}; std::uint64_t packed{}; std::memcpy(&packed,&extent,sizeof(packed));
        reinterpret_cast<void(__fastcall*)(void*,std::uint64_t,std::uint64_t,bool)>(base+Damage::Native::PushClipRva)(list,0,packed,false);
        return list;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return nullptr;}
}
void EndClip(void* list) noexcept {
    if(!list) return;
    __try {reinterpret_cast<void(__fastcall*)(void*)>(ctx->exeBase+Damage::Native::PopClipRva)(list);}
    __except(EXCEPTION_EXECUTE_HANDLER) {}
}
struct Clip {void* list;~Clip(){EndClip(list);}};

void Collect(std::uint64_t epoch) noexcept {
    try {
        if(enabled.load() && active.load() && generation.load()==epoch) {
            Damage::Native::Snapshot snapshot{};
            const bool valid=Damage::Native::SnapshotGame(ctx->exeBase,playerId.load(),snapshot,poisonReady.load());
            const auto now=GetTickCount64();
            std::lock_guard lock(stateMutex);
            if(enabled.load() && active.load() && generation.load()==epoch) {
                if(valid) {
                    model.Sample(std::span(snapshot.monsters.data(),snapshot.count),snapshot.game,epoch,now,settings);
                    if(TraceReady()) for(const auto& n:model.numbers) if(n.raw && n.last==now) {
                        char message[256]{};
                        std::snprintf(message,sizeof(message),"DamageCapture number tick=%llu enemy=%u from_hp=%lld to_hp=%lld raw=%lld mask=%u eligible=%u critical=%u emphasized=%u",
                            static_cast<unsigned long long>(now),n.target.id,static_cast<long long>(n.fromHp),static_cast<long long>(n.target.hp),static_cast<long long>(n.raw),n.elements,n.colorEligible?1U:0U,n.critical?1U:0U,Damage::Emphasized(n,settings)?1U:0U);
                        Trace(message);
                        TracePoison(n.target.identity,n.target.id,snapshot.game);
                    }
                    player=snapshot.player; playerValid=snapshot.playerValid; enemyCount=snapshot.count;
                    if(settings.debug && reports.load()<12) {
                        for(const auto& n:model.numbers) if(n.raw && n.last==now && reports.fetch_add(1)<12) {
                            char message[160]{};
                            std::snprintf(message,sizeof(message),"Floating Damage: enemy=%u observed total=%.3f HP (raw=%lld).",n.target.id,static_cast<double>(n.raw)/256.0,static_cast<long long>(n.raw));
                            ctx->LogInfo(message);
                        }
                    }
                } else {
                    model.Clear(); playerValid=false; enemyCount=0;
                    if(failures.fetch_add(1)==0) ctx->LogWarn("Floating Damage: server snapshot unavailable or layout refused; no numbers emitted.");
                }
            }
        }
    } catch(...) {ctx->LogWarn("Floating Damage: sample failed; game state was not modified.");}
}
void __cdecl Sample(const D2RL::PluginContext*,void* token) noexcept {
    const auto epoch=static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(token));
    Collect(epoch);
    auto expected=epoch; queued.compare_exchange_strong(expected,0);
}
void __cdecl Death(const D2RL::PluginContext*,const D2RL::Lifecycle::MonsterDeathEvent* event,void*) noexcept {
    if(!enabled.load() || !active.load() || !D2RL::Lifecycle::HasMonsterDeathEventField(event,D2RL::Lifecycle::MonsterDeathEventRequiredSize) ||
        event->monster.type!=D2RL::Lifecycle::UnitType::Monster) return;
    try {
        const auto epoch=generation.load();
        std::uintptr_t owner{};
        if(!Damage::Native::Field(ctx->exeBase+Damage::Native::GameGlobalRva,owner)) return;
        std::uint32_t frame{};const bool frameValid=poisonReady.load() && Damage::Native::ReadGameFrame(owner,frame);
        std::int64_t remaining{};
        {
            std::lock_guard lock(stateMutex);
            if(enabled.load() && active.load() && generation.load()==epoch)
                remaining=model.Death(event->monster.id,event->monster.classId,owner,epoch,GetTickCount64(),settings,frame,frameValid);
        }
        if(settings.debug && remaining>0 && reports.fetch_add(1)<12) {
            char message[160]{};
            std::snprintf(message,sizeof(message),"Floating Damage: enemy=%u death remainder=%.3f HP (raw=%lld).",event->monster.id,static_cast<double>(remaining)/256.0,static_cast<long long>(remaining));
            ctx->LogInfo(message);
        }
        if(TraceReady()) {
            unsigned mask{}; {std::lock_guard lock(stateMutex);for(const auto& n:model.numbers) if(n.raw && n.target.id==event->monster.id && n.target.hp==0) mask|=n.elements;}
            char message[256]{};
            std::snprintf(message,sizeof(message),"DamageCapture death tick=%llu thread=%lu enemy=%u class=%u remaining=%lld mask=%u frame=%u frame_valid=%u",
                static_cast<unsigned long long>(GetTickCount64()),GetCurrentThreadId(),event->monster.id,event->monster.classId,static_cast<long long>(remaining),mask,frame,frameValid?1U:0U);
            Trace(message);
        }
    } catch(...) {ctx->LogWarn("Floating Damage: death observation failed; game state was not modified.");}
}
void Text(D2RL::Overlay::CanvasHandle canvas,Damage::Point position,const char* text,float size,D2RL::Overlay::Color color) noexcept {
    const D2RL::Overlay::TextRequest request{sizeof(request),0,canvas,{position.x,position.y},color,size,0,text,static_cast<unsigned>(std::strlen(text)),0};
    overlay->drawText(ctx,&request);
}
bool Backing(D2RL::Overlay::CanvasHandle canvas,Damage::Point position,const D2RL::Overlay::TextMetrics& metrics,unsigned elements,float alpha,float scale=1.0f) noexcept {
    if(!Damage::ShowBacking(elements,settings) || !overlay || !overlay->drawFilledRectangle ||
        !std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(metrics.width) || !std::isfinite(metrics.height) ||
        metrics.width<=0 || metrics.height<=0 || !std::isfinite(alpha) || alpha<=0) return false;
    const float stroke=static_cast<float>(Damage::OutlineRadius(settings,scale));
    const float x=settings.backingPaddingX*scale+stroke,y=settings.backingPaddingY*scale+stroke;
    const D2RL::Overlay::FilledRectangleRequest request{sizeof(request),0,canvas,
        {position.x-x,position.y-y,position.x+metrics.width+x,position.y+metrics.height+y},
        {settings.backingColor[0],settings.backingColor[1],settings.backingColor[2],alpha*settings.backingOpacity},settings.backingRounding*scale,0};
    return overlay->drawFilledRectangle(ctx,&request)==D2RL::Overlay::Result::Success;
}
void DrawNumber(D2RL::Overlay::CanvasHandle canvas,Damage::Point position,const Damage::Number& number,float alpha,float scale=1.0f) noexcept {
    char text[64]{};Damage::FormatNumber(number,settings,text);
    const float size=Damage::TextSize(number,settings,scale);
    D2RL::Overlay::TextMetrics metrics{};metrics.structSize=sizeof(metrics);
    const D2RL::Overlay::MeasureTextRequest request{sizeof(request),0,canvas,size,0,text,static_cast<unsigned>(std::strlen(text)),0};
    if(overlay->measureText(ctx,&request,&metrics)!=D2RL::Overlay::Result::Success) return;
    position.x-=metrics.width*0.5f;
    Backing(canvas,position,metrics,number.elements,alpha,scale);
    // Approximate a soft offset shadow using weighted glyph passes;
    // the public overlay API has no text blur/shadow shader.
    if(settings.shadow && settings.shadowOpacity>0) {
        const auto radius=static_cast<int>(settings.shadowSoftness);
        for(int x=-radius;x<=radius;++x) for(int y=-radius;y<=radius;++y)
            Text(canvas,{position.x+(settings.shadowX+static_cast<float>(x))*scale,position.y+(settings.shadowY+static_cast<float>(y))*scale},
                text,size,{settings.shadowColor[0],settings.shadowColor[1],settings.shadowColor[2],
                    alpha*settings.shadowOpacity*Damage::ShadowWeight(x,y,settings.shadowSoftness)});
    }
    // Fill every offset in the configured stroke so the outline
    // stays continuous around corners and inside digit counters.
    const auto width=static_cast<int>(Damage::OutlineRadius(settings,scale));
    for(int x=-width;x<=width;++x) for(int y=-width;y<=width;++y) if(x || y)
        Text(canvas,{position.x+static_cast<float>(x),position.y+static_cast<float>(y)},text,size,
            {settings.outlineColor[0],settings.outlineColor[1],settings.outlineColor[2],alpha});
    const auto color=Damage::TextColor(number.elements,settings);
    Text(canvas,position,text,size,{color[0],color[1],color[2],alpha});
}
D2RL::ConsoleCommandResult __cdecl Command(D2R::Game::Client*,const D2RL::ConsoleCommandContext* command,void*) noexcept {
    if(!command || command->contextSize<D2RL::ConsoleCommandContextSize || command->plugin!=ctx ||
        command->argsLength>256 || (!command->args && command->argsLength)) return D2RL::ConsoleCommandResult::Failed;
    const auto request=Damage::ParseRequest({command->args?command->args:"",command->argsLength});
    const auto action=request.action;
    if(action==Damage::Control::Invalid) {
        ctx->WriteConsoleMessage("Usage: /damagenumbers [on|off|status] or motion/capture [on|off|status]; no argument toggles.",D2RL::ConsoleMessageKind::Warning);
        return D2RL::ConsoleCommandResult::InvalidArguments;
    }
    try {
        const char* message{}; bool changed{}; bool available{};
        {
            std::lock_guard lock(stateMutex);
            available=ready.load();
            if(available && request.capture) {
                available=combatReady.load();
                if(available) {
                    auto value=capture.load();changed=Damage::UpdateFlag(action,value);
                    if(changed && value) captureRecords.store(0);
                    capture.store(value);message=value?"Floating Damage capture: on (up to 128 log records).":"Floating Damage capture: off.";
                } else message="Floating Damage capture: combat observation unavailable.";
            } else if(available && request.motion) {
                auto value=fallingMotion.load();
                changed=Damage::UpdateFlag(action,value);
                if(action!=Damage::Control::Status && !ChatConfig::Save(ctx,"floating-damage",{{"falling_motion",value}},16384)) {
                    ctx->WriteConsoleMessage("Floating Damage: config save failed; motion setting unchanged.",D2RL::ConsoleMessageKind::Warning);
                    return D2RL::ConsoleCommandResult::Failed;
                }
                settings.fallingMotion=value;fallingMotion.store(value);
                message=value?"Floating Damage motion: rise, fall and drift.":"Floating Damage motion: straight rise.";
            } else if(available) {
                auto value=enabled.load();
                Damage::UpdateFlag(action,value);
                if(action!=Damage::Control::Status && !ChatConfig::Save(ctx,"floating-damage",{{"enabled",value}},16384)) {
                    ctx->WriteConsoleMessage("Floating Damage: config save failed; enabled setting unchanged.",D2RL::ConsoleMessageKind::Warning);
                    return D2RL::ConsoleCommandResult::Failed;
                }
                auto previous=enabled.load();changed=Damage::ApplyControl(action,previous,model);
                settings.enabled=value;
                if(changed) {
                    generation.fetch_add(1); queued.store(0);
                    playerValid=false; enemyCount=0; reports.store(0); failures.store(0);
                    enabled.store(value);
                }
                message=value?"Floating Damage: enabled.":"Floating Damage: disabled.";
            } else message="Floating Damage: unavailable for this executable/Core; compatibility checks must pass.";
        }
        ctx->WriteConsoleMessage(message,available?D2RL::ConsoleMessageKind::Output:D2RL::ConsoleMessageKind::Warning);
        if(changed) ctx->LogInfo(message);
        return available?D2RL::ConsoleCommandResult::Handled:D2RL::ConsoleCommandResult::Failed;
    } catch(...) {return D2RL::ConsoleCommandResult::Failed;}
}
void ChatCommand(const char* args,void*) noexcept {
    if(!ctx) return;
    const auto length=args?strnlen_s(args,257):0;
    if(length>256) return;
    const D2RL::ConsoleCommandContext command{.contextSize=D2RL::ConsoleCommandContextSize,
        .argsLength=static_cast<std::uint32_t>(length),.args=args,.plugin=ctx};
    Command(nullptr,&command,nullptr);
}
void __cdecl Frame(const D2RL::PluginContext*,const D2RL::Overlay::Frame* frame,void*) noexcept {
    if(!enabled.load() || !active.load() || !frame || frame->structSize<sizeof(*frame)) return;
    try {
        const auto now=GetTickCount64();
        if(now-lastQueue>=30) {
            const auto epoch=generation.load(); std::uint64_t expected=0;
            if(queued.compare_exchange_strong(expected,epoch)) {
                lastQueue=now;
                if(threads->runOnGameThread(ctx,Sample,reinterpret_cast<void*>(static_cast<std::uintptr_t>(epoch)))!=D2RL::Threads::Result::Success) {
                    expected=epoch; queued.compare_exchange_strong(expected,0);
                }
            }
        }
        std::array<Damage::Number,128> numbers{}; Damage::Point anchor{}; bool anchorValid{}; std::size_t count{};
        {
            std::lock_guard lock(stateMutex);
            model.Expire(now,settings); numbers=model.numbers; anchor=player; anchorValid=playerValid; count=enemyCount;
        }
        Damage::Projection projection{};
        const float scale=Damage::ResolutionScale(frame->screenWidth,frame->screenHeight,settings);
        const bool projected=anchorValid && Projection(frame->screenWidth,frame->screenHeight,anchor,projection);
        const Clip clip{BeginClip(frame->screenWidth,frame->screenHeight)};
        if(!clip.list) return;
        if(settings.debug) {
            char message[160]{};
            std::snprintf(message,sizeof(message),"Floating Damage | %zu enemies | projection %s",count,projected?"ready":"unavailable");
            Text(frame->canvas,{20*scale,80*scale},message,16*scale,{1,0.85f,0.5f,1});
        }
        if(!projected) return;
        for(const auto& n:numbers) {
            if(!n.raw || now<n.born) continue;
            Damage::Point screen{};
            if(!projection.Project(n.target.position,screen)) continue;
            const float age=static_cast<float>(now-n.born)/static_cast<float>(settings.lifetime);
            const float alpha=Damage::Opacity(age,settings);
            const auto motion=Damage::Motion(n,age,settings,fallingMotion.load(),scale);
            screen.x+=motion.x;screen.y+=motion.y;
            const float margin=100*scale;
            if(screen.x<-margin || screen.y<-margin || screen.x>frame->screenWidth+margin || screen.y>frame->screenHeight+margin) continue;
            DrawNumber(frame->canvas,screen,n,alpha,scale);
        }
    } catch(...) {ctx->LogWarn("Floating Damage: overlay frame failed.");}
}
void __cdecl Life(const D2RL::PluginContext*,const D2RL::Lifecycle::GameplayEvent* event,void*) noexcept {
    if(!D2RL::Lifecycle::HasGameplayEventField(event,D2RL::Lifecycle::GameplayEventRequiredSize)) return;
    active.store(false); generation.fetch_add(1); queued.store(0);
    if(event->kind==D2RL::Lifecycle::GameplayEventKind::GameLeft) capture.store(false);
    playerId.store(event->playerId);
    {std::lock_guard lock(stateMutex);model.Clear();playerValid=false;enemyCount=0;}
    reports.store(0); failures.store(0);
    active.store(event->kind!=D2RL::Lifecycle::GameplayEventKind::GameLeft);
}
bool Admit() noexcept {
    using namespace Damage::Native;
    if(!Hash(reinterpret_cast<HMODULE>(ctx->exeBase),ExeHash) || !Hash(GetModuleHandleW(L"D2RCore.dll"),CoreHash)) return false;
    return ctx->CheckExpectedBytes(GameWriterRva,GameWriterBytes,sizeof(GameWriterBytes)) &&
        ctx->CheckExpectedBytes(RendererRva,RendererBytes,sizeof(RendererBytes)) &&
        ctx->CheckExpectedBytes(PickWitnessRva,PickWitnessBytes,sizeof(PickWitnessBytes)) &&
        ctx->CheckExpectedBytes(DrawListRva,DrawListBytes,sizeof(DrawListBytes)) &&
        ctx->CheckExpectedBytes(PushClipRva,PushClipBytes,sizeof(PushClipBytes)) &&
        ctx->CheckExpectedBytes(PopClipRva,PopClipBytes,sizeof(PopClipBytes));
}
void Stop() noexcept {
    capture.store(false);
    combatReady.store(false);
    poisonReady.store(false);
    criticalReady.store(false);
    chatCommands.Stop(); // Drain retained chat callbacks before taking stateMutex.
    {std::lock_guard lock(stateMutex);ready.store(false);enabled.store(false);active.store(false);generation.fetch_add(1);model.Clear();playerValid=false;enemyCount=0;}
    if(ctx && overlay && frameHandle) overlay->unregisterFrameCallback(ctx,frameHandle);
    if(ctx && lifecycle) for(auto handle:lifeHandles) if(handle) lifecycle->unregisterGameplayEventListener(ctx,handle);
    if(ctx && lifecycle && deathHandle) lifecycle->unregisterMonsterDeathListener(ctx,deathHandle);
    frameHandle=0; lifeHandles={}; deathHandle=0; queued.store(0);
}
}
D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    static constexpr D2RL::PluginInfo info{.infoSize=D2RL::PluginInfoSize,.abiVersion=D2RL_PLUGIN_ABI_VERSION,
        .id="floating-damage",.name="Floating Damage",.version="1.3.1+rev.13",.author="PyreFly",
        .description="Floating numbers for observed enemy HP loss.",.flags=D2RL::PluginFlags::Client|D2RL::PluginFlags::NativeHooks};
    return &info;
}
D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* context) noexcept -> bool {
    if(!context || !context->GetApi()) return false;
    ctx=context;
    try {
        char config[16384]{}; std::uint32_t required{}; unsigned rejected{};
        if(!ctx->ReadConfig(config,sizeof(config)-1,&required)) return false;
        settings=Damage::Parse(config,rejected);
        if(rejected) ctx->LogWarn("Floating Damage: invalid configuration values ignored; defaults retained.");
        auto command=D2RL::MakeConsoleCommand("damagenumbers",Command,"Toggle floating enemy damage numbers; on/off/status; motion on/off/status.");
        command.usage="/damagenumbers [on|off|status] | motion/capture [on|off|status]";
        command.scope=D2RL::ConsoleCommandScope::LocalOnly;
        if(!ctx->RegisterConsoleCommand(command)) return false;
        if(!Admit()) {ctx->LogWarn("Floating Damage: unsupported executable/Core or byte guards; plugin remains inert.");return true;}
        if(ctx->QueryService(&overlay)!=D2RL::ServiceQueryResult::Success || !D2RL::HasOverlayServiceField(overlay,D2RL::OverlayServiceRequiredSize) ||
            ctx->QueryService(&threads)!=D2RL::ServiceQueryResult::Success || !D2RL::HasThreadServiceField(threads,D2RL::ThreadServiceRequiredSize) ||
            ctx->QueryService(&lifecycle)!=D2RL::ServiceQueryResult::Success || !D2RL::HasLifecycleServiceField(lifecycle,D2RL::LifecycleServiceRequiredSize)) return false;
        const D2RL::Lifecycle::GameplayEventKind kinds[]={D2RL::Lifecycle::GameplayEventKind::GameJoined,D2RL::Lifecycle::GameplayEventKind::LocalPlayerReady,D2RL::Lifecycle::GameplayEventKind::GameLeft};
        for(unsigned i=0;i<3;++i) {
            const D2RL::Lifecycle::GameplayEventListener listener{sizeof(listener),0,kinds[i],0,Life,nullptr};
            if(lifecycle->registerGameplayEventListener(ctx,&listener,&lifeHandles[i])!=D2RL::Lifecycle::Result::Success) {Stop();return false;}
        }
        const D2RL::Lifecycle::MonsterDeathListener death{sizeof(death),0,Death,nullptr};
        if(lifecycle->registerMonsterDeathListener(ctx,&death,&deathHandle)!=D2RL::Lifecycle::Result::Success) {Stop();return false;}
        const D2RL::Overlay::CallbackRegistration callback{sizeof(callback),0,D2RL::Overlay::Phase::AfterGameUi,0,Frame,nullptr};
        if(overlay->registerFrameCallback(ctx,&callback,&frameHandle)!=D2RL::Overlay::Result::Success) {Stop();return false;}
        enabled.store(settings.enabled); fallingMotion.store(settings.fallingMotion); ready.store(true);
        const bool observeCombat=settings.elementColors || settings.criticalHits;
        if(observeCombat && ctx->CheckExpectedBytes(Damage::Combat::ApplyRva,Damage::Combat::ApplyBytes,sizeof(Damage::Combat::ApplyBytes)) &&
            ctx->CheckExpectedBytes(Damage::Combat::HpSiteRva,Damage::Combat::HpBytes,sizeof(Damage::Combat::HpBytes)) &&
            ctx->InstallInlineHook(Damage::Combat::ApplyRva,Damage::Combat::ApplyBytes,sizeof(Damage::Combat::ApplyBytes),reinterpret_cast<void*>(CombatHook),&combatOriginal) && combatOriginal) {
            combatReady.store(true); ctx->LogInfo("Floating Damage: guarded hit observation enabled; unmatched HP decreases use normal styling.");
        } else if(observeCombat) ctx->LogWarn("Floating Damage: combat guards/hook refused; ordinary HP numbers remain available.");
        poisonReady.store(combatReady.load() && Damage::Native::PoisonLayoutMatches(ctx->exeBase));
        if(poisonReady.load()) ctx->LogInfo("Floating Damage: poison frame/expiration guards matched; exact periodic HP matching enabled.");
        else if(combatReady.load()) ctx->LogWarn("Floating Damage: poison layout guards refused; direct hit colors remain available.");
        criticalReady.store(combatReady.load() && Damage::Native::CriticalLayoutMatches(ctx->exeBase));
        if(criticalReady.load()) ctx->LogInfo(settings.criticalHits?
            "Floating Damage: native critical/deadly writer guards matched; configured critical styling enabled.":
            "Floating Damage: native critical/deadly writer guards matched; critical emphasis disabled in configuration.");
        else if(combatReady.load()) ctx->LogWarn("Floating Damage: critical writer guards refused; raw result flags remain diagnostic only.");
        if(!chatCommands.Start(ctx) || !chatCommands.Register("damagenumbers",ChatCommand,nullptr,"Toggle floating damage: on/off/status; motion on/off/status."))
            ctx->LogWarn("Floating Damage: native chat registration unavailable; debug-console command remains available.");
        ctx->LogInfo(settings.enabled?"Floating Damage 1.3.1+rev.13: admitted; waiting for local gameplay lifecycle.":
            "Floating Damage 1.3.1+rev.13: admitted; disabled in configuration; /damagenumbers on enables it.");return true;
    } catch(...) {Stop();return false;}
}
D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {Stop();}
