#include "aim_policy.h"
#include "aim_config.h"
#include "snap_policy.h"
#include "coexistence_policy.h"
#include "cast_observer_policy.h"
#include "skill_toggle.h"
#include "tree_layout.h"
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
    unsigned observerGuards=0,observerHooks=0;
    auto guardOk=[&]() noexcept { ++observerGuards; return true; };
    auto guardMismatch=[&]() noexcept { ++observerGuards; return false; };
    auto installOk=[&]() noexcept { ++observerHooks; return true; };
    auto installFailure=[&]() noexcept { ++observerHooks; return false; };
    Check(InstallCastObserver(false,guardOk,installOk)==CastObserverState::Disabled && observerGuards==0 && observerHooks==0,"disabled cast observer never reads or patches its site");
    Check(InstallCastObserver(true,guardMismatch,installOk)==CastObserverState::Unavailable && observerGuards==1 && observerHooks==0,"changed cast prefix is left untouched for another owner");
    Check(InstallCastObserver(true,guardOk,installFailure)==CastObserverState::Unavailable && observerHooks==1,"optional hook failure produces fallback rather than disabling essential aim");
    Check(InstallCastObserver(true,guardOk,installOk)==CastObserverState::Installed && observerHooks==2,"clean cast observer retains default full functionality");
    Check(ParseSettings("[aim]\ncast_observer_enabled=false",integrated) && !integrated.castObserver,"cast observer opt-out parses independently");
    Check(ParseSettings("[aim]",integrated) && integrated.castObserver,"old configs retain cast observer default");
    Check(!ParseSettings("[aim]\ncast_observer_enabled=1",integrated) && !ParseSettings("[aim]\ncast_observer_enabled=true\ncast_observer_enabled=false",integrated),"cast observer toggle must be unique boolean");
    // Native-observed Bow/Crossbow layout, leaf -> container -> tab -> root -> manager.
    std::array<LayoutNode,5> layout{{{601,182,132,130,0,0,1,false},{0,0,0,0,0,0,1,false},
        {19,5,0,0,0,0,1,false},{304,179,1420,1420,0.5f,0,1,false},{0,0,3586,2160,0,0,0.48935184f,false}}};
    ScreenRect icon{};Point badge{};float badgeRadius{};
    Check(LayoutBounds(layout,icon) && std::abs(icon.x-1329.56895f)<0.01f && std::abs(icon.y-179.10277f)<0.01f,"native-observed icon anchors and manager scale produce screen position");
    Check(IconBadge(icon,badge,badgeRadius) && badge.x+badgeRadius*1.3f<icon.x+icon.width && badge.y-badgeRadius*1.3f>icon.y,"corner crosshair stays fully inside icon bounds");
    layout[0].x=262;layout[0].y=563;ScreenRect different{};
    Check(LayoutBounds(layout,different) && different.x<icon.x && different.y>icon.y,"different skill layouts follow their own widget positions");
    layout.back().scale=1;Check(LayoutBounds(layout,different) && std::abs(different.width-132)<0.01f,"resolution scale affects icon dimensions without fixed grid coordinates");
    layout.back().scale=0;Check(!LayoutBounds(layout,different),"invalid UI scale refuses placement");
    Check(!IconBadge({0,0,10,10},badge,badgeRadius) && !LayoutBounds({},different),"tiny icons and missing ancestry cannot produce unsafe indicators");
    TreePress press;
    Check(!press.Update(false,true) && !press.Update(true,true) && !press.Update(true,true),"entering skill tree with R3 held does not toggle");
    Check(!press.Update(true,false) && press.Update(true,true) && !press.Update(true,true),"R3 toggles once on a fresh press, never repeats");
    Check(!press.Update(false,false) && !press.Update(true,false) && press.Update(true,true),"reopening tree preserves neutral button history");
    TreePress gaps;
    Check(!gaps.Update(false,false) && gaps.Update(true,true),"first R3 after neutral outside tree is accepted");
    Check(!gaps.Update(false,false,false) && !gaps.Update(true,true),"stale sample cannot turn a held R3 into a new press");
    Check(!gaps.Update(true,false) && gaps.Update(true,true),"release and re-press remain distinct after focus gap");
    TreeSequence edges;
    Check(!edges.Update(4,false) && edges.Update(5,true) && !edges.Update(5,true),"native R3 sequence accepts one press without UI-context priming or repeats");
    Check(!edges.Update(6,false) && !edges.Update(6,true),"press outside tree cannot be replayed after entering");
    Check(edges.Update(7,true),"short press released before UI sample is still observed");
    ReticleColor rgba{};
    Check(ParseReticleColor("\"#AABBCC\"",rgba) && std::abs(rgba.red-170.0f/255)<0.0001f && rgba.alpha==1,"RGB hex colors parse opaque");
    Check(ParseReticleColor("\"#aabbcc80\"",rgba) && std::abs(rgba.alpha-128.0f/255)<0.0001f,"case-insensitive RGBA color parses opacity");
    Check(!ParseReticleColor("\"#GG0000\"",rgba) && !ParseReticleColor("\"#abc\"",rgba),"malformed hex colors rejected");
    Check(ParseQolSettings("[aim]\nground_reticle_color=\"#FFFFFF\" # bright\nlock_reticle_color=\"#FFD166CC\"\nreticle_thickness=2.0",integrated,enabled) && integrated.groundReticleColor.red==1 && integrated.reticleThickness==2,"quoted hashes survive section/comment parsing with thickness");
    Check(!ParseSettings("[aim]\nreticle_thickness=0",integrated) && !ParseSettings("[aim]\nreticle_thickness=nan",integrated),"invalid thickness rejected");
    Check(!ParseSettings("[aim]\nground_reticle_color=\"#FFFFFF\"\nground_reticle_color=\"#000000\"",integrated),"duplicate colors rejected");
    std::string toggled;
    const std::string original="[qol]\r\nenabled = true\r\n[aim.amazon]\r\n# Multi Shot\r\n\"12\" = true  # keep this\r\n\"22\" = true\r\n[aim]\r\nmaximum_speed=100\r\n";
    Check(RewriteSkillToggle(original,12,false,toggled) && toggled==std::string(original).replace(original.find("true  # keep"),4,"false"),"toggle preserves other settings comments and CRLF bytes");
    Check(RewriteSkillToggle(toggled,12,true,toggled) && toggled==original,"second toggle restores exact document");
    const std::string colored=original+"lock_reticle_color = \"#FFD166\" # keep color\r\n";
    Check(RewriteSkillToggle(colored,12,false,toggled) && toggled.find("\"#FFD166\" # keep color")!=toggled.npos,"R3 save preserves quoted color codes and comments");
    Check(RewriteSkillToggle("[aim]\nenabled=true",54,false,toggled) && toggled.find("[aim.sorceress]")!=toggled.npos,"missing class section added for legacy configs");
    Check(RewriteSkillToggle("[aim.sorceress]\n[aim]\nenabled=true",54,false,toggled) && toggled.find("\"54\" = false\n[aim]")!=toggled.npos,"missing key inserted before next section");
    Check(RewriteSkillToggle("[aim.amazon]\n\"Guided Arrow\"=true",22,false,toggled) && toggled.find("false")!=toggled.npos,"legacy supported name entry updated without duplicate ID");
    Check(!RewriteSkillToggle("[aim.amazon]\n\"12\"=true\n\"12\"=false",12,false,toggled),"ambiguous duplicate document refused");
    Check(!RewriteSkillToggle("[aim.custom]\n\"900\"=\"snap\"",900,false,toggled),"unknown skill mode not guessed by tree toggle");
    SkillSettings locked{};
    Check(ParseSkills("[aim.amazon]\n\"9\"=\"disabled\"\n\"12\"=false",locked) && !locked.Enabled(9) && !locked.CanToggle(9) && locked.CanToggle(12),"locked catalog skill differs from toggleable false");
    Check(!CoordinateRoute(true,9,9,locked) && !OverrideScoring(true,9,locked),"locked skill never routes or overrides scoring");
    const std::string lockedDocument="[aim.amazon]\r\n\"9\" = \"disabled\" # passive\r\n\"12\" = false\r\n";
    Check(!RewriteSkillToggle(lockedDocument,9,true,toggled) && !RewriteSkillToggle(lockedDocument,9,false,toggled),"R3 rewrite refuses both directions on locked skill");
    Check(RewriteSkillToggle(lockedDocument,12,true,toggled) && toggled.find("\"9\" = \"disabled\" # passive")!=toggled.npos,"other R3 toggles preserve locked skill bytes");
    Check(!ParseSkills("[aim.amazon]\n\"9\"=\"disable\"",locked),"malformed locked state rejected");
    LiveSkills lockedLive;
    Check(ParseSkills(lockedDocument,lockedLive.baseline),"locked live baseline parses");
    lockedLive.Publish(9,TargetMode::Snap);
    Check(!lockedLive.CanToggle(9) && !lockedLive.Enabled(9),"runtime overrides cannot enable locked baseline");
    LiveSkills live;
    live.Publish(12,TargetMode::Disabled);Check(!live.Enabled(12) && live.baseline.Enabled(12),"atomic runtime override leaves immutable baseline intact");
    live.Publish(12,TargetMode::Snap);Check(live.Snaps(12),"runtime enable restores catalog snap mode");
    Check(ParseSettings("[aim]\nskill_tree_toggle_enabled=false",integrated) && !integrated.skillTreeToggle,"skill-tree shortcut opt-out parses");
    SkillSettings skills{};
    Check(ParseSkills("[aim.custom]\n\"900\"=true\n\"901\"=false\n\"902\"=\"disabled\"\n[aim.targeting]\n\"900\"=\"ground\"\n\"901\"=\"ground\"\n\"902\"=\"snap\"",skills) && skills.Mode(900)==TargetMode::Ground && !skills.Enabled(901) && !skills.Enabled(902) && skills.Targeting(901)==TargetMode::Ground,"custom boolean states and targeting are independent");
    Check(ParseSkills("[aim.targeting]\n\"56\"=\"ground\"\n\"54\"=\"snap\"",skills) && skills.Mode(56)==TargetMode::Ground && skills.Snaps(54),"built-in targeting overrides work without repeating enable flags");
    const std::string targetingDocument="[aim.targeting]\n\"12\"=\"ground\" # retain\n[aim.amazon]\n\"12\"=false\n\"9\"=\"disabled\"";
    Check(RewriteSkillToggle(targetingDocument,12,true,toggled) && toggled.find("\"12\"=\"ground\" # retain")!=toggled.npos && ParseSkills(toggled,skills) && skills.Mode(12)==TargetMode::Ground,"R3 changes only enable state and retains targeting override");
    LiveSkills targetLive;
    Check(ParseSkills(targetingDocument,targetLive.baseline),"off skill remembers targeting override");
    targetLive.Publish(12,targetLive.Targeting(12));
    Check(targetLive.Mode(12)==TargetMode::Ground,"runtime R3 enable uses configured targeting rather than catalog default");
    Check(ParseSkills("[aim.amazon]\n\"9\"=\"disabled\"\n[aim.targeting]\n\"9\"=\"snap\"",skills) && !skills.Enabled(9) && !skills.CanToggle(9),"target override cannot enable locked skill");
    Check(!ParseSkills("[aim.targeting]\n\"900\"=\"snap\"",skills),"undeclared custom target rejected");
    Check(!ParseSkills("[aim.targeting]\n\"54\"=\"ground\"\n\"054\"=\"snap\"",skills),"duplicate override numeric identity rejected");
    Check(!ParseSkills("[aim.targeting]\n\"54\"=true",skills) && !ParseSkills("[aim.targeting]\n\"65535\"=\"snap\"",skills),"invalid targeting mode and ID rejected");

    Check(ReleaseForSkill(48,skills),"unsupported active skill releases manual aim without cast observer");
    Check(!ReleaseForSkill(-1,skills) && !ReleaseForSkill(Meteor,skills),"idle queries and supported active skills preserve manual intent");
    for(const auto& entry:skills.entries) if(entry.id && !skills.Enabled(entry.id)) {
        Check(!CoordinateRoute(true,entry.id,-1,skills) && !ReleaseForSkill(-1,skills),"idle disabled target requests stay native without hiding the reticle");
        Check(!CoordinateRoute(true,entry.id,Meteor,skills) && !ReleaseForSkill(Meteor,skills),"mismatched query preserves active supported aim");
    }
    std::array<unsigned,8> classCounts{};
    const std::array<const char*,8> classNames{"amazon","sorceress","necromancer","paladin","barbarian","druid","assassin","warlock"};
    std::string allSkills;
    for(std::size_t c=0;c<classNames.size();++c) {
        allSkills+="[aim."+std::string(classNames[c])+"]\n";
        for(const auto& catalog:SkillCatalog) if(std::string_view(catalog.className)==classNames[c]) {
            ++classCounts[c];
            allSkills+='"'+std::to_string(catalog.id)+"\"=false\n";
        }
        Check(classCounts[c]==30,"each of eight classes has 30 catalog skills");
    }
    Check(ParseSkills(allSkills,skills),"full class catalog with all native defaults parses");
    for(const auto& catalog:SkillCatalog) Check(!skills.Enabled(catalog.id),"false catalog entry always preserves native behavior");
    Check(ParseSkills("[aim.sorceress]\n\"48\"=false\n[aim.necromancer]\n\"74\"=false\n\"92\"=false",skills) && !skills.Enabled(48) && !skills.Enabled(74) && !skills.Enabled(92),"Nova, Corpse Explosion and Poison Nova default native");
    Check(ParseSkills("[aim.sorceress]\n\"36\"=true\n[aim.necromancer]\n\"78\"=true\n[aim.warlock]\n\"373\"=false",skills) && skills.Snaps(36) && skills.Mode(78)==TargetMode::Ground && !skills.Enabled(373),"additional catalog toggles use provisional targeting modes");
    Check(!ParseSkills("[aim.warlock]\n\"373\"=false\n[aim.amazon]\n\"373\"=true",skills),"full catalog duplicates across classes rejected");
    Check(ParseSkills("[aim.amazon]\n# Guided Arrow\n\"22\"=true\n# Multi Shot\n\"12\"=false",skills) && skills.Snaps(GuidedArrow) && !skills.Enabled(MultiShot),"numeric class keys with name comments control built-in skills");
    Check(ParseSkills("[aim.barbarian]\n\"22\"=false",skills) && !skills.Enabled(GuidedArrow),"numeric IDs are not restricted by heading class");
    Check(!ParseSkills("[aim.amazon]\n\"22\"=true\n\"Guided Arrow\"=false",skills),"numeric and legacy keys cannot duplicate the same skill");
    Check(!ParseSkills("[aim.amazon]\n\"357\"=true",skills),"unlisted ID requires explicit custom targeting mode");
    Check(ParseQolSettings("[aim.amazon]\n\"Guided Arrow\"=true\n\"Multi Shot\"=false\n[aim.custom]\n\"357\"=\"ground\"\n\"358\"=\"snap\"",integrated,enabled,&skills),"class and numeric custom settings parse");
    Check(skills.Enabled(GuidedArrow) && !skills.Enabled(MultiShot) && skills.Enabled(Meteor),"per-skill disables preserve other defaults");
    Check(skills.Enabled(357) && !skills.Snaps(357) && skills.Snaps(358) && !skills.Enabled(359),"custom modes require explicit numeric opt-in");
    Check(ParseSkills("[aim.barbarian]\n\"Guided Arrow\"=false",skills) && !skills.Enabled(GuidedArrow),"headings do not restrict character or skill class");
    Check(!ParseSkills("[aim.amazon]\n\"Guided Arrow\"=true\n[aim.barbarian]\n\"Guided Arrow\"=false",skills),"duplicate skill identity across headings rejected");
    Check(!ParseSkills("[aim.custom]\n\"357\"=\"snap\"\n\"0357\"=\"ground\"",skills),"duplicate numeric identity rejected");
    Check(!ParseSkills("[aim.custom]\n\"22\"=\"ground\"",skills) && !ParseSkills("[aim.custom]\n\"65535\"=\"snap\"",skills),"custom IDs cannot override named skills or exceed coordinate contract");
    Check(!ParseSkills("[aim.custom]\n\"New Spell\"=\"snap\"",skills) && !ParseSkills("[aim.custom]\n\"357\"=\"automatic\"",skills),"unknown name or mode cannot authorize a skill");
    Check(!ParseSkills("[aim.amazom]\n\"Multi Shot\"=false",skills) && !ParseSkills("[aim.amazon]\n\"Multishot\"=false",skills),"misspelled section or skill is not silently ignored");
    skills={};
    Check(!CoordinateRoute(false,Meteor,Meteor,skills) && CoordinateRoute(true,Meteor,Meteor,skills),"supported casts remain native until stick activation");
    Check(!CoordinateRoute(true,143,143,skills) && !CoordinateRoute(true,Meteor,Teleport,skills),"unsupported and mismatched casts remain native");
    Check(!OverrideScoring(true,-1,skills) && !OverrideScoring(false,Meteor,skills) && !OverrideScoring(true,Teleport,skills) && OverrideScoring(true,Meteor,skills),"idle, unarmed and ground-only enumeration never changes native scoring");
    Check(!DeliberateStick({0.1f,0.1f},0.22f) && DeliberateStick({0.23f,0},0.22f),"activation uses radial deadzone");
    std::string extensions="[aim.custom]\n";
    for(int id=300;id<332;++id) extensions+='"'+std::to_string(id)+"\"=\"snap\"\n";
    Check(ParseSkills(allSkills+extensions,skills) && skills.Snaps(331),"full class catalog plus 32 custom entries fit bounded storage");
    extensions+="\"332\"=\"ground\"\n";
    Check(!ParseSkills(allSkills+extensions,skills) && skills.Snaps(331) && !skills.Enabled(332),"overflow is rejected without partial settings assignment");
    Check(ParseSkills("[aim.custom]\n\"357\"=\"disabled\"",skills) && !skills.Enabled(357),"explicit disabled custom skill keeps native targeting");
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
