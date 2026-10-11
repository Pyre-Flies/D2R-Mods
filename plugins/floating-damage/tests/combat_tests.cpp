#include "../src/plugin.cpp"
#include <cstdlib>
namespace {
unsigned calls{}; bool argsMatch{},die{},marked{},markedElement{};
unsigned logged{}; char lastLog[640]{};
unsigned rectangles{};D2RL::Overlay::FilledRectangleRequest lastRectangle{};
D2RL::Overlay::Result rectangleResult=D2RL::Overlay::Result::Success;
unsigned glyphs{};float measuredSize{},glyphSize{};char measuredText[64]{},glyphText[64]{};
D2RL::Overlay::Point glyphPosition{};D2RL::Overlay::Color glyphColor{};bool glyphsConsistent=true;
D2RL::Overlay::Result __cdecl Measure(const D2RL::PluginContext*,const D2RL::Overlay::MeasureTextRequest* request,D2RL::Overlay::TextMetrics* metrics) noexcept {
    measuredSize=request->textSize;std::snprintf(measuredText,sizeof(measuredText),"%.*s",static_cast<int>(request->textLength),request->text);
    metrics->width=80;metrics->height=request->textSize;return D2RL::Overlay::Result::Success;
}
D2RL::Overlay::Result __cdecl Glyph(const D2RL::PluginContext*,const D2RL::Overlay::TextRequest* request) noexcept {
    ++glyphs;glyphSize=request->textSize;glyphPosition=request->position;glyphColor=request->color;
    std::snprintf(glyphText,sizeof(glyphText),"%.*s",static_cast<int>(request->textLength),request->text);
    glyphsConsistent=glyphsConsistent && glyphSize==measuredSize && std::strcmp(glyphText,measuredText)==0;
    return D2RL::Overlay::Result::Success;
}
D2RL::Overlay::Result __cdecl Rectangle(const D2RL::PluginContext*,const D2RL::Overlay::FilledRectangleRequest* request) noexcept {
    ++rectangles;lastRectangle=*request;return rectangleResult;
}
void __cdecl Log(const D2RL::PluginContext*,const char* text) noexcept {++logged;std::snprintf(lastLog,sizeof(lastLog),"%s",text);}
void *expectedGame{},*expectedAttacker{},*expectedDefender{},*expectedDamage{};
std::uint32_t expectedMissile{}; std::int64_t* hpValue{};
std::uintptr_t __fastcall Original(void* game,void* attacker,void* defender,std::uint32_t missile,void* damage) {
    ++calls; argsMatch=game==expectedGame && attacker==expectedAttacker && defender==expectedDefender && missile==expectedMissile && damage==expectedDamage;
    const std::int32_t amount=die?10132:100;
    std::memset(damage,0,0x138);
    std::memcpy(static_cast<unsigned char*>(damage)+(marked && !markedElement?0x18:0x20),&amount,sizeof(amount));
    if(marked) {const std::uint16_t flags=0x2005;std::memcpy(static_cast<unsigned char*>(damage)+4,&flags,sizeof(flags));}
    std::memcpy(static_cast<unsigned char*>(damage)+0x134,&amount,sizeof(amount));
    if(die) {
        model.Death(17,5,reinterpret_cast<std::uintptr_t>(game),generation.load(),GetTickCount64(),settings);
        const std::uintptr_t removed{};
        std::memcpy(static_cast<unsigned char*>(defender)+0x88,&removed,sizeof(removed));
    } else *hpValue-=100;
    return 0xabcd1234;
}
template<class T,std::size_t N> void Put(std::array<unsigned char,N>& buffer,unsigned offset,T value) {std::memcpy(buffer.data()+offset,&value,sizeof(value));}
void Check(bool value,const char* name) {if(!value){std::fprintf(stderr,"FAIL: %s\n",name);std::exit(1);}}
}
int main() {
    const D2RL::OverlayService drawing{.drawFilledRectangle=Rectangle,.drawText=Glyph,.measureText=Measure};overlay=&drawing;
    const D2RL::Overlay::TextMetrics metrics{sizeof(metrics),0,40,26};settings=Damage::Settings{};
    Check(Backing(7,{100,200},metrics,Damage::Poison,0.5f) && rectangles==1 && lastRectangle.canvas==7 &&
        lastRectangle.structSize==sizeof(lastRectangle) && !lastRectangle.flags && !lastRectangle.reserved &&
        lastRectangle.rect.left==94 && lastRectangle.rect.top==197 && lastRectangle.rect.right==146 && lastRectangle.rect.bottom==229 &&
        lastRectangle.rounding==3 && std::abs(lastRectangle.color.alpha-0.34f)<0.001f,"real overlay adapter submits padded bounds, canvas and shared fade alpha");
    Check(!Backing(7,{100,200},metrics,Damage::Magic,1) && rectangles==1,"poison-only backing skips other damage");
    const D2RL::Overlay::TextMetrics invalid{sizeof(invalid),0,40,-1};
    Check(!Backing(7,{100,200},invalid,Damage::Poison,1) && rectangles==1,"invalid metrics cannot submit a rectangle");
    rectangleResult=D2RL::Overlay::Result::InvalidArgument;
    Check(!Backing(7,{100,200},metrics,Damage::Poison,1),"provider refusal is handled without native fallback");
    const D2RL::OverlayService unavailable{};overlay=&unavailable;
    Check(!Backing(7,{100,200},metrics,Damage::Poison,1),"missing optional rectangle function is skipped safely");overlay=nullptr;
    overlay=&drawing;rectangleResult=D2RL::Overlay::Result::Success;settings.textSize=36;
    Damage::Number display{};display.raw=64*256;display.critical=true;display.elements=Damage::Physical|Damage::Poison;
    DrawNumber(7,{100,200},display,0.5f);
    Check(std::strcmp(measuredText,"64!!")==0 && std::abs(measuredSize-48.6f)<0.001f && glyphsConsistent && glyphs==34 &&
        glyphPosition.x==60 && glyphPosition.y==200 && glyphColor.red==settings.mixedColor[0] && glyphColor.alpha==0.5f &&
        lastRectangle.rect.left==54 && lastRectangle.rect.right==146 && std::abs(lastRectangle.rect.bottom-251.6f)<0.001f,
        "real drawing adapter measures larger suffixed text, centers it and uses it for every glyph/backing layer");
    display.critical=false;DrawNumber(7,{100,200},display,1);
    Check(std::strcmp(glyphText,"64")==0 && measuredSize==36 && glyphSize==36 && glyphsConsistent,"normal drawing retains ordinary text and configured size");
    settings=Damage::Settings{};overlay=nullptr;
    std::uintptr_t game=0x12345678; std::array<unsigned char,0x160> unit{}; std::array<unsigned char,0xc0> list{};
    struct Stat {std::uint32_t layer,id;std::int64_t value;}; std::array<Stat,2> stats{{{0,6,1000},{0,7,2000}}};
    Put(unit,0,std::uint32_t{1});Put(unit,4,std::uint32_t{5});Put(unit,8,std::uint32_t{17});Put(unit,0xd8,game);
    Put(unit,0x88,reinterpret_cast<std::uintptr_t>(list.data()));
    Put(list,0x30,reinterpret_cast<std::uintptr_t>(stats.data()));Put(list,0x38,std::uint64_t{2});Put(list,0x40,std::uint64_t{2});
    Damage::CombatPacket packet{};
    expectedGame=reinterpret_cast<void*>(game); expectedAttacker=reinterpret_cast<void*>(std::uintptr_t{0x87654321});
    expectedDefender=unit.data();expectedDamage=packet.bytes.data();expectedMissile=1;hpValue=&stats[0].value;
    const D2RL::PluginApi api{.apiSize=D2RL::PluginApiSize,.logInfo=Log};
    const D2RL::PluginContext context{.contextSize=sizeof(D2RL::PluginContext),.api=&api,
        .exeBase=reinterpret_cast<std::uintptr_t>(&game)-Damage::Native::GameGlobalRva};
    ctx=&context;combatOriginal=reinterpret_cast<void*>(Original);combatReady.store(true);active.store(true);enabled.store(false);generation.store(7);
    Check(CombatHook(expectedGame,expectedAttacker,expectedDefender,expectedMissile,expectedDamage)==0xabcd1234 && calls==1 && argsMatch && stats[0].value==900,
        "disabled feature preserves original arguments, one call and RAX");
    enabled.store(true); Damage::Monster monster{reinterpret_cast<std::uintptr_t>(unit.data()),17,5,900,2000,{100,100}};
    model.Sample(std::span(&monster,1),game,7,GetTickCount64(),settings);
    Check(CombatHook(expectedGame,expectedAttacker,expectedDefender,expectedMissile,expectedDamage)==0xabcd1234 && calls==2 && stats[0].value==800,
        "enabled observer forwards native call once");
    monster.hp=800; model.Sample(std::span(&monster,1),game,7,GetTickCount64(),settings);
    Check(model.numbers[0].raw==100 && model.numbers[0].elements==Damage::Fire,"real hook adapter correlates copied packet and HP");
    Put(unit,0xd8,std::uintptr_t{0});
    CombatHook(expectedGame,expectedAttacker,expectedDefender,expectedMissile,expectedDamage);
    monster.hp=700;model.Sample(std::span(&monster,1),game,7,GetTickCount64(),settings);
    Check(calls==3 && model.numbers[0].elements==Damage::Unclassified,"foreign owner passes through without coloring missing loss");
    Put(unit,0xd8,game);model.Clear(); monster.hp=700;model.Sample(std::span(&monster,1),game,7,GetTickCount64(),settings);die=true;
    CombatHook(expectedGame,expectedAttacker,expectedDefender,expectedMissile,expectedDamage);
    Check(calls==4 && model.numbers[0].raw==700 && model.numbers[0].elements==Damage::Fire,"destroyed stats after synchronous death preserve remainder and color");
    Put(unit,0x88,reinterpret_cast<std::uintptr_t>(list.data()));stats[0].value=700;die=false;marked=true;criticalReady.store(true);
    settings.elementColors=false;settings.mergeMs=0;model.Clear();monster.hp=700;model.Sample(std::span(&monster,1),game,7,GetTickCount64(),settings);
    Check(CombatHook(expectedGame,expectedAttacker,expectedDefender,expectedMissile,expectedDamage)==0xabcd1234 && calls==5 && argsMatch,"critical observation retains native forwarding with element colors disabled");
    monster.hp=600;model.Sample(std::span(&monster,1),game,7,GetTickCount64(),settings);
    Check(model.numbers[0].critical && model.numbers[0].raw==100,"actual hook correlates the guarded native marker even without element colors");
    criticalReady.store(false);CombatHook(expectedGame,expectedAttacker,expectedDefender,expectedMissile,expectedDamage);
    monster.hp=500;model.Sample(std::span(&monster,1),game,7,GetTickCount64(),settings);
    Check(!model.numbers[1].critical && model.numbers[1].raw==100,"optional critical guard refusal preserves ordinary HP numbers");
    criticalReady.store(true);markedElement=true;CombatHook(expectedGame,expectedAttacker,expectedDefender,expectedMissile,expectedDamage);
    monster.hp=400;model.Sample(std::span(&monster,1),game,7,GetTickCount64(),settings);
    Check(model.numbers[2].critical && model.numbers[2].elements==Damage::Fire,"native bit rather than final component color governs emphasis");
    markedElement=false;model.Clear();monster.hp=400;model.Sample(std::span(&monster,1),game,7,GetTickCount64(),settings);die=true;
    CombatHook(expectedGame,expectedAttacker,expectedDefender,expectedMissile,expectedDamage);
    Check(model.numbers[0].critical && model.numbers[0].raw==400,"real adapter handles a marked death inside the original call");
    ready.store(true);fallingMotion.store(true);ChatCommand("motion off",nullptr);
    Check(!fallingMotion.load() && model.numbers[0].raw==400,"native chat adapter dispatches motion without clearing HP state");
    ChatCommand("off",nullptr); Check(!enabled.load() && model.numbers[0].raw==0,"native chat adapter disables and clears baselines");
    ChatCommand("capture on",nullptr);Check(capture.load() && captureRecords.load()==0,"capture starts from actual chat adapter while feature is off");
    Damage::CombatPacket diagnostic{};const std::int32_t magic=4321;std::memcpy(diagnostic.bytes.data()+0x30,&magic,sizeof(magic));
    TracePacket(diagnostic,"after",17);Check(std::strstr(lastLog,"magic=4321"),"capture copies and logs the magic component");
    criticalReady.store(true);Put(diagnostic.bytes,4,std::uint32_t{0x20002001});
    TracePacket(diagnostic,"before",17);
    Check(diagnostic.CriticalMarker() && diagnostic.ResultFlags()==0x2001 && std::strstr(lastLog,"result4=2001 extra6=2000 crit_guard=1 critical_candidate=1"),
        "capture reads the result word separately from adjacent extra flags");
    Put(diagnostic.bytes,4,std::uint32_t{0x20000001});TracePacket(diagnostic,"after",17);
    Check(!diagnostic.CriticalMarker() && std::strstr(lastLog,"critical_candidate=0"),"extra flags do not produce a critical marker");
    Put(diagnostic.bytes,4,std::uint32_t{0x2000});criticalReady.store(false);TracePacket(diagnostic,"after",17);
    Check(std::strstr(lastLog,"result4=2000") && std::strstr(lastLog,"crit_guard=0 critical_candidate=0"),"guard refusal keeps raw flags but disables interpretation");
    captureRecords.store(127);Trace("last record");Check(!capture.load() && captureRecords.load()==128,"capture auto-stops at bounded log limit");
    ChatCommand("capture on",nullptr);ChatCommand("capture off",nullptr);Check(!capture.load(),"capture has an explicit stop control");
    std::array<unsigned char,0xa8> effect{};std::array<Stat,1> rate{{{0,74,-60}}};
    Put(unit,0x88,reinterpret_cast<std::uintptr_t>(list.data()));Put(list,0x1c,std::int32_t{-1});
    Put(list,0x90,reinterpret_cast<std::uintptr_t>(effect.data()));Put(list,0x98,std::uintptr_t{0});
    Put(effect,0x20,std::uint32_t{2});Put(effect,0x30,reinterpret_cast<std::uintptr_t>(rate.data()));
    Put(effect,0x38,std::uint64_t{1});Put(effect,0x40,std::uint64_t{1});
    auto state=Damage::Native::ReadPoisonState(reinterpret_cast<std::uintptr_t>(unit.data()));
    Check(state.readable && state.found && state.rate==-60,"bounded reader copies state2 stat74");
    Put(effect,0x20,std::uint32_t{3});Put(effect,0x68,reinterpret_cast<std::uintptr_t>(effect.data()));
    state=Damage::Native::ReadPoisonState(reinterpret_cast<std::uintptr_t>(unit.data()));
    Check(!state.readable && !state.found,"cyclic state-list diagnostic is refused");
    std::puts("Combat adapter preserves native behavior, refuses foreign owners, handles synchronous death and dispatches chat controls.");
}
