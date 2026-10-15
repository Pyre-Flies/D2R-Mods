#include "../src/plugin.cpp"
#include <cstdlib>
namespace {
unsigned calls{}; bool argsMatch{},die{},marked{},markedElement{};
std::string commandConfig="# keep\r\n[floating-damage]\r\nenabled = true # enabled\r\nfalling_motion = true\r\ntext_size = 31\r\n[other]\r\nkeep = 8\r\n";
unsigned configReads{},configWrites{};bool configReadFail{},configWriteFail{};
bool __cdecl ReadConfig(const D2RL::PluginContext*,char* output,unsigned size,unsigned* required) noexcept {
    ++configReads;*required=static_cast<unsigned>(commandConfig.size()+1);
    if(configReadFail || size<*required)return false;
    std::memcpy(output,commandConfig.c_str(),*required);return true;
}
bool __cdecl WriteConfig(const D2RL::PluginContext*,const char* text) noexcept {
    ++configWrites;if(configWriteFail)return false;commandConfig=text;return true;
}
D2RL::ConsoleCommandResult Invoke(const char* args) noexcept {
    const D2RL::ConsoleCommandContext command{.contextSize=D2RL::ConsoleCommandContextSize,
        .argsLength=static_cast<unsigned>(std::strlen(args)),.args=args,.plugin=ctx};
    return Command(nullptr,&command,nullptr);
}

unsigned logged{}; char lastLog[640]{};
unsigned rectangles{};D2RL::Overlay::FilledRectangleRequest lastRectangle{};
D2RL::Overlay::Result rectangleResult=D2RL::Overlay::Result::Success;
unsigned glyphs{};float measuredSize{},glyphSize{},metricWidth=80;char measuredText[64]{},glyphText[64]{};
D2RL::Overlay::Point glyphPosition{};D2RL::Overlay::Color glyphColor{};bool glyphsConsistent=true;
struct GlyphPass {D2RL::Overlay::Point position{};D2RL::Overlay::Color color{};};
std::array<GlyphPass,128> passes{};
D2RL::Overlay::Result __cdecl Measure(const D2RL::PluginContext*,const D2RL::Overlay::MeasureTextRequest* request,D2RL::Overlay::TextMetrics* metrics) noexcept {
    measuredSize=request->textSize;std::snprintf(measuredText,sizeof(measuredText),"%.*s",static_cast<int>(request->textLength),request->text);
    metrics->width=metricWidth;metrics->height=request->textSize;return D2RL::Overlay::Result::Success;
}
D2RL::Overlay::Result __cdecl Glyph(const D2RL::PluginContext*,const D2RL::Overlay::TextRequest* request) noexcept {
    if(glyphs<passes.size()) passes[glyphs]={request->position,request->color};
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
    settings=Damage::Settings{};metricWidth=160;glyphs=0;glyphsConsistent=true;display.critical=true;
    const auto before4K=rectangles;
    DrawNumber(7,{100,200},display,0.5f,2);
    Check(std::strcmp(measuredText,"64!!")==0 && std::abs(measuredSize-70.2f)<0.001f && glyphsConsistent && glyphs==90 &&
        glyphPosition.x==20 && glyphPosition.y==200 && glyphColor.red==settings.mixedColor[0] && glyphColor.alpha==0.5f,
        "4K adapter doubles configured text size and applies combined critical styling consistently to all ninety passes");
    Check(rectangles==before4K+1 && lastRectangle.rect.left==8 && lastRectangle.rect.top==194 && lastRectangle.rect.right==192 &&
        std::abs(lastRectangle.rect.bottom-276.2f)<0.001f && lastRectangle.rounding==6 && std::abs(lastRectangle.color.alpha-0.34f)<0.001f,
        "4K backing uses scaled padding plus the actual four-pixel outline, scaled rounding and unchanged fade opacity");
    Check(passes[0].position.x==26 && passes[0].position.y==206 &&
        std::abs(passes[1].position.x-passes[0].position.x)+std::abs(passes[1].position.y-passes[0].position.y)==2 &&
        passes[8].position.x==30 && passes[8].position.y==210 && passes[4].position.x==28 && passes[4].position.y==208 &&
        std::abs(passes[0].color.alpha-0.0234375f)<0.00001f,
        "4K shadow doubles offset and kernel spacing without changing opacity weights");
    std::array<bool,81> covered{};
    for(unsigned i=9;i<89;++i) {
        const float dx=passes[i].position.x-20,dy=passes[i].position.y-200;
        Check(dx>=-4 && dx<=4 && dy>=-4 && dy<=4 && std::floor(dx)==dx && std::floor(dy)==dy && (dx!=0 || dy!=0),
            "scaled outline uses dense integer pixel offsets within its four-pixel radius");
        const auto index=static_cast<unsigned>((dy+4)*9+dx+4);
        Check(!covered[index] && passes[i].color.red==settings.outlineColor[0] && passes[i].color.alpha==0.5f,
            "each scaled outline position is drawn once with the shared fade alpha");
        covered[index]=true;
    }
    for(unsigned i=0;i<covered.size();++i) Check(covered[i]==(i!=40),"4K outline covers every pixel around the glyph without sparse scaled gaps");
    for(const auto mode:{Damage::CriticalStyle::Suffix,Damage::CriticalStyle::Size,Damage::CriticalStyle::Both}) {
        settings.criticalStyle=mode;glyphs=0;glyphsConsistent=true;
        DrawNumber(7,{100,200},display,1,2);
        Check(std::strcmp(glyphText,mode==Damage::CriticalStyle::Size?"64":"64!!")==0 &&
            std::abs(measuredSize-(mode==Damage::CriticalStyle::Suffix?52.0f:70.2f))<0.001f && glyphsConsistent && glyphs==90,
            "actual 4K rendering honors each critical style across measurement, shadow, outline and fill");
    }
    display.critical=false;glyphs=0;glyphsConsistent=true;DrawNumber(7,{100,200},display,1,2);
    Check(std::strcmp(glyphText,"64")==0 && measuredSize==52 && glyphSize==52 && glyphsConsistent && glyphs==90,
        "actual ordinary 4K rendering retains normal styling at twice the base font size");
    display.critical=true;settings.criticalHits=false;glyphs=0;glyphsConsistent=true;DrawNumber(7,{100,200},display,1,2);
    Check(std::strcmp(glyphText,"64")==0 && measuredSize==52 && glyphsConsistent,"critical opt-out leaves viewport scaling active in the real drawing adapter");
    settings=Damage::Settings{};overlay=nullptr;
    std::uintptr_t game=0x12345678; std::array<unsigned char,0x160> unit{}; std::array<unsigned char,0xc0> list{};
    struct Stat {std::uint32_t layer,id;std::int64_t value;}; std::array<Stat,2> stats{{{0,6,1000},{0,7,2000}}};
    Put(unit,0,std::uint32_t{1});Put(unit,4,std::uint32_t{5});Put(unit,8,std::uint32_t{17});Put(unit,0xd8,game);
    Put(unit,0x88,reinterpret_cast<std::uintptr_t>(list.data()));
    Put(list,0x30,reinterpret_cast<std::uintptr_t>(stats.data()));Put(list,0x38,std::uint64_t{2});Put(list,0x40,std::uint64_t{2});
    Damage::CombatPacket packet{};
    expectedGame=reinterpret_cast<void*>(game); expectedAttacker=reinterpret_cast<void*>(std::uintptr_t{0x87654321});
    expectedDefender=unit.data();expectedDamage=packet.bytes.data();expectedMissile=1;hpValue=&stats[0].value;
    const D2RL::PluginApi api{.apiSize=D2RL::PluginApiSize,.logInfo=Log,.readConfig=ReadConfig,.writeConfig=WriteConfig};
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
    unsigned commandRejected{};auto restored=Damage::Parse(commandConfig,commandRejected);
    Check(!commandRejected && !restored.enabled && !restored.fallingMotion && restored.textSize==31,
        "actual chat commands save startup values and preserve unrelated styling");
    Check(commandConfig.starts_with("# keep\r\n") && commandConfig.ends_with("[other]\r\nkeep = 8\r\n") &&
        commandConfig.find("# enabled")!=commandConfig.npos,"commands preserve comments, CRLF and other sections");
    auto readCount=configReads,writeCount=configWrites;
    for(const char* arg:{"status","motion status","capture status","invalid","on off"}) Invoke(arg);
    Check(configReads==readCount && configWrites==writeCount,"status and rejected commands do no configuration IO");
    Check(Invoke("on")==D2RL::ConsoleCommandResult::Handled && enabled.load(),"explicit on saves and enables");
    Check(Invoke("toggle")==D2RL::ConsoleCommandResult::Handled && !enabled.load(),"toggle saves off");
    ChatCommand("",nullptr);Check(enabled.load(),"bare chat command saves the toggled state");
    Check(Invoke("on")==D2RL::ConsoleCommandResult::Handled && configWrites==writeCount+4,"repeated explicit on still confirms the saved preference");
    Invoke("motion on");Check(Damage::Parse(commandConfig,commandRejected).fallingMotion,"motion on restores from saved text");
    Invoke("motion toggle");Check(!Damage::Parse(commandConfig,commandRejected).fallingMotion,"motion toggle restores from saved text");
    Invoke("off");const auto retained=commandConfig;configWriteFail=true;
    Check(Invoke("on")==D2RL::ConsoleCommandResult::Failed && !enabled.load() && commandConfig==retained,
        "write failure leaves runtime and saved enable state unchanged");
    Check(Invoke("motion on")==D2RL::ConsoleCommandResult::Failed && !fallingMotion.load(),"failed motion save leaves runtime unchanged");
    configWriteFail=false;configReadFail=true;writeCount=configWrites;
    Check(Invoke("on")==D2RL::ConsoleCommandResult::Failed && !enabled.load() && configWrites==writeCount,
        "read failure never overwrites configuration");
    configReadFail=false;commandConfig=std::string(16384,'#');
    Check(Invoke("on")==D2RL::ConsoleCommandResult::Failed && configWrites==writeCount,"oversize config is not truncated");
    commandConfig=retained;ready.store(false);Invoke("on");
    Check(!enabled.load() && configWrites==writeCount,"unsupported state cannot save commands");ready.store(true);
    readCount=configReads;writeCount=configWrites;
    Invoke("capture on");Invoke("capture off");
    Check(configReads==readCount && configWrites==writeCount,"bounded diagnostic capture is an action, not a startup preference");
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
