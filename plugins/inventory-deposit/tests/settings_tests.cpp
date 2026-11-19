#include "settings.h"
#include "shared/chat_config.h"
#include <cstdio>
#include <cstdlib>
#include <string>
void Check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
bool Same(const InventoryDeposit::Settings& a,const InventoryDeposit::Settings& b){
    return a.enabled==b.enabled && a.depositRejuvenationPotions==b.depositRejuvenationPotions &&
        a.x==b.x && a.y==b.y &&
        a.anchorToGold==b.anchorToGold && a.goldGap==b.goldGap && a.goldOffsetY==b.goldOffsetY;
}
int main(){
    using namespace InventoryDeposit;Settings s{};
    Check(ParseSettings("",s) && s.enabled && s.depositRejuvenationPotions && s.x==860 && s.y==1620 &&
        s.anchorToGold && s.goldGap==85 && s.goldOffsetY==1,"missing config uses gold anchor, rejuvenation deposit, and manual placement defaults");
    Check(ParseSettings("[other]\nenabled=false\ndeposit_rejuvenation_potions=false\n[inventory_deposit]\n# enabled=true\nenabled = false # off\ndeposit_rejuvenation_potions = false\nbutton_x=970\nbutton_y = 1600\n",s) &&
        !s.enabled && !s.depositRejuvenationPotions && s.x==970 && s.y==1600,"exact section, comments and explicit disable");
    Check(s.anchorToGold && s.goldGap==85 && s.goldOffsetY==1,"legacy coordinates do not become anchor offsets");
    Check(ParseSettings("[inventory_deposit]\nanchor_to_gold = false\ndeposit_rejuvenation_potions = true\nbutton_x=800\nbutton_y=1644\ngold_gap=140\ngold_offset_y=-24\n",s) &&
        !s.anchorToGold && s.depositRejuvenationPotions && s.x==800 && s.y==1644 && s.goldGap==140 && s.goldOffsetY==-24,"manual mode preserves exact legacy coordinates independently of anchor offsets");
    Check(ParseSettings("[other]\nanchor_to_gold=false\ngold_gap=9000\n[inventory_deposit]\nanchor_to_gold=true # use live gold\ngold_gap=0\ngold_offset_y=4096\nbutton_x=0\nbutton_y=4096\n",s) &&
        s.anchorToGold && s.depositRejuvenationPotions && s.goldGap==0 && s.goldOffsetY==4096 && s.x==0 && s.y==4096,"anchor keys are section scoped and accept valid upper and lower boundaries");
    Check(ParseSettings("[inventory_deposit]\ngold_gap=4096\ngold_offset_y=-4096\n",s) &&
        s.goldGap==4096 && s.goldOffsetY==-4096,"anchor offset lower bound and gap upper bound parse");
    for(const auto text:{"enabled=trueish","button_x=-1","button_x=800junk","button_y=4097","button_x=1\nbutton_x=2","enabled=false\n[inventory_deposit]\nenabled=true",
        "deposit_rejuvenation_potions=1","deposit_rejuvenation_potions=True","deposit_rejuvenation_potions=falseish","deposit_rejuvenation_potions=","deposit_rejuvenation_potions=true\ndeposit_rejuvenation_potions=false",
        "anchor_to_gold=1","anchor_to_gold=True","anchor_to_gold=trueish","anchor_to_gold=","anchor_to_gold=true\nanchor_to_gold=false",
        "gold_gap=-1","gold_gap=4097","gold_gap=122junk","gold_gap=2147483648","gold_gap=0\ngold_gap=1",
        "gold_offset_y=-4097","gold_offset_y=4097","gold_offset_y=-2147483649","gold_offset_y=-24junk","gold_offset_y=0\ngold_offset_y=-1",
        "enabled=false\nanchor_to_gold=false\nbutton_x=5\ngold_gap=24\ngold_offset_y=-4097"}) {
        const auto before=s;std::string input="[inventory_deposit]\n";input+=text;
        Check(!ParseSettings(input,s) && Same(s,before),"invalid action settings fail without partial mutation");
    }
    Check(ParseSettings("",s) && Same(s,Settings{}),"a subsequent parse resets all fields to defaults");

    // Rejuvenation potion item code checks (space-padded and NUL-padded 'rvs'/'rvl').
    Check(IsRejuvenationPotionCode(ThreeCharCode('r','v','s') | 0x20000000u),"space-padded rvs is a rejuvenation potion");
    Check(IsRejuvenationPotionCode(ThreeCharCode('r','v','l') | 0x20000000u),"space-padded rvl is a full rejuvenation potion");
    Check(IsRejuvenationPotionCode(ThreeCharCode('r','v','s')),"NUL-padded rvs is a rejuvenation potion");
    Check(IsRejuvenationPotionCode(ThreeCharCode('r','v','l')),"NUL-padded rvl is a full rejuvenation potion");
    Check(!IsRejuvenationPotionCode(ThreeCharCode('h','p','5') | 0x20000000u),"healing potion is not a rejuvenation potion");
    Check(!IsRejuvenationPotionCode(ThreeCharCode('m','p','5') | 0x20000000u),"mana potion is not a rejuvenation potion");
    Check(!IsRejuvenationPotionCode(ThreeCharCode('g','c','v') | 0x20000000u),"gem is not a rejuvenation potion");
    Check(!IsRejuvenationPotionCode(ThreeCharCode('r','v','s') | 0x61000000u),"4-character code starting with rvs is not a rejuvenation potion");
    Check(!IsRejuvenationPotionCode(0),"zero code is not a rejuvenation potion");

    // /autodeposit command grammar checks.
    Check(ParseCommand("")==CommandAction::Status,"empty args report status");
    Check(ParseCommand("   ")==CommandAction::Status,"whitespace args report status");
    Check(ParseCommand("status")==CommandAction::Status,"status command");
    Check(ParseCommand("STATUS")==CommandAction::Status,"case-insensitive status command");
    Check(ParseCommand("help")==CommandAction::Help && ParseCommand("?")==CommandAction::Help,"help commands");
    Check(ParseCommand("rejuv")==CommandAction::RejuvToggle,"bare rejuv toggles");
    Check(ParseCommand("rejuv toggle")==CommandAction::RejuvToggle,"explicit rejuv toggle");
    Check(ParseCommand("rejuv on")==CommandAction::RejuvOn,"rejuv on enables");
    Check(ParseCommand("rejuv off")==CommandAction::RejuvOff,"rejuv off disables");
    Check(ParseCommand("rejuv status")==CommandAction::Status,"rejuv status");
    Check(ParseCommand("rejuv help")==CommandAction::Help,"rejuv help");
    Check(ParseCommand("REJUVS OFF")==CommandAction::RejuvOff && ParseCommand("rejuvenation on")==CommandAction::RejuvOn,"aliases and case-insensitivity");
    Check(ParseCommand("/autodeposit rejuv off")==CommandAction::RejuvOff && ParseCommand("autodeposit status")==CommandAction::Status,"optional command prefix");
    for(const auto bad:{"on","off","toggle","unknown","rejuv maybe","rejuv on extra","status extra","help extra"}) {
        Check(ParseCommand(bad)==CommandAction::Invalid,"invalid command syntax rejected");
    }

    // ChatConfig::UpdateBoolean round-trip with ParseSettings.
    const auto updatedOff=ChatConfig::UpdateBoolean("[inventory_deposit]\nenabled = true\ndeposit_rejuvenation_potions = true\ngold_gap = 85\n",
        "inventory_deposit",{"deposit_rejuvenation_potions",false});
    Check(updatedOff.has_value() && ParseSettings(*updatedOff,s) && s.enabled && !s.depositRejuvenationPotions && s.goldGap==85,
        "ChatConfig::UpdateBoolean updates deposit_rejuvenation_potions in place");
    const auto insertedOff=ChatConfig::UpdateBoolean("[inventory_deposit]\nenabled = true\ngold_gap = 90\n",
        "inventory_deposit",{"deposit_rejuvenation_potions",false});
    Check(insertedOff.has_value() && ParseSettings(*insertedOff,s) && s.enabled && !s.depositRejuvenationPotions && s.goldGap==90,
        "ChatConfig::UpdateBoolean inserts missing deposit_rejuvenation_potions into existing section");

    std::puts("Standalone configuration scope, explicit disable, rejuvenation potion controls, manual coordinates, gold anchoring and invalid settings passed.");
}
