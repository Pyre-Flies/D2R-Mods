#include "style.h"
#include "config.h"
#include <cstdlib>
#include <cstring>
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
    std::puts("Native hit association, critical formatting, death timing, mixed merging, poison isolation and bounded config pass.");
}
