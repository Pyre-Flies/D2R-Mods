#include "settings.h"
#include <cstdio>
#include <cstdlib>
#include <string>
void Check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
bool Same(const InventoryDeposit::Settings& a,const InventoryDeposit::Settings& b){
    return a.enabled==b.enabled && a.x==b.x && a.y==b.y &&
        a.anchorToGold==b.anchorToGold && a.goldGap==b.goldGap && a.goldOffsetY==b.goldOffsetY;
}
int main(){
    using namespace InventoryDeposit;Settings s{};
    Check(ParseSettings("",s) && s.enabled && s.x==860 && s.y==1620 &&
        s.anchorToGold && s.goldGap==85 && s.goldOffsetY==1,"missing config uses gold anchor and manual placement defaults");
    Check(ParseSettings("[other]\nenabled=false\n[inventory_deposit]\n# enabled=true\nenabled = false # off\nbutton_x=970\nbutton_y = 1600\n",s) && !s.enabled && s.x==970 && s.y==1600,"exact section, comments and explicit disable");
    Check(s.anchorToGold && s.goldGap==85 && s.goldOffsetY==1,"legacy coordinates do not become anchor offsets");
    Check(ParseSettings("[inventory_deposit]\nanchor_to_gold = false\nbutton_x=800\nbutton_y=1644\ngold_gap=140\ngold_offset_y=-24\n",s) &&
        !s.anchorToGold && s.x==800 && s.y==1644 && s.goldGap==140 && s.goldOffsetY==-24,"manual mode preserves exact legacy coordinates independently of anchor offsets");
    Check(ParseSettings("[other]\nanchor_to_gold=false\ngold_gap=9000\n[inventory_deposit]\nanchor_to_gold=true # use live gold\ngold_gap=0\ngold_offset_y=4096\nbutton_x=0\nbutton_y=4096\n",s) &&
        s.anchorToGold && s.goldGap==0 && s.goldOffsetY==4096 && s.x==0 && s.y==4096,"anchor keys are section scoped and accept valid upper and lower boundaries");
    Check(ParseSettings("[inventory_deposit]\ngold_gap=4096\ngold_offset_y=-4096\n",s) &&
        s.goldGap==4096 && s.goldOffsetY==-4096,"anchor offset lower bound and gap upper bound parse");
    for(const auto text:{"enabled=trueish","button_x=-1","button_x=800junk","button_y=4097","button_x=1\nbutton_x=2","enabled=false\n[inventory_deposit]\nenabled=true",
        "anchor_to_gold=1","anchor_to_gold=True","anchor_to_gold=trueish","anchor_to_gold=","anchor_to_gold=true\nanchor_to_gold=false",
        "gold_gap=-1","gold_gap=4097","gold_gap=122junk","gold_gap=2147483648","gold_gap=0\ngold_gap=1",
        "gold_offset_y=-4097","gold_offset_y=4097","gold_offset_y=-2147483649","gold_offset_y=-24junk","gold_offset_y=0\ngold_offset_y=-1",
        "enabled=false\nanchor_to_gold=false\nbutton_x=5\ngold_gap=24\ngold_offset_y=-4097"}) {
        const auto before=s;std::string input="[inventory_deposit]\n";input+=text;
        Check(!ParseSettings(input,s) && Same(s,before),"invalid action settings fail without partial mutation");
    }
    Check(ParseSettings("",s) && Same(s,Settings{}),"a subsequent parse resets all fields to defaults");
    std::puts("Standalone configuration scope, explicit disable, manual coordinates, gold anchoring and invalid settings passed.");
}
