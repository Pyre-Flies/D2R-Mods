#include "shared_page_policy.h"
#include "glyph_policy.h"
#include <cstdio>
#include <cstdlib>
unsigned checks{};
void Check(bool ok,const char* why) { ++checks; if(!ok) {std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);} }
int main() {
    Check(Probe::FilterLootTrigger(200,true,true)==200,"Shared chord survives existing raw trigger filter");
    Check(Probe::FilterLootTrigger(200,true,false)==0,"ground loot still suppresses trigger");
    Check(Probe::FilterLootTrigger(200,false,false)==200,"unmodified trigger preserved");
    Check(Probe::FilterLootTrigger(20,true,false)==20,"below threshold unchanged");
    Probe::SharedPageGesture g;
    Check(g.Handle(true,true,true,7,true,false)==19,"LB LT maps previous");
    Check(g.Handle(true,true,true,7,true,true)==-1,"held repeat consumed");
    Check(g.Handle(true,true,true,7,true,false)==-1,"duplicate begin consumed");
    Check(g.Handle(true,true,false,7,true,true)==-1,"LB released first retains trigger ownership");
    Check(g.Handle(true,true,false,7,false,false)==-1,"owned release consumed");
    Check(g.Handle(true,true,true,7,true,false)==19,"release rearms");
    g={};
    Check(g.Handle(true,true,true,8,true,false)==20,"LB RT maps next");
    Check(g.Handle(true,false,false,8,true,true)==-1,"owned gesture cannot leak after tab change");
    g={};
    Check(g.Handle(true,true,false,7,true,false)==0,"bare LT main tab unchanged");
    Check(g.Handle(true,true,true,7,true,true)==0,"LB added after held trigger does not turn page");
    g={};
    Check(g.Handle(true,false,true,8,true,false)==0,"outside Shared unchanged");
    Check(g.Handle(true,true,true,19,true,false)==-1,"bare LB no page turn");
    Check(g.Handle(true,true,false,20,true,false)==-1,"bare RB no page turn");
    Check(g.Handle(true,false,true,19,true,false)==0,"bumpers elsewhere preserved");
    Check(g.Handle(true,true,true,0,true,false)==0,"activation preserved");
    Check(g.Handle(true,true,true,2,true,false)==0,"face action preserved");
    Check(g.Handle(false,true,true,7,true,false)==0,"disabled passes through");
    g={};
    Check(g.Handle(true,true,true,7,true,true)==0,"initial repeat cannot fabricate a press");
    using namespace QolGlyphPolicy;
    const std::string_view left[]={"Text","StashLeftArrow","StashNavigation","SharedStashTabContainer","BankExpansionLayout"};
    const std::string_view right[]={"StashRightArrow","BankExpansionLayout"};
    Check(Classify(left,5,false,false,true)==Hint::SharedLeft,"left Shared legend scope");
    Check(Classify(right,2,false,false,true)==Hint::SharedRight,"right Shared legend scope");
    Check(Classify(left,5,false,false,false)==Hint::None,"failed hook retains native glyph");
    Check(Classify(right,1,false,false,true)==Hint::None,"same name outside bank unchanged");
    Check(std::string_view(Text(Hint::SharedLeft))=="\xEE\x80\xA7+\xEE\x80\x9B","LB LT native action glyphs");
    Check(std::string_view(Text(Hint::SharedRight))=="\xEE\x80\xA7+\xEE\x80\x9C","LB RT native action glyphs");
    const std::string_view nested[]={"Text","CycleLeftIndicator","StashRightArrow","BankExpansionLayout"};
    Check(Classify(nested,4,true,false,true)==Hint::SharedRight,"Shared owner wins over generic child indicator");
    DrawRect input{100,20,64,64},output{};
    Check(SharedChordRect(Hint::SharedLeft,input,output) && output.x==36 && output.width==192 && output.x+output.width/2==input.x+input.width/2,"left chord centered on original slot");
    Check(SharedChordRect(Hint::SharedRight,input,output) && output.x==36 && output.width==192 && output.x+output.width/2==input.x+input.width/2,"right chord centered on original slot");
    Check(input.x==100 && input.width==64,"native rectangle unchanged");
    Check(!SharedChordRect(Hint::MainLeft,input,output) && !SharedChordRect(Hint::SharedLeft,{0,0,-1,64},output),"unrelated and invalid rectangles excluded");
    std::printf("Shared page routing/glyph: %u checks passed.\n",checks);
}
