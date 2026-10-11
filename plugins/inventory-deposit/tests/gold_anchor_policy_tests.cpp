#include "gold_anchor_policy.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <limits>

namespace {
using Rect=D2RL::Widgets::Rect;
void Check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}
}
bool Same(const Rect& a,const Rect& b){
    return a.x==b.x && a.y==b.y && a.width==b.width && a.height==b.height;
}
void Refused(const Rect& gold,const Rect& button,const Rect& parent,
    int gap,int offsetY,const char* why){
    Rect output{11,22,33,44};const auto before=output;
    Check(!InventoryDepositGoldAnchor::Compute(gold,button,parent,gap,offsetY,output) &&
        Same(output,before),why);
}
}

int main(){
    using InventoryDepositGoldAnchor::Compute;
    const Rect parent{0,0,1162,1798},gold{492,1656,246,48},button{860,1620,120,120};
    Rect output{};
    Check(Compute(gold,button,parent,122,0,output) && Same(output,{860,1620,120,120}),
        "live gold geometry centers the reference button beside the gold amount");
    Check(Compute(gold,{860,1620,144,136},parent,85,1,output) && Same(output,{823,1613,144,136}) &&
        output.y+output.height==1749,
        "top clearance moves the upper edge down while preserving the previous bottom and horizontal alignment");
    Check(Compute({522,1626,300,64},button,parent,20,8,output) && Same(output,{842,1606,120,120}),
        "shifted and resized gold changes horizontal and vertical anchor position");
    Check(Compute(gold,{0,0,80,40},parent,0,-16,output) && Same(output,{738,1644,80,40}),
        "button size and signed vertical offset participate in the anchor");
    Check(Compute(gold,button,{900,-200,1162,1798},122,0,output) && Same(output,{860,1620,120,120}),
        "parent origin is ignored because child positions are already parent local");
    Check(Compute(gold,{-900,-900,120,120},parent,122,0,output) && Same(output,{860,1620,120,120}),
        "previous button position does not affect resolved anchor");
    Check(Compute({50,50,100,50},{0,0,100,100},{0,0,250,125},0,0,output) &&
        Same(output,{150,25,100,100}),"target may touch the right and bottom panel boundaries");
    Check(Compute({100,5000,100,100},{0,0,50,50},{0,0,10000,10000},4096,-4096,output) &&
        Same(output,{4296,929,50,50}),"maximum gap and minimum vertical offset remain valid when geometry fits");
    Check(Compute({100,100,100,100},{0,0,50,50},{0,0,10000,10000},0,4096,output) &&
        Same(output,{200,4221,50,50}),"maximum vertical offset remains valid when geometry fits");

    Refused(gold,button,{0,0,900,1798},122,0,"narrower panel rejects target outside right edge");
    Refused(gold,button,{0,0,1162,1720},122,0,"shorter panel rejects target outside bottom edge");
    Refused({-1,10,10,10},button,parent,0,0,"negative gold x is malformed");
    Refused({10,-1,10,10},button,parent,0,0,"negative gold y is malformed");
    Refused({1100,10,100,10},button,parent,0,0,"gold itself must fit horizontally");
    Refused({10,1790,10,10},button,parent,0,0,"gold itself must fit vertically");
    Refused({10,10,20,20},button,parent,0,0,"centering cannot place the target above the panel");
    for(const int dimension:{0,-1}){
        Refused({492,1656,dimension,48},button,parent,122,0,"nonpositive gold width is refused");
        Refused({492,1656,246,dimension},button,parent,122,0,"nonpositive gold height is refused");
        Refused(gold,{0,0,dimension,120},parent,122,0,"nonpositive button width is refused");
        Refused(gold,{0,0,120,dimension},parent,122,0,"nonpositive button height is refused");
        Refused(gold,button,{0,0,dimension,1798},122,0,"nonpositive parent width is refused");
        Refused(gold,button,{0,0,1162,dimension},122,0,"nonpositive parent height is refused");
    }
    for(const int gap:{-1,4097})Refused(gold,button,parent,gap,0,"out of range gap is refused");
    for(const int dy:{-4097,4097})Refused(gold,button,parent,122,dy,"out of range vertical offset is refused");

    constexpr int maximum=(std::numeric_limits<int32_t>::max)();
    const Rect hugeParent{0,0,maximum,maximum};
    Refused({maximum-10,10,20,20},{0,0,1,1},hugeParent,0,0,"overflowing gold right edge is refused");
    Refused({10,maximum-10,20,20},{0,0,1,1},hugeParent,0,0,"overflowing gold bottom edge is refused");
    Refused({maximum-10,10,10,20},{0,0,1,1},hugeParent,4096,0,"overflowing target x is refused");
    Refused({10,maximum-20,20,20},{0,0,1,1},hugeParent,0,4096,"overflowing target y is refused");
    Refused({maximum-20,10,10,20},{0,0,20,1},hugeParent,0,0,"overflowing target right edge is refused");
    Refused({10,maximum-20,20,20},{0,0,1,40},hugeParent,0,0,"overflowing target bottom edge is refused");
    std::puts("Gold anchor geometry, layout variation, configured bounds, overflow and output atomicity passed.");
}
