#include "controls.h"
#include "style.h"
#include <cstdio>
#include <cstdlib>
using namespace Damage;
void Check(bool ok,const char* name) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",name);std::exit(1);}}
std::size_t Count(const Model& model) {return static_cast<std::size_t>(std::count_if(model.numbers.begin(),model.numbers.end(),[](const Number& n){return n.raw>0;}));}
int main() {
    Settings config; Model model; Monster enemy{100,17,5,1755,3584,{100,100}};
    auto sample=[&](std::uint64_t time){model.Sample(std::span(&enemy,1),1,1,time,config);};
    sample(100); Check(Count(model)==0,"first observation is a baseline");
    enemy.hp=660; sample(150);
    Check(Count(model)==1 && model.numbers[0].raw==1095,"live-captured 4.277 HP decrease preserves raw fraction");
    enemy.hp=800; sample(180); Check(Count(model)==1,"healing does not emit damage");
    enemy.hp=600; sample(220); Check(model.numbers[0].raw==1295,"small tick merges without losing fraction and healing updated baseline");
    enemy.hp=500; sample(550); Check(Count(model)==2 && model.numbers[0].lane!=model.numbers[1].lane,"separate hits get separate lanes");
    model.Sample({},1,1,600,config); Check(Count(model)==2,"disappearance does not fabricate lethal damage");
    enemy.hp=100; sample(650); Check(Count(model)==2,"reappearance establishes baseline");
    enemy.identity=200; enemy.hp=90; sample(700); Check(Count(model)==2,"reused ID with new address establishes baseline");
    model.Sample(std::span(&enemy,1),1,2,750,config); Check(Count(model)==0,"session change clears state");
    enemy.hp=0; model.Sample(std::span(&enemy,1),1,2,800,config);
    Check(Count(model)==1 && model.numbers[0].raw==90,"observed zero HP retains lethal remainder");
    model.Expire(1800,config); Check(Count(model)==0,"numbers expire at lifetime");
    model.Clear(); config.maximum=2; config.mergeMs=0; enemy.hp=2000;
    sample(2000); enemy.hp=1900; sample(2010); enemy.hp=1800; sample(2020); enemy.hp=1700; sample(2030);
    Check(Count(model)==2,"active number cap replaces oldest entry");
    Settings deathConfig; Model deaths; Monster dying{300,23,5,400,1024,{120,130}};
    deaths.Sample(std::span(&dying,1),10,7,3000,deathConfig);
    dying.hp=300; deaths.Sample(std::span(&dying,1),10,7,3030,deathConfig);
    Check(deaths.Death(23,5,10,7,3040,deathConfig)==300 && Count(deaths)==2,"confirmed death emits last remaining HP as a separate number");
    Check(deaths.numbers[1].target.position.x==120 && deaths.numbers[1].raw==300,"death number keeps tracked position and raw HP");
    Check(deaths.Death(23,5,10,7,3050,deathConfig)==0 && Count(deaths)==2,"duplicate death does not repeat remainder");
    dying.hp=0;deaths.Sample(std::span(&dying,1),10,7,3060,deathConfig);
    Check(Count(deaths)==2,"later zero HP observation does not repeat death number");
    deaths.Clear();dying.hp=660;deaths.Sample(std::span(&dying,1),10,7,4000,deathConfig);
    deaths.Sample({},10,7,4050,deathConfig);
    Check(Count(deaths)==0 && deaths.Death(23,5,10,7,4060,deathConfig)==660,"brief disappearance keeps HP for confirmed death only");
    deaths.Clear();dying.hp=660;deaths.Sample(std::span(&dying,1),10,7,5000,deathConfig);
    dying.hp=0;deaths.Sample(std::span(&dying,1),10,7,5030,deathConfig);
    Check(deaths.Death(23,5,10,7,5040,deathConfig)==0 && Count(deaths)==1,"polling zero before death callback is not duplicated");
    deaths.Clear();dying.hp=660;deaths.Sample(std::span(&dying,1),10,7,6000,deathConfig);
    Check(deaths.Death(24,5,10,7,6010,deathConfig)==0 && deaths.Death(23,6,10,7,6010,deathConfig)==0,"unknown or different-class deaths are ignored");
    Check(deaths.Death(23,5,11,7,6010,deathConfig)==0 && deaths.Death(23,5,10,8,6010,deathConfig)==0,"death must belong to sampled game and session");
    Check(deaths.Death(23,5,10,7,8001,deathConfig)==0,"expired death cache cannot emit stale HP");
    deaths.Clear();dying.hp=660;deaths.Sample(std::span(&dying,1),10,7,9000,deathConfig);
    dying.identity=400;dying.hp=800;deaths.Sample(std::span(&dying,1),10,7,9030,deathConfig);
    Check(deaths.Death(23,5,10,7,9040,deathConfig)==800,"reused unit ID replaces stale death cache baseline");
    unsigned rejected{};
    const auto parsed=Parse("[floating-damage]\ntext_size=32\ndecimal_places=1\nmax_numbers=999\nrise_pixels=nan\n",rejected);
    Check(parsed.textSize==32 && parsed.precision==1 && parsed.maximum==128 && rejected==2,"invalid settings retain bounded defaults");
    const auto styled=Parse("[floating-damage]\ntext_color=[0.2, 0.8, 1]\noutline_color=[0.1, 0.2, 0.3]\noutline_width=3\nfade_start=0.95\ndebug=false\n",rejected);
    Check(!styled.debug && styled.textColor[0]==0.2f && styled.textColor[2]==1 && styled.outlineColor[1]==0.2f &&
        styled.outlineWidth==3 && styled.fadeStart==0.95f && rejected==0,"RGB colors and bounded stroke/fade settings parse");
    const auto badStyle=Parse("[floating-damage]\ntext_color=[1, 0]\noutline_color=[0, nan, 0]\noutline_width=4\nfade_start=1\ntext_color=[1, 2, 0]\ntext_color=[1, 0, 0, 0]\n",rejected);
    Check(badStyle.textColor==Settings{}.textColor && badStyle.outlineColor==Settings{}.outlineColor &&
        badStyle.outlineWidth==2 && badStyle.fadeStart==0.8f && rejected==6,"invalid colors cannot partly replace defaults or make fade divide by zero");
    const auto hex=Parse("[floating-damage]\ntext_color=\"#ff7F00\" # warm orange\noutline_color='#123456'\nshadow_color=\"#000001\"\nfalling_motion=false\nshadow=false\nshadow_offset_x=-4\nshadow_opacity=0.5\nshadow_softness=2\n",rejected);
    Check(rejected==0 && hex.textColor[0]==1 && hex.textColor[1]==127.0f/255 && hex.textColor[2]==0 &&
        hex.outlineColor[2]==86.0f/255 && hex.shadowColor[2]==1.0f/255 && !hex.fallingMotion && !hex.shadow &&
        hex.shadowX==-4 && hex.shadowOpacity==0.5f && hex.shadowSoftness==2,"quoted hex survives comments, accepts case/literal strings and shadow controls");
    const auto badHex=Parse("[floating-damage]\ntext_color=\"#GG0000\"\noutline_color=\"#00000000\"\nshadow_color=\"#123\"\nshadow_opacity=1.1\nshadow_softness=3\nfalling_motion=maybe\n",rejected);
    Check(rejected==6 && badHex.textColor==Settings{}.textColor && badHex.outlineColor==Settings{}.outlineColor &&
        badHex.shadowColor==Settings{}.shadowColor && badHex.shadowOpacity==0.75f && badHex.shadowSoftness==1,
        "invalid hex and shadow values keep complete defaults");
    const auto backed=Parse("[floating-damage]\nbacking=true\nbacking_poison_only=false\nbacking_color=\"#102030\"\nbacking_opacity=0.5\nbacking_padding_x=8\nbacking_padding_y=0\nbacking_rounding=12\n",rejected);
    Check(rejected==0 && backed.backing && !backed.backingPoisonOnly && backed.backingColor[2]==48.0f/255 &&
        backed.backingOpacity==0.5f && backed.backingPaddingX==8 && backed.backingPaddingY==0 && backed.backingRounding==12,
        "backing settings accept bounded geometry, opacity and hex color");
    const auto badBacking=Parse("[floating-damage]\nbacking=maybe\nbacking_poison_only=0\nbacking_color=\"#123\"\nbacking_opacity=1.1\nbacking_padding_x=-1\nbacking_padding_y=13\nbacking_rounding=nan\n",rejected);
    Check(rejected==7 && badBacking.backingOpacity==Settings{}.backingOpacity && badBacking.backingPaddingX==4 && badBacking.backingPaddingY==1 && badBacking.backingRounding==3,
        "invalid backing settings preserve safe defaults");
    Check(ShowBacking(Poison,Settings{}) && ShowBacking(Poison|Physical,Settings{}) && !ShowBacking(Magic,Settings{}) &&
        !ShowBacking(Unclassified,Settings{}) && !ShowBacking(Poison|Unsupported,Settings{}) && ShowBacking(Unclassified,backed),
        "poison-only backing includes known mixed poison and all-types mode includes fallback");
    auto noBacking=Settings{};noBacking.backing=false;Check(!ShowBacking(Poison,noBacking),"backing can be disabled");
    noBacking=Settings{};noBacking.elementColors=false;Check(!ShowBacking(Poison,noBacking),"poison-only backing follows element detection availability");
    const auto motionRequest=ParseRequest("  MoTiOn off  ");
    Check(motionRequest.motion && motionRequest.action==Control::Off && ParseRequest("motion").action==Control::Toggle &&
        ParseRequest("motion on extra").action==Control::Invalid && !ParseRequest("on").motion,"motion command is independent and rejects extra arguments");
    Number moving{};moving.target.id=17; Settings visuals;
    const auto start=Motion(moving,0,visuals,true),top=Motion(moving,0.42f,visuals,true),later=Motion(moving,0.8f,visuals,true),finish=Motion(moving,1,visuals,true);
    Check(start.x==0 && start.y==-visuals.offset && std::abs(top.y+visuals.offset+visuals.rise)<0.001f &&
        later.y>top.y && finish.y>later.y && finish.x==visuals.side,"falling motion rises to a peak then falls/drifts to the side");
    moving.lane=1;const auto other=Motion(moving,1,visuals,true);
    Check(other.x==-finish.x && other.y==finish.y-visuals.laneSpacing,"adjacent lanes drift to opposite sides with stable spacing");
    const auto straight=Motion(moving,0.5f,visuals,false);
    Check(straight.x==0 && straight.y==-visuals.offset-visuals.laneSpacing-visuals.rise*0.5f,"motion off retains the original straight rise");
    Check(Opacity(0.8f,visuals)==1 && std::abs(Opacity(0.9f,visuals)-0.5f)<0.001f && Opacity(1,visuals)==0,
        "fill/outline/shadow can fade together at the configured threshold");
    for(unsigned radius=0;radius<=2;++radius) {
        float total{};const auto extent=static_cast<int>(radius);
        for(int x=-extent;x<=extent;++x) for(int y=-extent;y<=extent;++y) total+=ShadowWeight(x,y,radius);
        Check(std::abs(total-1)<0.001f,"shadow kernel conserves configured opacity across softness levels");
    }
    Check(ParseControl("")==Control::Toggle && ParseControl(" TOGGLE ")==Control::Toggle &&
        ParseControl("\tON\r")==Control::On && ParseControl("off")==Control::Off && ParseControl("Status")==Control::Status,
        "command accepts empty toggle, explicit actions, case and outer whitespace");
    Check(ParseControl("on off")==Control::Invalid && ParseControl("off extra")==Control::Invalid &&
        ParseControl(std::string_view("on\0off",6))==Control::Invalid,"extra and embedded-null arguments are rejected");
    Model controlled; bool controlEnabled=true; Monster controlledEnemy{500,24,5,1000,2000,{120,130}};
    controlled.Sample(std::span(&controlledEnemy,1),10,1,100,config);
    controlledEnemy.hp=900; controlled.Sample(std::span(&controlledEnemy,1),10,1,130,config);
    Check(Count(controlled)==1 && !ApplyControl(Control::Status,controlEnabled,controlled) &&
        !ApplyControl(Control::On,controlEnabled,controlled) && Count(controlled)==1,"status and repeated on preserve active numbers");
    Check(ApplyControl(Control::Off,controlEnabled,controlled) && !controlEnabled && Count(controlled)==0 &&
        controlled.Death(24,5,10,1,140,config)==0,"off clears numbers and cached death baselines");
    controlledEnemy.hp=200; // HP lost during the disabled interval is not sampled.
    Check(ApplyControl(Control::Toggle,controlEnabled,controlled) && controlEnabled,"toggle enables a disabled runtime");
    controlled.Sample(std::span(&controlledEnemy,1),10,2,200,config);
    Check(Count(controlled)==0,"reenabling creates a fresh baseline rather than reporting damage while off");
    controlledEnemy.hp=100; controlled.Sample(std::span(&controlledEnemy,1),10,2,230,config);
    Check(Count(controlled)==1 && controlled.numbers[0].raw==100,"fresh damage appears after reenabling");
    Projection projection{{200,200},{100,100},{102,100},{100,102},40};Point screen{};
    Check(projection.Project({101,101},screen) && screen.x==220 && screen.y==220,"world basis is inverted correctly");
    projection.worldY=projection.worldX;Check(!projection.Project({101,101},screen),"singular projection is refused");
    std::puts("HP loss, confirmed deaths, identity/session lifetime, command transitions, styling config and projection pass.");
}
