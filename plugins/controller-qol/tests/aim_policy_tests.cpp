#include "aim_policy.h"
#include "aim_config.h"
#include "snap_policy.h"
#include "coexistence_policy.h"
#include "cast_observer_policy.h"
#include "skill_toggle.h"
#include "tree_layout.h"
#include "general_skill_profile.h"
#include <map>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <limits>
void Check(bool b,const char* message) {
    if (!b) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
int main() {
    using namespace Aim;
    MotionSettings logSettings{}; bool logEnabled{};
    Check(ParseQolSettings("[aim]\nenabled=true",logSettings,logEnabled) && !logSettings.verbose,"existing configs default to quiet aim logging");
    Check(ParseQolSettings("[aim]\nverbose=true\ndebug_overlay=false",logSettings,logEnabled) && logSettings.verbose && !logSettings.debugOverlay,"verbose file logging is independent of HUD");
    Check(ParseQolSettings("[aim]\nverbose=false\ndebug_overlay=true",logSettings,logEnabled) && !logSettings.verbose && logSettings.debugOverlay,"debug HUD does not enable file logging");
    Check(!ParseQolSettings("[aim]\nverbose=1",logSettings,logEnabled) && !ParseQolSettings("[aim]\nverbose=true\nverbose=false",logSettings,logEnabled),"verbose requires a single true/false value");
    for(bool nativeUnits:{false,true}) {
        Check(ReticleScoringCategory(1,nativeUnits),"ordinary monster keeps full-circle aim scoring");
        for(int category:{-1,0,4,5,6,7,8,9,10,999})
            Check(!ReticleScoringCategory(category,nativeUnits),"NPC and other native categories retain interaction scoring after snap casts");
        for(int category:{2,3}) Check(ReticleScoringCategory(category,nativeUnits)==nativeUnits,"only Telekinesis preparation scores objects and items");
    }
    TargetCategoryObservation observed{};
    Check(!observed.EnemyOnly(),"unclassified candidate cannot enter snap cache");
    observed.Observe(1); observed.Observe(1);
    Check(observed.EnemyOnly(),"repeated ordinary enemy scoring remains eligible");
    observed.Observe(5);
    Check(!observed.EnemyOnly(),"mixed native classifications cannot enter enemy cache");
    observed={}; observed.Observe(5); observed.Observe(1);
    Check(!observed.EnemyOnly(),"later enemy score cannot erase protected classification");
    SnapBook npcBook{}; npcBook.Observe(1,7,{110,100},100,true);
    npcBook.Observe(1,7,{110,100},101,observed.EnemyOnly());
    Check(!npcBook.Choose(1,{110,100},{100,100},101).valid,"noncombat classification removes previously retained snap identity");
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
    Check(RewriteSkillToggle("[aim.custom]\n\"900\"=\"snap\"",900,false,toggled),"explicit custom snap skill can be disabled");
    Check(!RewriteSkillToggle("[aim.custom]\n",900,true,toggled),"undeclared custom skill is never invented by R3");
    const std::string warp="[aim.custom]\r\n\"429\" = \"ground\" # Warp\r\n\"900\" = false\r\n[aim.leading]\r\n\"429\" = 0\r\n";
    SkillSettings warpSettings;
    Check(RewriteSkillToggle(warp,429,false,toggled) && ParseSkills(toggled,warpSettings) && !warpSettings.Enabled(429) && warpSettings.Targeting(429)==TargetMode::Ground,"legacy custom ground mode survives disable and a fresh parse");
    Check(toggled.find("false # Warp\r\n")!=toggled.npos && toggled.find("\"900\" = false\r\n")!=toggled.npos,"custom save preserves comments newlines and unrelated settings");
    Check(RewriteSkillToggle(toggled,429,true,toggled) && ParseSkills(toggled,warpSettings) && warpSettings.Mode(429)==TargetMode::Ground && warpSettings.LeadMillisecondsPerTile(429)==0,"custom Warp off/on across reload keeps ground routing and zero lead");
    Check(CoordinateRoute(true,429,429,warpSettings) && !CoordinateRoute(true,54,429,warpSettings) && !OverrideScoring(true,429,warpSettings),"Warp uses its own authorized ground cast without enemy snap");
    for(const std::string suffix:{"[aim.targeting]\n", "[aim.targeting]\n\"12\"=\"snap\"\n[aim]\nverbose=false"}) {
        Check(RewriteSkillToggle("[aim.custom]\n\"429\"=\"ground\"\n"+suffix,429,false,toggled) && ParseSkills(toggled,warpSettings) && warpSettings.Targeting(429)==TargetMode::Ground,"legacy mode migration uses existing targeting section including one at EOF");
    }
    const std::string explicitWarp="[aim.custom]\n\"429\"=false\n[aim.targeting]\n\"429\"=\"ground\" # retain\n";
    Check(RewriteSkillToggle(explicitWarp,429,true,toggled) && toggled==std::string(explicitWarp).replace(explicitWarp.find("false"),5,"true"),"declared Warp with explicit mode changes only the enable flag");
    Check(RewriteSkillToggle("[aim.custom]\n\"429\"=\"ground\"\n[aim.targeting]\n\"429\"=\"snap\"",429,false,toggled) && ParseSkills(toggled,warpSettings) && warpSettings.Targeting(429)==TargetMode::Snap,"explicit targeting takes priority over legacy ground spelling");
    Check(!RewriteSkillToggle("[aim.custom]\n\"429\"=\"disabled\"",429,true,toggled),"locked custom skills remain locked");
    LiveSkills customLive;
    Check(ParseSkills("[aim.custom]\n\"429\"=false\n\"900\"=\"disabled\"\n[aim.targeting]\n\"429\"=\"ground\"",customLive.baseline),"custom live settings parse");
    customLive.Publish(429,customLive.Targeting(429));customLive.Publish(900,TargetMode::Snap);customLive.Publish(901,TargetMode::Snap);
    Check(customLive.Mode(429)==TargetMode::Ground && !customLive.Enabled(900) && !customLive.Enabled(901) && !customLive.CanToggle(0),"live overrides respect declared custom identity and locked state");
    // Synthetic native widgets exercise the actual decoder at the captured
    // offsets; IDs move between buttons when equipment rearranges General Skills.
    constexpr std::uintptr_t base=0x140000000,button1=0x300000000,button2=0x300002000,record=0x300004000;
    std::map<std::uintptr_t,std::uintptr_t> memory{{button1,base+GeneralSkillNative::GeneralButton},{button2,base+GeneralSkillNative::GeneralButton},
        {button1+0xb88,0},{button1+0xc0c,static_cast<std::uintptr_t>(-1)},{button1+0xc08,429},
        {button2+0xb88,0},{button2+0xc0c,static_cast<std::uintptr_t>(-1)},{button2+0xc08,12}};
    auto pointer=[&](std::uintptr_t address) noexcept {const auto it=memory.find(address);return it==memory.end()?std::uintptr_t{}:it->second;};
    unsigned integerReads{};
    auto integer=[&](std::uintptr_t address) noexcept {++integerReads;return static_cast<int>(pointer(address));};
    auto readButton=[&](std::uintptr_t button) {return GeneralSkillNative::ReadId(button,base,true,pointer,integer);};
    Check(readButton(button1)==429 && readButton(button2)==12,"Warp and item-granted Multi Shot use actual General button IDs");
    std::swap(memory[button1+0xc08],memory[button2+0xc08]);
    Check(readButton(button1)==12 && readButton(button2)==429,"rearranged widgets cannot retain old skill identity");
    integerReads=0;
    Check(GeneralSkillNative::ReadId(button1,base,false,pointer,integer)==-1 && integerReads==0,"failed General guard prevents all extended field reads");
    for(int id:{-1,0,1,2,3,4,5,357,358,359,360,361,362,363,364,370,65535}) {
        memory[button1+0xc08]=static_cast<std::uintptr_t>(id);
        Check(readButton(button1)==-1,"template native utility and out-of-range IDs do not expose toggles");
    }
    memory[button1+0xc08]=429;memory[button1+0xb88]=1;
    Check(readButton(button1)==-1,"unreviewed General button kinds fail open");
    memory[button1+0xb88]=0;memory[button1+0xc0c]=123;
    Check(readButton(button1)==-1,"unreviewed charged/item-specific representation fails open");
    memory[button1]=base+GeneralSkillNative::ClassButton;memory[button1+0x668]=record;memory[record]=12;
    Check(GeneralSkillNative::ReadId(button1,base,false,pointer,integer)==12,"class skill reader remains available when General support is unavailable");
    memory[button1+0x668]=0;Check(readButton(button1)==-1,"class record null is rejected");
    memory[button1]=base+0x1234;Check(readButton(button1)==-1,"unknown widget type is rejected");
    auto slots=[](std::uintptr_t rva) noexcept {return rva==GeneralSkillNative::GeneralButton+0x20?std::uintptr_t{0x238c20}:std::uintptr_t{0x2382d0};};
    Check(GeneralSkillNative::Admit([](const auto&) noexcept {return true;},slots),"verified General witnesses and vtables admit decoder");
    for(const auto& failed:GeneralSkillNative::Guards)
        Check(!GeneralSkillNative::Admit([&](const auto& g) noexcept {return g.rva!=failed.rva;},slots),"each General field guard is required");
    Check(!GeneralSkillNative::Admit([](const auto&) noexcept {return true;},[](auto) noexcept {return 0u;}),"foreign vtable ownership refuses General support");
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
    SkillSettings special{};
    const std::string leadConfig="[aim.sorceress]\n\"47\"=false\n[aim.leading]\n\"47\"=50 # preserve\n\"251\"=100\n[aim.custom]\n\"900\"=true\n";
    Check(ParseSkills(leadConfig,special) && special.LeadMillisecondsPerTile(47)==50 && special.LeadMillisecondsPerTile(251)==100 && !special.Enabled(47),"lead estimates never enable skills");
    Check(RewriteSkillToggle(leadConfig,47,true,toggled) && toggled.find("\"47\"=50 # preserve")!=toggled.npos && ParseSkills(toggled,special) && special.Enabled(47) && special.LeadMillisecondsPerTile(47)==50,"R3 preserves leading entry when it follows the enable section");
    for(const char* bad:{"[aim.leading]\n\"47\"=-1", "[aim.leading]\n\"47\"=201", "[aim.leading]\n\"47\"=50.5", "[aim.leading]\n\"900\"=50", "[aim.leading]\n\"47\"=50\n\"047\"=60"})
        Check(!ParseSkills(bad,special),"invalid or undeclared lead configuration rejected");
    Check(ParseSkills("[aim.custom]\n\"900\"=true\n[aim.leading]\n\"900\"=75\n\"47\"=0",special) && special.LeadMillisecondsPerTile(900)==75 && special.LeadMillisecondsPerTile(47)==0,"custom declared skills and explicit zero lead supported");
    Check(ParseSkills("[aim.sorceress]\n\"43\"=true\n[aim.assassin]\n\"251\"=true",special),"special skill fixture parses");
    Check(!CoordinateRoute(true,43,43,special) && CoordinateRoute(true,251,251,special),"Telekinesis retains unit identity while Fire Blast uses snapped coordinates");
    for(unsigned type:{1u,2u,4u}) {
        Check(NativeUnitScoring(true,43,251,type,special) && NativeUnitScoring(true,-1,43,type,special),"Telekinesis scores enemies objects and items during cast and its own preview");
        Check(!NativeUnitScoring(false,43,43,type,special) && !NativeUnitScoring(true,251,43,type,special),"unarmed and other active skills cannot inherit Telekinesis scoring");
    }
    for(unsigned type:{0u,3u,5u,999u}) Check(!NativeUnitScoring(true,43,43,type,special),"other unit categories retain native geometry");
    for(unsigned type:{2u,4u}) {
        Check(NativeUnitScoring(true,-1,54,type,special) && NativeUnitScoring(true,-1,251,type,special),"idle after another enabled skill prepares the first Telekinesis object/item selection");
        Check(!NativeUnitScoring(true,54,43,type,special) && !NativeUnitScoring(true,357,43,type,special) && !NativeUnitScoring(true,-1,143,type,special),"active movement/interaction and disabled previews cannot inherit Telekinesis preparation");
    }
    Check(!NativeUnitScoring(true,-1,54,1,special),"ordinary monster preview remains owned by the existing snap pipeline");
    for(int id:{234,244,251,256,257,261,262,271,272,276,393,396,400}) {
        Check(special.Targeting(id)==TargetMode::Snap,"offensive ground placement defaults to enemy snapping");
        if(id!=251) Check(!special.Enabled(id),"new targeting defaults do not enable previously disabled skills");
    }
    Check(special.Targeting(54)==TargetMode::Ground && special.Targeting(132)==TargetMode::Ground && special.Targeting(78)==TargetMode::Ground && special.Targeting(75)==TargetMode::Ground,"movement wall and minion placement retain ground policy");
    Check(ParseSkills("[aim.sorceress]\n\"43\"=true\n[aim.assassin]\n\"251\"=true\n[aim.targeting]\n\"43\"=\"ground\"\n\"251\"=\"ground\"",special) && !NativeUnitScoring(true,43,43,2,special) && !CoordinateRoute(true,43,43,special) && special.Mode(251)==TargetMode::Ground,"explicit ground preferences are retained; Telekinesis never discards its native unit");
    Check(ParseSkills("[aim.sorceress]\n\"43\"=false",special) && !NativeUnitScoring(true,43,43,2,special),"disabled Telekinesis stays fully native");

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
    Check(OverrideScoring(true,-1,skills,Meteor) && OverrideScoring(true,-1,skills,Teleport),"armed enabled preview acquires around the shared cursor before the first cast");
    Check(!OverrideScoring(false,-1,skills,Meteor) && !OverrideScoring(true,-1,skills,143),"unarmed or disabled preview cannot replace native geometry");
    Check(!OverrideScoring(true,Teleport,skills,Meteor) && !OverrideScoring(true,143,skills,Meteor),"active ground and disabled skills cannot inherit preview geometry");
    Check(!CoordinateRoute(true,Meteor,-1,skills),"idle acquisition never authorizes a cast coordinate route");
    Check(!DeliberateStick({0.1f,0.1f},0.22f) && DeliberateStick({0.23f,0},0.22f),"activation uses radial deadzone");
    std::string extensions="[aim.custom]\n";
    constexpr int customEnd=1000+static_cast<int>(SkillSettings::Capacity-SkillCatalog.size());
    for(int id=1000;id<customEnd;++id) extensions+='"'+std::to_string(id)+"\"=\"snap\"\n";
    Check(ParseSkills(allSkills+extensions,skills) && skills.Snaps(customEnd-1),"full class catalog plus expanded custom capacity fit bounded storage");
    extensions+='"'+std::to_string(customEnd)+"\"=\"ground\"\n";
    Check(!ParseSkills(allSkills+extensions,skills) && skills.Snaps(customEnd-1) && !skills.Enabled(customEnd),"overflow is rejected without partial settings assignment");
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
    Check(settings.initialSpeed==8 && settings.maximumSpeed==48 && settings.accelerationSeconds==0.35f,"requested faster cursor defaults");
    // Keep the measured acceleration regression fixture independent of defaults.
    settings.initialSpeed=4; settings.maximumSpeed=28; settings.accelerationSeconds=0.65f;
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
    CursorMotion quantized{}; Point quantizedOffset{};
    for(int i=0;i<64;++i) {
        quantized.Advance({1,0},projection,false,1.0f/64,settings,quantizedOffset);
        const auto saved=quantizedOffset; const auto held=quantized.heldSeconds;
        Check(!quantized.Advance({1,0},projection,false,0,settings,quantizedOffset) &&
            DistanceSquared(saved,quantizedOffset)==0 && quantized.heldSeconds==held,
            "duplicate timestamp preserves acceleration without movement");
    }
    Check(std::abs(std::hypot(quantizedOffset.x,quantizedOffset.y)-20.2f)<0.002f,
        "quantized clock reaches same one-second travel");
    Check(!quantized.Advance({0,0},projection,false,0,settings,quantizedOffset) && quantized.heldSeconds==0,
        "release at duplicate timestamp still resets acceleration");
    quantized.Advance({1,0},projection,false,0.05f,settings,quantizedOffset);
    Check(!quantized.Advance({-1,0},projection,false,0,settings,quantizedOffset) && quantized.heldSeconds==0,
        "direction reversal at duplicate timestamp restarts fine control");

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
    MotionTrack moving{};
    const std::string iceLimits="[aim.sorceress]\n\"39\"=false\n[aim.leading]\n\"39\"=100\n[aim.leading_max_ms]\n\"39\"=1200\n[aim.leading_max_tiles]\n\"39\"=6\n";
    Check(ParseSkills(iceLimits,special) && !special.Enabled(39) && special.LeadMaxMilliseconds(39)==1200 && special.LeadMaxTiles(39)==6 && special.LeadMaxMilliseconds(36)==600 && special.LeadMaxTiles(36)==3,"ice limits do not enable skill or change other skills");
    Check(RewriteSkillToggle(iceLimits,39,true,toggled) && ParseSkills(toggled,special) && special.Enabled(39) && special.LeadMaxMilliseconds(39)==1200 && special.LeadMaxTiles(39)==6,"R3 preserves both leading limit sections");
    for(const char* bad : {"[aim.leading_max_ms]\n\"39\"=99", "[aim.leading_max_ms]\n\"39\"=1501", "[aim.leading_max_tiles]\n\"39\"=0", "[aim.leading_max_tiles]\n\"39\"=9", "[aim.leading_max_tiles]\n\"39\"=1.5", "[aim.leading_max_ms]\n\"900\"=1000", "[aim.leading_max_tiles]\n\"39\"=6\n\"039\"=4"})
        Check(!ParseSkills(bad,special),"leading limits reject invalid bounds, undeclared IDs and duplicates");
    moving.Observe({110,100},1000);
    moving.Observe({110.2f,100},1050);
    Check(moving.Destination({108,100},{110.2f,100},1050,50).x==110.2f,"one velocity window cannot lead");
    moving.Observe({110.4f,100},1100);
    const auto fastLead=moving.Destination({108,100},{110.4f,100},1100,50);
    const auto slowLead=moving.Destination({108,100},{110.4f,100},1100,100);
    Check(std::abs((fastLead.x-110.4f)-0.48f)<0.002f && std::abs((slowLead.x-110.4f)-0.96f)<0.003f,"lead scales with distance and per-skill travel estimate");
    moving.Observe({110.4f,100},1100);
    Check(moving.consistent==2 && Finite(moving.velocity),"same timestamp does not divide by zero or clear stable motion");
    Check(moving.Destination({108,100},{110.4f,100},1200,50).x==110.4f && moving.Destination({108,100},{110.4f,100},1100,0).x==110.4f,"stale and disabled lead fall back to current position");
    moving.Observe({110.2f,100},1150);
    Check(moving.Destination({108,100},{110.2f,100},1150,100).x==110.2f,"direction reversal immediately drops prediction until movement agrees");
    moving.Observe({120,100},1200);
    Check(moving.consistent==0,"teleport-like position jump invalidates motion estimate");
    moving.Observe({120,100},1250);
    Check(moving.velocity.x==0 && moving.consistent==0,"stationary target needs no lead");
    moving.Reset({110,100},2000);moving.Observe({111,100},2050);moving.Observe({112,100},2100);
    Check(std::abs(moving.Destination({100,100},{112,100},2100,200).x-115)<0.001f,"lead displacement capped at three tiles");
    Check(std::abs(moving.Destination({100,100},{112,100},2100,100,1200,6).x-118)<0.001f,"ice override permits six tiles without changing default cap");
    Check(moving.Destination({100,100},{112,100},2100,100,1600,6).x==112 && moving.Destination({100,100},{112,100},2100,100,1200,9).x==112,"invalid runtime limits fail open to current position");
    Check(moving.Destination({83,100},{112,100},2100,100).x==112,"prediction cannot exceed outer aim range");
    Check(moving.Destination({83,100},{112,100},2100,100,1200,6).x==112,"expanded ice lead still respects outer aim range");
    MotionTrack slow{};
    slow.Observe({111,100},3000); slow.Observe({111.125f,100},3125); slow.Observe({111.25f,100},3250);
    Check(std::abs(slow.Destination({100,100},{111.25f,100},3250,100).x-111.85f)<0.001f && std::abs(slow.Destination({100,100},{111.25f,100},3250,100,1200,6).x-112.375f)<0.001f,"ice time horizon grows only when overridden");
    MotionTrack chilled{};
    chilled.Observe({110,100},4000); chilled.Observe({110.5f,100},4100); chilled.Observe({111,100},4200);
    chilled.Observe({111.25f,100},4300);
    Check(chilled.consistent==2 && chilled.velocity.x<5 && chilled.velocity.x>2.5f,"halved movement reduces lead estimate without a cold-state guess");
    chilled.Observe({111.3125f,100},4400);
    Check(chilled.consistent==1,"large slowdown drops confidence instead of keeping old fast lead");
    chilled.Observe({111.375f,100},4500);
    Check(chilled.consistent==2 && std::abs(chilled.velocity.x-0.625f)<0.001f,"slowed movement establishes its own measured velocity");
    chilled.Observe({111.375f,100},4600);
    Check(chilled.consistent==0 && chilled.Destination({100,100},{111.375f,100},4600,100,1200,6).x==111.375f,"frozen target receives no leading");
    SnapBook leadBook{};
    for(unsigned t:{1000u,1050u,1100u}) leadBook.Observe(1,99,{110+(t-1000)*0.004f,100},t,true);
    Check(leadBook.Choose(1,{110,100},{100,100},1100).motion.consistent==2,"native copied observations carry stable movement to selection");
    leadBook.Observe(1,99,{110.4f,100},1110,false);
    leadBook.Observe(1,99,{110.4f,100},1120,true);
    Check(leadBook.Choose(1,{110,100},{100,100},1120).motion.consistent==0,"rejected target cannot reuse previous velocity on reacquisition");
    leadBook.Observe(2,99,{110.4f,100},1130,true);
    Check(leadBook.Choose(2,{110,100},{100,100},1130).motion.consistent==0,"unit IDs cannot transfer velocity across player identity");
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
    DisplayPoint smoothing{};
    smoothing.Update({100,100},1000,60);
    float filteredError=0,rawError=0;
    for(unsigned i=1;i<=40;++i) {
        const Point raw{100+(i%2?3.0f:-3.0f),100};
        const auto filtered=smoothing.Update(raw,1000+i*16,60);
        rawError+=std::abs(raw.x-100);filteredError+=std::abs(filtered.x-100);
    }
    Check(filteredError<rawError*0.3f,"walking/camera screen-position jitter is reduced at 60 ms");
    DisplayPoint coherent{};Point walkingScreen{};
    auto walkingProjection=projection;
    Check(walkingProjection.Project(center,walkingScreen),"walking fixture projects");
    const auto stationaryScreen=walkingScreen;
    coherent.Update(walkingScreen,1000,60);
    for(unsigned i=1;i<=20;++i) {
        walkingProjection.worldOrigin.x+=1;walkingProjection.worldX.x+=1;walkingProjection.worldY.x+=1;
        Check(walkingProjection.Project({center.x+static_cast<float>(i),center.y},walkingScreen),"moving player/camera project together");
        const auto filtered=coherent.Update(walkingScreen,1000+i*16,60);
        Check(DistanceSquared(filtered,stationaryScreen)<0.001f,"coherent player/camera movement introduces no artificial display drift");
    }
    DisplayPoint direct{};direct.Update({100,100},1000,60);
    Check(direct.Update({125,100},1016,60,{25,0}).x==125,"right-stick offset change bypasses display smoothing");
    Check(direct.Update({125,100},1032,60).x==125,"released stick keeps its immediate display position");
    Check(direct.Update({500,100},1048,60).x==500,"large screen jump snaps without sweeping reticle");
    Check(direct.Update({505,100},1500,60).x==505,"stale display history resets");
    Check(direct.Update({510,100},1516,0).x==510,"zero smoothing uses raw screen position");
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
