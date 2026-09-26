#include "inline_format.h"
#include <cstdlib>
#include <string>
#include <climits>
namespace {
unsigned char row[0x144]{};
bool grouped{}, emit=true, returnRow=true;
int actualSeen{}, layerSeen{}, normalizeCalls{};
bool formatSuccess=true;
std::string text;
void check(bool ok) { if (!ok) { std::fputs("Inline formatting failure\n",stderr); std::exit(1); } }
unsigned char __fastcall Bank(void*) { return 2; }
const unsigned char* __fastcall Row(unsigned char bank,int stat) { check(bank==2 && stat==31); return returnRow?row:nullptr; }
int __fastcall Normalize(void*,int stat,int value,const void* record,InlineRanges::OptionalValue* opt,bool bound) {
    check(stat==31 && record==row && !opt->present); ++normalizeCalls;
    return bound ? value : value/256; // Realized values may be fixed-point; definitions aren't.
}
int __fastcall Group(void*,void*,int stat,int,const void* record,int* shouldEmit) {
    check(stat==31 && record==row); *shouldEmit=emit?1:0; return grouped?1:0;
}
bool __fastcall Single(void*,const void* record,int value,int layer,bool isGroup,char* out,int mode) {
    check(record==row && isGroup==grouped && InlineRanges::ScalarMode(mode));
    actualSeen=value; layerSeen=layer;
    if (!text.empty()) std::snprintf(out,256,"%s",text.c_str());
    else std::snprintf(out,256,"%+d Defense",value);
    return formatSuccess;
}
}
int main() {
    check(InlineRanges::ScalarMode(0) && InlineRanges::ScalarMode(4));
    for (int mode : {-1,1,2,3,5,6,7,8}) check(!InlineRanges::ScalarMode(mode));
    InlineRanges::Functions f{Bank,Row,Normalize,Group,Single};
    char out[256]{}; row[0x32]=19;
    auto run=[&](int actual=118,int lo=80,int hi=120) {
        return InlineRanges::Format(f,nullptr,nullptr,31,9,lo,hi,actual*256,out,4);
    };
    check(run()==1 && std::strcmp(out,"[+80 - +120] +118 Defense")==0);
    check(actualSeen==118 && layerSeen==9 && normalizeCalls==3);
    check(InlineRanges::Format(f,nullptr,nullptr,31,9,80,120,118*256,out,0)==1);
    check(std::strcmp(out,"[+80 - +120] +118 Defense")==0);
    check(run(200)==1 && std::strcmp(out,"[+80 - +120] +200 Defense")==0);
    check(run(-8,-10,-5)==1 && std::strcmp(out,"[-10 - -5] -8 Defense")==0);
    check(run(1,-5,10)==1 && std::strcmp(out,"[-5 - +10] +1 Defense")==0);
    text="Defense locale: +118";
    check(run()==1 && std::strcmp(out,"[+80 - +120] Defense locale: +118")==0);
    text=std::string(250,'a');
    check(run()==1 && std::strlen(out)==250 && out[0]=='a'); // Preserve actual line on overflow.
    text.clear();
    check(run(118,80,80)==1 && std::strcmp(out,"+118 Defense")==0);
    row[0x32]=15; check(run()==1 && std::strcmp(out,"+118 Defense")==0);
    row[0x32]=19; grouped=true;
    check(run()==1 && std::strcmp(out,"+118 Defense")==0);
    emit=false; check(run()==0 && !out[0]); grouped=false; emit=true;
    formatSuccess=false; check(run()==0 && !out[0]); formatSuccess=true;
    returnRow=false; check(run()==-1); returnRow=true;
    int a=64,b=128; check(InlineRanges::DisplayBounds(5,a,b) && a==50 && b==100);
    a=5;b=10; check(InlineRanges::DisplayBounds(20,a,b) && a==-10 && b==-5);
    a=-10;b=5; check(InlineRanges::DisplayBounds(29,a,b) && a==0 && b==10);
    a=INT_MIN;b=-1;check(!InlineRanges::DisplayBounds(20,a,b));
    a=INT_MAX-1;b=INT_MAX;check(InlineRanges::DisplayBounds(19,a,b));
    std::puts("Passed bounds + actual, normalization, signed ranges, localized text preservation, overflow and native suppression.");
}
