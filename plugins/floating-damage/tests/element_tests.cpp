#include "style.h"
#include "config.h"
#include "controls.h"
#include <cstdio>
#include <cstdlib>
using namespace Damage;
void Check(bool value,const char* name) {if(!value){std::fprintf(stderr,"FAIL: %s\n",name);std::exit(1);}}
void Put(CombatPacket& packet,unsigned offset,std::int32_t value) {std::memcpy(packet.bytes.data()+offset,&value,sizeof(value));}
int main() {
    CombatPacket packet{};
    Put(packet,0x20,10132); Put(packet,0x134,10132);
    Check(packet.Elements()==Fire,"captured Fire Bolt identifies fire");
    packet={}; Put(packet,0x18,1120); Put(packet,0x134,1120);
    Check(packet.Elements()==Physical,"captured plain attack identifies physical");
    packet={}; Put(packet,0x34,5714); Put(packet,0x134,5714);
    Check(packet.Elements()==Cold,"captured Cold Bolt identifies cold");
    packet={}; Put(packet,0x2c,48499); Put(packet,0x134,48499);
    Check(packet.Elements()==Lightning,"captured Lightning identifies lightning");
    Put(packet,0x18,100); Put(packet,0x134,48599);
    Check(packet.Elements()==(Physical|Lightning),"verified mixed components remain mixed");
    packet={};Put(packet,0x30,1641);Put(packet,0x134,1641);
    Check(packet.Elements()==Magic,"captured Teeth identifies magic");
    packet={};Put(packet,0x38,296);Put(packet,0x3c,50);Put(packet,0x134,296);
    Check(packet.Elements()==Poison,"captured Poison Nova identifies poison");
    Put(packet,0x18,100);Put(packet,0x134,396);Check(packet.Elements()==(Poison|Physical),"poison and physical payloads retain both types");
    Put(packet,0x30,1);Check(packet.Elements()==Unsupported,"a total missing one component remains unsupported");
    packet={}; Put(packet,0x20,100); Put(packet,0x134,99);
    Check(packet.Elements()==Unsupported,"unaccounted total is ambiguous");
    Put(packet,0x20,-1); Check(packet.Elements()==Unsupported,"negative components are refused");

    Settings config; config.mergeMs=0; Model model;
    Monster enemy{100,17,5,1000,2000,{100,100}};
    model.Sample(std::span(&enemy,1),1,7,100,config);
    model.RecordHit(enemy,700,true,Fire,1,7,110,115);
    enemy.hp=700; model.Sample(std::span(&enemy,1),1,7,130,config);
    Check(model.numbers[0].raw==300 && model.numbers[0].elements==Fire,"color is correlated with exact authoritative HP endpoints");
    model.RecordHit(enemy,500,true,Cold,1,7,140,145);
    auto next=enemy; next.hp=500;
    model.RecordHit(next,400,true,Lightning,1,7,150,155);
    enemy.hp=400; model.Sample(std::span(&enemy,1),1,7,160,config);
    Check(model.numbers[1].raw==300 && model.numbers[1].elements==(Cold|Lightning),"two hits between samples combine colors and retain HP total");
    enemy.hp=300; model.Sample(std::span(&enemy,1),1,7,180,config);
    Check(model.numbers[2].elements==Unclassified,"missing native observation uses fallback");
    model.Clear(); enemy.hp=1000; model.Sample(std::span(&enemy,1),1,7,200,config);
    model.RecordHit(enemy,900,true,Fire,2,7,210,211);
    model.RecordHit(enemy,900,true,Fire,1,8,210,211);
    auto reused=enemy; reused.identity=200;
    model.RecordHit(reused,900,true,Fire,1,7,210,211);
    enemy.hp=900; model.Sample(std::span(&enemy,1),1,7,230,config);
    Check(model.numbers[0].elements==Unclassified,"foreign games, sessions and recycled identities cannot color a number");
    model.Clear(); enemy.hp=1792; model.Sample(std::span(&enemy,1),1,7,300,config);
    Check(model.Death(17,5,1,7,310,config)==1792,"lethal number retains seven HP, not 39 HP overkill");
    model.RecordHit(enemy,0,false,Fire,1,7,305,315);
    Check(model.numbers[0].raw==1792 && model.numbers[0].elements==Fire,"post-call packet can color death emitted inside original call");
    model.Clear(); enemy.hp=1000; model.Sample(std::span(&enemy,1),1,7,400,config);
    model.RecordHit(enemy,0,false,Fire,1,7,410,415);
    enemy.hp=900; model.Sample(std::span(&enemy,1),1,7,430,config);
    Check(model.numbers[0].elements==Unclassified,"invalid HP read without confirmed death is never treated as zero");
    model.Clear(); config.mergeMs=120; enemy.hp=1000; model.Sample(std::span(&enemy,1),1,7,500,config);
    model.RecordHit(enemy,900,true,Fire,1,7,510,515);
    enemy.hp=900; model.Sample(std::span(&enemy,1),1,7,530,config);
    model.RecordHit(enemy,800,true,Cold,1,7,540,545);
    enemy.hp=800; model.Sample(std::span(&enemy,1),1,7,560,config);
    Check(model.numbers[0].raw==200 && model.numbers[0].elements==(Fire|Cold),"small ticks merge with component union");
    enemy.hp=950; model.Sample(std::span(&enemy,1),1,7,580,config);
    model.RecordHit(enemy,900,true,Fire,1,7,590,595);
    enemy.hp=900; model.Sample(std::span(&enemy,1),1,7,600,config);
    Check(model.numbers[0].raw==250 && model.numbers[0].elements==Unclassified,"healing inside merged lifetime makes classification conservative");
    model.Clear(); config.mergeMs=0; enemy.hp=1000; model.Sample(std::span(&enemy,1),1,7,700,config);
    auto dead=enemy; dead.hp=0; model.Sample(std::span(&dead,1),1,7,710,config);
    model.RecordHit(enemy,0,false,Lightning,1,7,705,715);
    Check(model.numbers[0].elements==Lightning && model.Death(17,5,1,7,720,config)==0,"valid zero sample can color without repeating lethal loss");
    Check(TextColor(Fire,config)==config.fireColor && TextColor(Fire|Cold,config)==config.mixedColor &&
        TextColor(Unsupported|Fire,config)==config.textColor,"known, mixed and unsupported choose appropriate colors");
    model.Clear();enemy.hp=1792;model.Sample(std::span(&enemy,1),1,7,800,config);
    Check(model.RecordHit(enemy,0,false,Fire,1,7,810,815,true)==Model::HitResult::PendingDeath && model.numbers[0].raw==0,
        "potential killing record waits for delayed death without inventing HP loss");
    Check(model.Death(17,5,1,7,850,config)==1792 && model.numbers[0].elements==Fire,
        "death after original call resolves retained component while preserving remaining HP");
    model.Clear();enemy.hp=1000;model.Sample(std::span(&enemy,1),1,7,1000,config);
    model.RecordHit(enemy,0,false,Fire,1,7,1010,1015,true);
    Check(model.Death(17,5,1,7,1516,config)==1000 && model.numbers[0].elements==Unclassified,
        "late death does not reuse an expired unresolved element");
    model.Clear();model.Sample(std::span(&enemy,1),1,7,1600,config);
    model.RecordHit(enemy,0,false,Fire,1,7,1610,1615,true);
    model.RecordHit(enemy,0,false,Cold,1,7,1630,1635,true);
    Check(model.Death(17,5,1,7,1650,config)==1000 && model.numbers[0].elements==Cold,
        "later readable pre-hit HP invalidates an earlier pending terminal record");
    model.Clear();model.Sample(std::span(&enemy,1),1,7,1700,config);
    model.RecordHit(enemy,0,false,Lightning,1,7,1710,1715,true);
    auto zero=enemy;zero.hp=0;model.Sample(std::span(&zero,1),1,7,1750,config);
    Check(model.numbers[0].raw==1000 && model.numbers[0].elements==Lightning,
        "valid zero sample also resolves a deferred endpoint");
    model.Clear();model.Sample(std::span(&enemy,1),1,7,1800,config);
    model.RecordHit(enemy,0,false,Fire,1,7,1810,1815,true);
    model.Sample({},1,1,1830,config);Check(model.Death(17,5,1,1,1850,config)==0,"new session drops pending element history");

    // Replay the live Poison Nova sequence: application, four periodic drops,
    // then a supported death with just 56 raw units remaining (<1 displayed HP).
    model.Clear();config.mergeMs=120;enemy.hp=1536;enemy.poison={};
    model.Sample(std::span(&enemy,1),1,7,2000,config);
    model.RecordHit(enemy,1240,true,Poison,1,7,2005,2005,false,{296,101,150,true});
    enemy.hp=1240;enemy.poison={296,101,150,true};model.Sample(std::span(&enemy,1),1,7,2010,config);
    for(unsigned i=1;i<=4;++i) {
        enemy.hp=1240-296*static_cast<std::int64_t>(i);enemy.poison.frame=101+i;
        model.Sample(std::span(&enemy,1),1,7,2010+31*i,config);
        Check(model.numbers[0].raw==296*(i+1) && model.numbers[0].elements==Poison,"matching ongoing ticks keep merged poison green");
    }
    Check(model.Death(17,5,1,7,2165,config,106,true)==56 && model.numbers[1].raw==56 && model.numbers[1].elements==Poison,
        "confirmed periodic death keeps its actual sub-one-HP remainder and poison type");
    Check(model.Death(17,5,1,7,2166,config,106,true)==0,"periodic terminal number is not repeated");
    model.Clear();enemy.hp=8192;enemy.maximum=16384;enemy.poison={296,200,300,true};
    model.Sample(std::span(&enemy,1),1,7,2200,config);
    for(unsigned i=1;i<=11;++i) {
        enemy.hp-=296;enemy.poison.frame=200+i;model.Sample(std::span(&enemy,1),1,7,2200+31*i,config);
    }
    Check(model.numbers[0].elements==Poison && model.numbers[0].raw==3256,"owned interval records preserve merged classification beyond eight total frames");
    model.Clear();enemy.hp=1000;enemy.maximum=2000;enemy.poison={296,400,450,true};
    model.Sample(std::span(&enemy,1),1,7,2600,config);
    auto between=enemy;between.hp=704;between.poison.frame=401;
    model.RecordHit(between,600,true,Fire,1,7,2610,2611,false,between.poison);
    enemy.hp=600;enemy.poison.frame=401;model.Sample(std::span(&enemy,1),1,7,2630,config);
    Check(model.numbers[0].elements==(Poison|Fire),"a witnessed fire hit during a predicted poison interval stays mixed");
    model.Clear();enemy.hp=1000;enemy.poison={296,500,550,true};
    model.Sample(std::span(&enemy,1),1,7,2700,config);
    enemy.hp=700;enemy.poison.frame=501;model.Sample(std::span(&enemy,1),1,7,2730,config);
    Check(model.numbers[0].elements==Unclassified,"poison presence does not color a mismatched HP drop");
    model.Clear();enemy.hp=1000;enemy.poison={296,600,650,true};
    model.Sample(std::span(&enemy,1),1,7,2800,config);
    enemy.hp=704;enemy.poison={200,601,650,true};model.Sample(std::span(&enemy,1),1,7,2830,config);
    Check(model.numbers[0].elements==Unclassified,"a replaced poison rate refuses prediction");
    model.Clear();enemy.hp=1000;enemy.poison={296,700,701,true};
    model.Sample(std::span(&enemy,1),1,7,2900,config);
    enemy.hp=704;enemy.poison.frame=701;model.Sample(std::span(&enemy,1),1,7,2930,config);
    Check(model.numbers[0].elements==Unclassified,"expired poison refuses prediction");
    model.Clear();enemy.hp=1000;enemy.poison={296,800,850,true};
    model.Sample(std::span(&enemy,1),1,7,3000,config);
    enemy.hp=56;enemy.poison.frame=809;model.Sample(std::span(&enemy,1),1,7,3030,config);
    Check(model.numbers[0].elements==Unclassified,"an oversized frame gap refuses prediction");
    model.Clear();enemy.hp=512;enemy.poison={};model.Sample(std::span(&enemy,1),1,7,3100,config);
    model.RecordHit(enemy,0,false,Magic,1,7,3110,3111,true);
    Check(model.Death(17,5,1,7,3112,config)==512 && model.numbers[0].elements==Magic,"captured Teeth death retains its remaining HP and magic color");
    Check(TextColor(Poison,config)==config.poisonColor && TextColor(Magic,config)==config.magicColor,"new types use their distinct bright colors");
    model.Clear();enemy={100,38,237,36608,36608,{100,100}};
    model.Sample(std::span(&enemy,1),1,7,10000,config);
    model.RecordHit(enemy,36377,true,Poison,1,7,10001,10001,false,{231,1526,1576,true});
    enemy.hp=36377;enemy.poison={231,1526,1576,true};model.Sample(std::span(&enemy,1),1,7,10001,config);
    for(unsigned i=1;i<=26;++i) {
        enemy.hp-=231;enemy.poison.frame=1526+i;const auto tick=10001+40*i;
        model.Sample(std::span(&enemy,1),1,7,tick,config);
        bool matched=false;
        for(const auto& n:model.numbers) if(n.raw && n.last==tick) {matched=true;Check(n.elements==Poison,"captured surviving sequence stays poison across every merged group");}
        Check(matched,"every live poison interval produces its observed HP loss");
    }
    Check(enemy.hp==30371 && enemy.hp>0,"surviving capture ends with its observed HP and no fabricated death");
    const auto capture=ParseRequest("capture ON");
    Check(capture.capture && !capture.motion && capture.action==Control::On && ParseRequest("capture on extra").action==Control::Invalid,
        "bounded capture commands are independent of motion and feature controls");
    config.elementColors=false; Check(TextColor(Fire,config)==config.textColor,"config can disable all element colors");
    unsigned rejected{};
    auto parsed=Parse("[floating-damage]\nelement_colors=false\nfire_color=\"#FF0000\"\ncold_color=\"#00FFFF\"\nmagic_color=\"#C080FF\"\npoison_color=\"#60FF40\"\nmixed_color=\"#bad\"\n",rejected);
    Check(!parsed.elementColors && parsed.fireColor[0]==1 && parsed.fireColor[1]==0 && parsed.coldColor[2]==1 &&
        parsed.magicColor[0]==192.0f/255 && parsed.magicColor[1]==128.0f/255 && parsed.magicColor[2]==1 &&
        parsed.poisonColor[0]==96.0f/255 && parsed.poisonColor[1]==1 && parsed.poisonColor[2]==64.0f/255 && rejected==1,
        "element toggle and hex colors use existing bounded parser");
    std::puts("Captured fields, HP endpoint correlation, death timing, mixed/merged/unknown fallback and hex settings pass.");
}
