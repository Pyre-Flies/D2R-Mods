#include "style.h"
#include "config.h"
#include <cstdlib>
#include <cstring>
#include <limits>
using namespace Damage;
void Check(bool value,const char* text){if(!value){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
int main() {
    Settings config;config.mergeMs=0;config.textSize=36;Model model;
    Monster enemy{100,17,5,1000,20000,{100,100}};
    const auto sample=[&](std::uint64_t now){model.Sample(std::span(&enemy,1),1,7,now,config);};
    sample(100);model.RecordHit(enemy,700,true,Physical,1,7,110,115,false,{},true);
    enemy.hp=700;sample(130);
    Check(model.numbers[0].raw==300 && model.numbers[0].critical && model.numbers[0].elements==Physical,"exact marked HP endpoints retain critical metadata");
    model.RecordHit(enemy,500,true,Physical,1,7,140,145);enemy.hp=500;sample(160);
    Check(!model.numbers[1].critical,"later ordinary hit on the same target does not inherit critical");
    char text[64]{};Number visible{};visible.raw=64*256;visible.critical=true;visible.elements=Physical|Poison;
    FormatNumber(visible,config,text);
    Check(std::strcmp(text,"64!!")==0 && std::abs(TextSize(visible,config)-48.6f)<0.001f && TextColor(visible.elements,config)==config.mixedColor,
        "marked display grows 35 percent, appends !! and retains mixed color");
    visible.raw=1;FormatNumber(visible,config,text);Check(std::strcmp(text,"<1!!")==0,"sub-unit critical preserves the real fraction display");
    visible.raw=257;config.precision=2;FormatNumber(visible,config,text);Check(std::strcmp(text,"1.00!!")==0,"precision and critical suffix coexist");
    config.precision=0;visible.raw=64*256;
    for(const auto mode:{CriticalStyle::Suffix,CriticalStyle::Size,CriticalStyle::Both}) {
        config.criticalStyle=mode;
        const bool suffix=mode!=CriticalStyle::Size;
        const float expectedSize=mode==CriticalStyle::Suffix?36.0f:48.6f;
        FormatNumber(visible,config,text);
        Check(std::strcmp(text,suffix?"64!!":"64")==0 && std::abs(TextSize(visible,config)-expectedSize)<0.001f &&
            std::abs(TextSize(visible,config,2)-expectedSize*2)<0.001f,
            "critical mode independently selects suffix and size at baseline and 4K scales");
        visible.critical=false;FormatNumber(visible,config,text);
        Check(std::strcmp(text,"64")==0 && TextSize(visible,config,2)==72,"ordinary hits stay ordinary in every critical mode");
        visible.critical=true;config.criticalHits=false;FormatNumber(visible,config,text);
        Check(std::strcmp(text,"64")==0 && TextSize(visible,config,2)==72,"master disable overrides every critical mode while retaining resolution scaling");
        config.criticalHits=true;
    }
    config.criticalStyle=CriticalStyle::Both;visible.raw=257;config.precision=2;
    config.criticalHits=false;FormatNumber(visible,config,text);
    Check(std::strcmp(text,"1.00")==0 && TextSize(visible,config)==36,"critical styling can be disabled without changing data");
    config.criticalHits=true;config.precision=0;
    model.Clear();enemy.hp=10000;sample(200);enemy.hp=1;sample(230);
    Check(!model.numbers[0].critical,"large unobserved damage is never guessed critical");
    model.Clear();enemy.hp=1000;sample(300);
    model.RecordHit(enemy,700,true,Physical,2,7,310,315,false,{},true);
    model.RecordHit(enemy,700,true,Physical,1,8,310,315,false,{},true);
    auto reused=enemy;reused.identity=200;model.RecordHit(reused,700,true,Physical,1,7,310,315,false,{},true);
    enemy.hp=700;sample(330);Check(!model.numbers[0].critical,"foreign owners, generations and identities cannot supply critical metadata");
    model.Clear();enemy.hp=1000;sample(400);
    model.RecordHit(enemy,0,false,Physical,1,7,410,415,false,{},true);enemy.hp=900;sample(430);
    Check(!model.numbers[0].critical,"unreadable endpoint without supported death cannot mark damage");
    model.Clear();enemy.hp=512;sample(500);
    Check(model.RecordHit(enemy,0,false,Physical|Poison,1,7,510,515,true,{},true)==Model::HitResult::PendingDeath,"marked lethal record waits for supported endpoint");
    Check(model.Death(17,5,1,7,550,config)==512 && model.numbers[0].critical && model.numbers[0].raw==512,
        "delayed lethal mark emphasizes actual remaining HP without overkill or multiplication");
    Check(model.Death(17,5,1,7,551,config)==0,"marked death cannot repeat its amount");
    model.Clear();enemy.hp=512;sample(600);model.Death(17,5,1,7,610,config);
    model.RecordHit(enemy,0,false,Physical,1,7,605,615,false,{},true);
    Check(model.numbers[0].critical,"death inside the original call receives post-call critical metadata");
    model.Clear();enemy.hp=1000;sample(700);
    model.RecordHit(enemy,0,false,Physical,1,7,710,715,true,{},true);
    model.RecordHit(enemy,0,true,Physical,1,7,730,735);model.Death(17,5,1,7,750,config);
    Check(!model.numbers[0].critical,"readable later hit invalidates an older unresolved critical");
    model.Clear();enemy.hp=512;sample(800);model.RecordHit(enemy,0,false,Physical,1,7,810,815,true,{},true);
    model.Death(17,5,1,7,1316,config);Check(!model.numbers[0].critical,"expired pending record cannot mark a later death");
    model.Clear();config.mergeMs=120;enemy.hp=10000;enemy.poison={};sample(2000);
    model.RecordHit(enemy,8000,true,Physical|Poison,1,7,2001,2001,false,{118,1800,1900,true},true);
    enemy.hp=8000;enemy.poison={118,1800,1900,true};sample(2030);
    for(unsigned i=1;i<=13;++i) {
        enemy.hp-=118;enemy.poison.frame=1800+i;const auto now=2030+40*i;sample(now);
        bool found=false;
        for(const auto& n:model.numbers) if(n.raw && n.last==now) {
            found=true;Check(n.critical==(i<=12),"mixed group retains its marked hit but a new poison-only group is ordinary");
            Check(n.elements==(i<=12?Physical|Poison:Poison),"critical metadata does not change existing mixed/periodic element coverage");
        }
        Check(found,"each poison interval still produces its HP loss");
    }
    Check(model.numbers[0].raw==3416 && model.numbers[1].raw==118,"critical styling leaves merged HP magnitudes unchanged");
    model.Clear();enemy.hp=4096;enemy.poison={};sample(3000);
    model.RecordHit(enemy,256,true,Physical|Poison,1,7,3001,3001,false,{118,100,150,true},true);
    enemy.hp=256;enemy.poison={118,100,150,true};sample(3030);
    enemy.hp=138;enemy.poison.frame=101;sample(3070);enemy.hp=20;enemy.poison.frame=102;sample(3110);
    Check(model.Death(17,5,1,7,3150,config,103,true)==20 && !model.numbers[1].critical && model.numbers[1].elements==Poison,
        "later poison killing remainder does not inherit the initial weapon critical");
    model.Clear();enemy.hp=1000;enemy.poison={};sample(4000);
    model.RecordHit(enemy,900,true,Physical,1,7,4010,4015);enemy.hp=900;sample(4030);
    model.RecordHit(enemy,800,true,Physical,1,7,4040,4045,false,{},true);enemy.hp=800;sample(4060);
    Check(model.numbers[0].critical && model.numbers[0].raw==200,"a marked hit merged after an ordinary hit emphasizes the group");
    enemy.hp=950;sample(4080);model.RecordHit(enemy,900,true,Physical,1,7,4090,4095,false,{},true);enemy.hp=900;sample(4110);
    Check(!model.numbers[0].critical && model.numbers[0].elements==Unclassified,"healing inside an ambiguous merged group clears critical association");
    model.Expire(6000,config);Check(!model.numbers[0].raw && !model.numbers[0].critical,"expiration clears amount and critical metadata together");
    model.Sample(std::span(&enemy,1),1,8,6010,config);Check(!model.numbers[0].critical,"session change retires critical history");
    unsigned rejected{};const auto styled=Parse("[floating-damage]\ncritical_hits=false\ncritical_size_scale=2\n",rejected);
    Check(rejected==0 && !styled.criticalHits && styled.criticalScale==2,"critical enable/scale configuration parses");
    const auto bad=Parse("[floating-damage]\ncritical_hits=maybe\ncritical_size_scale=0.9\ncritical_size_scale=2.1\ncritical_size_scale=nan\n",rejected);
    Check(rejected==4 && bad.criticalHits && bad.criticalScale==Settings{}.criticalScale,"invalid critical settings keep bounded defaults");
    const auto legacy=Parse("[floating-damage]\ntext_size=36\ncritical_size_scale=1.5\n",rejected);
    Check(rejected==0 && legacy.resolutionScaling && legacy.criticalStyle==CriticalStyle::Both && legacy.textSize==36 && legacy.criticalScale==1.5f,
        "existing configs keep saved sizes and gain default resolution scaling and combined critical styling");
    const auto suffix=Parse("[floating-damage]\ncritical_style=\"suffix\" # suffix only\nresolution_scaling=false\n",rejected);
    Check(rejected==0 && suffix.criticalStyle==CriticalStyle::Suffix && !suffix.resolutionScaling,"quoted suffix style and resolution scaling opt-out parse");
    const auto size=Parse("[floating-damage]\ncritical_style='size'\nresolution_scaling=true\n",rejected);
    Check(rejected==0 && size.criticalStyle==CriticalStyle::Size && size.resolutionScaling,"single-quoted size style and scaling opt-in parse");
    const auto both=Parse("[floating-damage]\ncritical_style = 'both'\n",rejected);
    Check(rejected==0 && both.criticalStyle==CriticalStyle::Both,"explicit combined critical style parses");
    for(const auto invalid:{"critical_style=\"large\"", "critical_style=both", "critical_style=\"BOTH\"", "critical_style=\"both",
                           "critical_style=1", "resolution_scaling=maybe", "resolution_scaling=\"true\"", "resolution_scaling=1"}) {
        char input[128]{};std::snprintf(input,sizeof(input),"[floating-damage]\n%s\n",invalid);
        const auto refused=Parse(input,rejected);
        Check(rejected==1 && refused.criticalStyle==CriticalStyle::Both && refused.resolutionScaling,
            "unsupported or malformed styling settings are rejected without replacing defaults");
    }
    Settings scaleConfig;
    struct Viewport {float width,height,expected;};
    for(const auto view:{Viewport{1920,1080,1},Viewport{3840,2160,2},Viewport{2560,1440,4.0f/3},
                         Viewport{1280,720,2.0f/3},Viewport{3440,1440,4.0f/3},Viewport{1920,1200,1},
                         Viewport{1600,1080,5.0f/6},Viewport{320,180,0.25f},Viewport{10240,5760,4}}) {
        Check(std::abs(ResolutionScale(view.width,view.height,scaleConfig)-view.expected)<0.0001f,
            "viewport scaling uses the smaller 1080p-relative dimension and stays within bounded limits");
    }
    const auto maximum=Parse("[floating-damage]\ntext_size=64\ncritical_size_scale=2\ncritical_style=\"both\"\n",rejected);
    Number maximumDisplay{};maximumDisplay.critical=true;
    const float maximumScale=ResolutionScale(15360,8640,maximum);
    Check(rejected==0 && maximumScale==4 && TextSize(maximumDisplay,maximum,maximumScale)==512,
        "maximum admitted font, resolution and critical multiplier reach the inclusive SDK limit of 512");
    maximumDisplay.critical=false;
    Check(TextSize(maximumDisplay,maximum,maximumScale)==256,"maximum unmarked text remains within the same SDK font limit");
    const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
    for(const auto view:{Viewport{0,1080,1},Viewport{1920,0,1},Viewport{-1,1080,1},Viewport{1920,-1,1},
                         Viewport{nan,1080,1},Viewport{1920,nan,1},Viewport{inf,1080,1},Viewport{1920,inf,1}}) {
        Check(ResolutionScale(view.width,view.height,scaleConfig)==1,"invalid viewport dimensions safely retain baseline pixel sizes");
    }
    scaleConfig.resolutionScaling=false;
    Check(ResolutionScale(3840,2160,scaleConfig)==1 && ResolutionScale(320,180,scaleConfig)==1,"resolution scaling can be disabled at any viewport size");
    scaleConfig=Settings{};Number moving{};moving.target.id=17;moving.lane=2;
    for(const bool falling:{false,true}) for(const float age:{0.0f,0.42f,0.75f,1.0f}) {
        const auto baseline=Motion(moving,age,scaleConfig,falling),scaled=Motion(moving,age,scaleConfig,falling,2);
        Check(std::abs(scaled.x-baseline.x*2)<0.001f && std::abs(scaled.y-baseline.y*2)<0.001f,
            "viewport scaling applies to rise, fall, side drift, anchor offset and lane spacing in both motion modes");
    }
    const auto start=Motion(moving,0,scaleConfig,false,2);
    const auto peak=Motion(moving,0.42f,scaleConfig,true,2);
    Check(start.x==0 && start.y==-180 && std::abs(peak.x-16.9344f)<0.001f && peak.y==-308,
        "4K motion doubles the configured pixel distances rather than animation duration");
    Check(OutlineRadius(scaleConfig)==2 && OutlineRadius(scaleConfig,2)==4 && OutlineRadius(scaleConfig,0.25f)==1 &&
        OutlineRadius(scaleConfig,1.2f)==3,"outline coverage rounds scaled fractional radii upward to dense pixels");
    scaleConfig.outlineWidth=3;Check(OutlineRadius(scaleConfig,4)==12 && OutlineRadius(scaleConfig,5)==12,"outline radius is capped at twelve pixels");
    scaleConfig.outlineWidth=0;Check(OutlineRadius(scaleConfig,2)==0,"zero outline stays disabled under resolution scaling");
    std::puts("Native critical association, presentation modes, viewport scaling, motion and bounded config pass.");
}
