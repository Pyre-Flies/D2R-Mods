#include "aim_policy.h"
#include "aim_config.h"
#include "snap_policy.h"
#include "coexistence_policy.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <limits>
void Check(bool b,const char* message) {
    if (!b) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
int main() {
    using namespace Aim;
    MotionSettings integrated{}; bool enabled=true;
    Check(ParseQolSettings("[qol]\nenabled=true",integrated,enabled) && enabled,"missing aim section defaults enabled");
    Check(ParseQolSettings("[qol]\nenabled=true\n[aim]\nenabled=false",integrated,enabled) && !enabled,"qol enabled cannot enable aim");
    Check(ParseQolSettings("[aim]\nenabled=true\ninitial_speed=25\nmaximum_speed=100\nacceleration_seconds=0.05\n[qol]\nenabled=false",integrated,enabled) && enabled && integrated.initialSpeed==25,"aim parsed independently of later qol section");
    Check(!ParseQolSettings("[aim]\nenabled=true\nenabled=false",integrated,enabled),"duplicate aim enable rejected");
    Check(!ParseQolSettings("[aim]\nenabled=true\n[aim]\nenabled=false",integrated,enabled),"duplicate aim sections rejected");
    Check(!ParseQolSettings("[aim]\nenabled=1",integrated,enabled),"malformed enable cannot activate aim");
    Check(!ParseQolSettings("[aim]\nenabled=true\nunknown=2",integrated,enabled),"unknown aim key disables feature");
    std::string qol;
    Check(QolAim::Section("[aim]\nenabled=true\n[qol] # settings\nenabled=false # comment\n[other]\nenabled=true","qol",qol) && qol=="enabled=false\n","QOL settings exclude other sections and comments");
    for(int skill:std::array<int,10>{12,22,51,54,56,59,62,67,132,151}) {
        Check(Supported(skill) && SkillName(skill),"all requested skills have explicit identity");
        Check(SnapSkill(skill)==(skill!=Teleport && skill!=Leap),"Teleport/Leap ground-only; other requested skills snap-capable");
        Check(UseCoordinateTarget(skill,skill),"requested skill uses coordinate route");
    }
    Check(MeteorObservation(Whirlwind,Meteor)==ObservationMode::MeteorCircle,"active Whirlwind enables circular acquisition");
    Check(MeteorObservation(-1,Whirlwind)==ObservationMode::MeteorPreviewCircle,"idle after Whirlwind prepares next snap skill");
    Check(!UseCoordinateTarget(Whirlwind,Meteor) && !UseCoordinateTarget(Meteor,Whirlwind),"preview cannot authorize mismatched Whirlwind route");
    Check(MeteorObservation(Leap,Whirlwind)==ObservationMode::None,"Leap never inherits Whirlwind enemy scoring");
    Check(MeteorObservation(-1,Leap)==ObservationMode::MeteorPreviewCircle,"idle after Leap prepares snap skills");
    Check(!Supported(143),"Leap Attack remains outside requested allowlist");
    Check(GroundSnapPreview(-1,Teleport,true),"idle Teleport UI previews next snap");
    Check(GroundSnapPreview(-1,Leap,true),"idle Leap UI previews next snap");
    Check(!GroundSnapPreview(Teleport,Teleport,true),"active Teleport never previews a snap");
    Check(!GroundSnapPreview(Leap,Leap,true),"active Leap remains ground only");
    Check(!GroundSnapPreview(-1,Teleport,false),"cast lookup cannot opt into UI preview");
    Check(!GroundSnapPreview(143,Teleport,true) && !GroundSnapPreview(-1,143,true),"unsupported skills cannot inherit preview");
    Point endpoint{};
    Check(WhirlwindEndpoint({100,100},{110,100},endpoint) && endpoint.x==111.5f,"Whirlwind extends 1.5 tiles past enemy");
    Check(WhirlwindEndpoint({111.5f,100},{110,100},endpoint) && endpoint.x==108.5f,"next spin reverses across retained enemy");
    Check(WhirlwindEndpoint({100,100},{129,100},endpoint) && endpoint.x==130,"Whirlwind endpoint respects range cap");
    Check(!WhirlwindEndpoint({100,100},{100,100},endpoint),"coincident enemy falls back without undefined direction");
    Check(!WhirlwindEndpoint({100,100},{131,100},endpoint),"out of range enemy excluded");
    SnapBook whirlBook{};
    whirlBook.Observe(1,42,{110,100},1000,true);
    Check(whirlBook.Choose(1,{110,100},{100,100},1000,nullptr,6,1.5f,true).id==42,"Whirlwind initially acquires in cursor circle");
    whirlBook.Observe(1,42,{110,100},1050,true);
    Check(whirlBook.Choose(1,{133,100},{113,100},1050,nullptr,6,1.5f,true).id==42,"retained enemy survives passing through and cursor drift");
    Check(!whirlBook.Choose(1,{133,100},{113,100},1300,nullptr,6,1.5f,true).valid,"Whirlwind lock never uses stale positions");
    whirlBook.Observe(1,42,{110,100},1400,true);
    whirlBook.Choose(1,{110,100},{100,100},1400,nullptr,6,1.5f,true);
    whirlBook.Observe(1,42,{110,100},1410,false);
    Check(!whirlBook.Choose(1,{110,100},{100,100},1410,nullptr,6,1.5f,true).valid,"native rejection releases Whirlwind lock");
    Check(WhirlwindEndpoint({100,100},{110,100},endpoint,4) && endpoint.x==114,"configured overshoot applied");
    Check(WhirlwindEndpoint({100,100},{110,100},endpoint,0) && endpoint.x==110,"disabled overshoot targets enemy directly");
    Check(WhirlwindEndpoint({110,100},{110,100},endpoint,0) && endpoint.x==110,"zero extension allows coincident target");
    Check(!WhirlwindEndpoint({100,100},{110,100},endpoint,-1),"invalid overshoot rejected");
    MotionSettings whirlSettings{};
    Check(ParseSettings("[aim]",whirlSettings) && whirlSettings.whirlwindPassThrough && whirlSettings.whirlwindPassThroughDistance==1.5f,"old config retains Whirlwind defaults");
    Check(ParseSettings("[aim]\nwhirlwind_pass_through_enabled=false\nwhirlwind_pass_through_distance=4",whirlSettings) && !whirlSettings.whirlwindPassThrough && whirlSettings.whirlwindPassThroughDistance==4,"Whirlwind config parsed");
    Check(!ParseSettings("[aim]\nwhirlwind_pass_through_distance=16",whirlSettings),"overshoot above limit rejected");
    Check(!ParseSettings("[aim]\nwhirlwind_pass_through_distance=nan",whirlSettings),"nonfinite overshoot rejected");
    Check(!ParseSettings("[aim]\nwhirlwind_pass_through_enabled=1",whirlSettings),"overshoot toggle must be boolean");
    Point center{};
    Check(Center({100,100},{2,1},20,center),"valid aim direction");
    Check(std::abs(std::sqrt(DistanceSquared(center,{100,100}))-20)<0.001f,"normalized world distance");
    Check(!Center({100,100},{0,0},20,center),"zero facing fails open");
    Check(!Center({1,1},{-1,0},20,center),"world bounds fail open");
    center={120,100};
    for(int i=0;i<360;++i) {
        const float angle=i*0.01745329252f;
        const Point inside{center.x+5.99f*std::cos(angle),center.y+5.99f*std::sin(angle)};
        const Point outside{center.x+6.01f*std::cos(angle),center.y+6.01f*std::sin(angle)};
        Check(Score(center,inside)>0,"full circle admits every angle");
        Check(Score(center,outside)<0,"outside circle rejected");
    }
    Check(Score(center,{114,100})==1,"near side boundary included");
    Check(Score(center,{120,100})>Score(center,{123,100}),"closest candidate wins");
    Check(Score(center,{100,120})<0,"same distance around player is not snapping circle");
    Check(Supported(54)&&Supported(56)&&!Supported(23)&&!Supported(-1),"explicit allowlist excludes unrequested skill");
    Check(Fresh(1000,900)&&!Fresh(1000,700)&&!Fresh(1000,1001)&&!Fresh(1000,0),"snapshot lifetime");
    Projection projection{{400,300},{100,100},{110,90},{110,110},100};
    Point screen{};
    Check(projection.Project({120,100},screen)&&screen.x==500&&screen.y==400,"projection inverse");
    Point offset{};
    Check(EnemySnapping,"Meteor smart snapping enabled");
    for(const Point stick:std::array<Point,4>{{{1,0},{-1,0},{0,1},{0,-1}}}) {
        Check(StickDirection(stick,projection,false,0.22f,offset),"four stick directions admitted");
        Check(std::abs(std::hypot(offset.x,offset.y)-1)<0.001f,"unit world direction");
        Check(projection.Project({100+offset.x,100+offset.y},screen),"project direct aim");
        const Point delta{screen.x-400,screen.y-300};
        Check(delta.x*stick.x-delta.y*stick.y>0,"screen direction agrees with stick");
        Check(std::abs(delta.x*stick.y+delta.y*stick.x)<0.001f,"screen direction no isometric rotation");
    }
    MotionSettings settings{};
    CursorMotion motion{};
    offset={0,0};
    Check(!motion.Advance({0.15f,0.15f},projection,false,0.05f,settings,offset),"radial dead zone rejects drift");
    Check(motion.Advance({1,0},projection,false,0.05f,settings,offset),"full input tap accepted");
    const float tap=std::hypot(offset.x,offset.y);
    Check(tap>0 && tap<0.3f,"50 ms full input tap stays below 0.3 tile");
    const Point retained=offset;
    Check(!motion.Advance({0,0},projection,false,0.05f,settings,offset)&&DistanceSquared(retained,offset)==0,"release stops immediately");
    Check(motion.heldSeconds==0,"release resets acceleration");
    auto travel=[&](int hz,Point stick) {
        CursorMotion cursor{}; Point point{};
        for(int i=0;i<hz;++i) cursor.Advance(stick,projection,false,1.0f/hz,settings,point);
        return std::hypot(point.x,point.y);
    };
    Check(std::abs(travel(60,{1,0})-travel(120,{1,0}))<0.002f,"acceleration independent of frame rate");
    Check(std::abs(travel(60,{1,0})-20.2f)<0.002f,"one second hold integrates configured ramp");
    Check(std::abs(travel(60,{0.61f,0})/travel(60,{1,0})-0.25f)<0.001f,"half usable tilt has quarter speed");
    Check(std::abs(travel(60,{1,1})-travel(60,{1,0}))<0.002f,"diagonal movement no faster");
    offset={29,0}; motion={};
    for(int i=0;i<200;++i) motion.Advance({1,1},projection,false,0.01f,settings,offset);
    Check(std::hypot(offset.x,offset.y)<=30.001f,"cursor bounded at outer radius");
    offset={0.1f,0}; motion={};
    for(int i=0;i<60;++i) motion.Advance({-1,1},projection,false,1.0f/60,settings,offset);
    Check(offset.x<0,"cursor crosses player center without inner-radius trap");
    const auto before=offset;
    Check(!motion.Advance({1,0},projection,false,1.0f,settings,offset)&&DistanceSquared(before,offset)==0,"pause does not jump");
    settings.accelerationSeconds=0; offset={}; motion={};
    motion.Advance({1,0},projection,false,0.05f,settings,offset);
    Check(std::abs(std::hypot(offset.x,offset.y)-1.4f)<0.001f,"zero acceleration time immediately uses max speed");
    settings={}; settings.deadzone=0.5f; offset={};
    Check(!motion.Advance({0.4f,0},projection,false,0.05f,settings,offset),"configured deadzone applied");
    Check(ParseSettings("[aim]\ndeadzone = 0.3 # drift\ninitial_speed=2\nmaximum_speed=20\nacceleration_seconds=1.2\n",settings)&&settings.deadzone==0.3f&&settings.accelerationSeconds==1.2f,"numeric configuration parsed");
    Check(!ParseSettings("[aim]\ndeadzone=nan",settings),"nonfinite configuration rejected");
    Check(!ParseSettings("[aim]\ndeadzone=1",settings),"invalid deadzone rejected");
    Check(!ParseSettings("[aim]\ninitial_speed=40\nmaximum_speed=20",settings),"inverted speed limits rejected");
    Check(!ParseSettings("[aim]\ndeadzone=0.3\ndeadzone=0.4",settings),"duplicate configuration rejected");
    Check(!ParseSettings("[aim]\nmaximum_speed=20oops",settings),"trailing configuration junk rejected");
    Check(UseCoordinateTarget(Meteor,Meteor)&&UseCoordinateTarget(Teleport,Teleport),"active prototype skills choose coordinate routing");
    Check(!UseCoordinateTarget(Meteor,-1)&&!UseCoordinateTarget(Meteor,Teleport)&&!UseCoordinateTarget(357,357)&&!UseCoordinateTarget(23,23),"idle and other skill requests preserve native selected targets");
    Check(MeteorObservation(-1,Meteor)==ObservationMode::MeteorPreviewCircle,"idle Meteor preview uses circular geometry before the next cast");
    Check(MeteorObservation(Meteor,Meteor)==ObservationMode::MeteorCircle,"active Meteor admits circular scoring");
    Check(MeteorObservation(Teleport,Meteor)==ObservationMode::None && MeteorObservation(357,Meteor)==ObservationMode::None,"other active skills cannot inherit Meteor preview");
    Check(MeteorObservation(-1,Teleport)==ObservationMode::MeteorPreviewCircle,"idle after Teleport prepares the first Meteor cast");
    Check(MeteorObservation(-1,-1)==ObservationMode::None,"unestablished cursor does not gather candidates");
    SnapBook beforeCast{};
    beforeCast.Observe(1,42,{112,100},1000,true);
    Check(beforeCast.Choose(1,{110,100},{100,100},1010).id==42,"idle observation supplies next lookup destination");
    Check(!beforeCast.Choose(1,{110,100},{100,100},1200).valid,"idle sampling does not allow stale destination reuse");
    SnapBook book{};
    const Point player{100,100}, cursor{110,100};
    book.Observe(1,10,{112,100},100,true);
    book.Observe(1,20,{114,100},100,true);
    Check(book.Choose(1,cursor,player,100).id==10,"nearest eligible target acquired");
    book.Observe(1,20,{111,100},110,true);
    Check(book.Choose(1,cursor,player,110).id==10,"small challenger advantage does not flicker");
    book.Observe(1,20,{110,100},120,true);
    Check(book.Choose(1,cursor,player,120).id==20,"clearly closer challenger switches target");
    book.Observe(1,20,{110,100},121,false);
    Check(book.Choose(1,cursor,player,121).id==10,"native rejection drops retained target immediately");
    book.Observe(1,10,{113,100},130,true);
    Check(book.Choose(1,cursor,player,130).position.x==113,"retention uses refreshed position");
    Check(!book.Choose(1,cursor,player,281).valid,"stale observations fall back to ground");
    book={}; book.Observe(1,1,{110,100},300,true);
    Check(!book.Choose(2,cursor,player,300).valid,"candidate cannot cross player identity");
    Check(!book.Choose(1,{120,100},player,300).valid,"moving cursor away breaks lock");
    book={}; book.Observe(1,1,{131,100},400,true);
    Check(!book.Choose(1,{130,100},player,400).valid,"snap does not exceed outer cast radius");
    for(const Point side:std::array<Point,4>{{{110,100},{90,100},{100,110},{100,90}}}) {
        book={}; book.Observe(1,7,side,500,true);
        Check(book.Choose(1,side,player,500).id==7,"snapping covers left right forward rear independent of facing");
    }
    // Regression: configured user settings survive parsing with snapping enabled.
    Check(ParseSettings("[aim]\ndeadzone=0.22\ninitial_speed=25.0\nmaximum_speed=100.0\nacceleration_seconds=0.05",settings)&&settings.maximumSpeed==100&&settings.initialSpeed==25&&settings.accelerationSeconds==0.05f,"user cursor settings preserved");
    Check(ParseSettings("[aim]\nsnapping_enabled=false\noverlay_enabled=false\ndebug_overlay=true\nsnap_radius=4.5\nswitch_advantage=2.0\nprojection_hz=60\noverlay_smoothing_ms=35",settings)&&!settings.snapping&&!settings.overlay&&settings.debugOverlay&&settings.snapRadius==4.5f,"toggles and tuning parse");
    Check(!ParseSettings("[aim]\nprojection_hz=0",settings)&&!ParseSettings("[aim]\nsnapping_enabled=1",settings),"invalid sampler and nonboolean toggle rejected");
    book={}; book.Observe(1,10,{115,100},100,true);
    Check(!book.Choose(1,{110,100},{100,100},100,nullptr,4,0).valid,"configured snap radius excludes outside candidate");
    Check(book.Choose(1,{110,100},{100,100},100,nullptr,6,0).valid,"configured larger radius admits candidate");
    DisplayProjection smoothing{};
    auto first=smoothing.Update(projection,1000,35);
    auto moved=projection; moved.worldOrigin.x+=1; moved.worldX.x+=1; moved.worldY.x+=1;
    auto filtered=smoothing.Update(moved,1016,35);
    Check(filtered.worldOrigin.x>first.worldOrigin.x && filtered.worldOrigin.x<moved.worldOrigin.x,"display-only camera correction interpolates");
    auto jump=moved; jump.worldOrigin.x+=20; jump.worldX.x+=20; jump.worldY.x+=20;
    Check(smoothing.Update(jump,1032,35).worldOrigin.x==jump.worldOrigin.x,"teleport-sized projection jump resets filter");
    Check(smoothing.Update(projection,1048,0).worldOrigin.x==projection.worldOrigin.x,"zero smoothing uses raw projection");
    projection.worldY=projection.worldX;
    Check(!projection.Project(center,screen),"degenerate projection rejected");
    std::array<unsigned char,2480> expected{}, actual{};
    expected[0x8dd]=actual[0x8dd]=0xe8; expected[0x988]=actual[0x988]=0xe8;
    actual[0x8de]=42; actual[0x989]=99;
    Check(SameEnumeration(actual,expected),"known contact displacements admitted for separate validation");
    actual[0x8dd]=0xe9;
    Check(!SameEnumeration(actual,expected),"contact opcode replacement rejected");
    actual[0x8dd]=0xe8; actual[123]=1;
    Check(!SameEnumeration(actual,expected),"unrelated enumeration mutation rejected");
    std::puts("Distance, full-circle selection, skill isolation, lifetime and rendering policy passed.");
}
