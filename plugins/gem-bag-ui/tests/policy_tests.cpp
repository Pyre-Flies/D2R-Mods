#include "policy.h"
#include "layout.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <set>
void Check(bool b,const char* label){if(!b){std::fprintf(stderr,"FAIL %s\n",label);std::exit(1);}}
struct Backend {
    int credits=2,gems=0;bool space=true,createFail{},verifyFail{},publishFail{},removeFail{},restoreFail{},halted{};
    bool Preflight(){return credits>0&&space;}
    bool Debit(){--credits;return true;}
    bool Create(){if(createFail)return false;++gems;return true;}
    bool Verify(){return !verifyFail;}
    bool Publish(){return !publishFail;}
    bool Remove(){if(removeFail)return false;--gems;return true;}
    bool Restore(){if(restoreFail)return false;++credits;return true;}
    void Halt(){halted=true;}
};
namespace {
using namespace GemBag;
// Reproduce the game's localization output, including its private-use color
// introducer. No copied game catalog is compiled into the plugin.
GemLabels sampleLabels{};
int failure=-1;bool terminateCopy=true;
D2RL::Localization::Result __cdecl CopyLabel(const D2RL::PluginContext*,const char* key,char* out,uint32_t size,uint32_t* required) noexcept {
    for(unsigned i=0;i<Gems.size();++i)if(!std::strcmp(key,Gems[i].nameKey+1)) {
        if(static_cast<int>(i)==failure)return D2RL::Localization::Result::NotFound;
        *required=static_cast<uint32_t>(sampleLabels[i].size()+1);
        if(size<*required)return D2RL::Localization::Result::BufferTooSmall;
        if(!terminateCopy){std::memset(out,'x',size);return D2RL::Localization::Result::Success;}
        std::memcpy(out,sampleLabels[i].c_str(),*required);return D2RL::Localization::Result::Success;
    }
    return D2RL::Localization::Result::NotFound;
}
}
int main(int argc,char** argv) {
    using namespace GemBag;
    unsigned checks=0;
    std::set<uint32_t> codes;
    for(const auto& g:Gems)Check(codes.insert(g.code).second,"unique gem code");
    Check(codes.size()==8,"all eight gems");
    for(unsigned w=1;w<=16;++w)for(unsigned h=1;h<=16;++h) {
        std::array<uintptr_t,256> cells{};cells.fill(1);
        Check(!FreeCell(w,h,cells),"full grid refusal");
        for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x) {
            cells[y*w+x]=0;auto found=FreeCell(w,h,cells);
            Check(found&&found->x==x&&found->y==y,"free cell in correct bounds");++checks;cells[y*w+x]=1;
        }
    }
    for(unsigned gem=0;gem<8;++gem)for(auto container:{Container::Inventory,Container::Cube,Container::PersonalStash,Container::SharedStash}) {
        Info i{};i.handle=1;i.runtimeId=3;i.code=BagCode;i.container=container;i.sharedStashPage=4;
        Check(Supported(i),"requested storage supported");
        auto moved=i;moved.x=1;Check(!Same(i,moved),"moved bag stale");
        moved=i;moved.sharedStashPage=5;Check(!Same(i,moved),"shared page identity");
        for(int credit:{1,2,65535}) {
            Backend b;b.credits=credit;
            Check(Withdraw(b)&&b.credits==credit-1&&b.gems==1&&!b.halted,"one gem one credit");
            b=Backend{};b.credits=credit;b.createFail=true;
            Check(!Withdraw(b)&&b.credits==credit&&!b.gems&&!b.halted,"creation failure refunds even last credit");
            for(bool publish:{false,true}) {
                b=Backend{};b.credits=credit;b.verifyFail=!publish;b.publishFail=publish;
                Check(!Withdraw(b)&&b.credits==credit&&!b.gems&&!b.halted,"failed result is compensated");
            }
            ++checks;
        }
    }
    Backend b;b.space=false;Check(!Withdraw(b)&&b.credits==2&&!b.gems,"no mutation on full grid");
    b=Backend{};b.credits=0;Check(!Withdraw(b)&&!b.gems,"empty bag refusal");
    b=Backend{};b.verifyFail=true;b.removeFail=true;Check(!Withdraw(b)&&b.halted&&b.credits==1&&b.gems==1,"never refund an unremoved output");
    b=Backend{};b.createFail=true;b.restoreFail=true;Check(!Withdraw(b)&&b.halted&&!b.gems,"uncertain compensation stops later actions");
    Info i{};i.handle=1;i.runtimeId=3;i.code=BagCode;i.container=Container::SharedStash;i.sharedStashPage=UINT32_MAX;
    Check(!Supported(i),"unknown shared page refusal");
    Info ui{};ui.code=BagCode;ui.classId=746;ui.runtimeId=137;ui.container=Container::Cube;ui.inventoryPage=3;ui.x=4;ui.y=2;
    auto server=ui;server.runtimeId=999;server.handle=15;server.itemLevel=101;ui.itemLevel=99;
    Check(SameStoredTarget(ui,server),"cross-domain bag resolution uses stable storage and item properties");
    server.x=5;Check(!SameStoredTarget(ui,server),"different authoritative cell refused");server=ui;
    server.code=Gems[0].code;Check(!SameStoredTarget(ui,server),"different authoritative item refused");
    ui.container=Container::SharedStash;server=ui;server.sharedStashPage=3;
    Check(!SameStoredTarget(ui,server),"another shared page is never the same target");
    Check(FreshHover(1000,1400,4,-4),"fresh bag hover at click boundary");
    Check(!FreshHover(1000,1401,0,0)&&!FreshHover(1000,1100,5,0)&&!FreshHover(1000,999,0,0)&&!FreshHover(0,100,0,0),"stale or moved mouse target refused");
    Gesture g;Check(g.Edge(0x2000,true),"first B press");g.owned|=0x2000;
    Check(!g.Edge(0x2000,true)&&g.ReleaseOwned(0x2000,true),"held B cannot reopen or close");
    g.Edge(0x2000,false);Check(g.ReleaseOwned(0x2000,false)&&!g.owned,"owned release swallowed");
    Check(g.Edge(0x2000,true),"fresh B after release");
    for(unsigned n=0;n<Gems.size();++n)sampleLabels[n]=std::string("\xee\x81\xbe")+"U"+Gems[n].name;
    const D2RL::LocalizationService localization{.serviceSize=D2RL::LocalizationServiceSize,.serviceVersion=D2RL::LocalizationService::AbiVersion,.getStringByKey=CopyLabel};
    auto labels=ReadGemLabels(nullptr,&localization);Check(labels.has_value(),"all eight copied localized labels resolved");
    for(unsigned n=0;n<Gems.size();++n)Check((*labels)[n]==Gems[n].name,"native color tokens removed before TextBox layout");
    Check(PlainGemLabel("\xc3\xbf" "cUSch\xc3\xa4" "del")=="Sch\xc3\xa4" "del","source UTF-8 color stripped without damaging German letters");
    Check(PlainGemLabel("\xee\x81\xbe" "U\xe7\xb4\xab\xe5\xaf\xb6\xe7\x9f\xb3")=="\xe7\xb4\xab\xe5\xaf\xb6\xe7\x9f\xb3","native color stripped without damaging CJK letters");
    Check(PlainGemLabel("\xff" "c1A\xee\x81\xbe" "UB\xc3\xbf" "c0C")=="ABC","legacy and repeated internal color instructions stripped");
    Check(PlainGemLabel("*U text \xc3\xbf" "c")=="*U text \xc3\xbf" "c","literal lookalikes and incomplete tokens preserved");
    failure=3;Check(!ReadGemLabels(nullptr,&localization),"missing translation never replaced with an English name");failure=-1;
    terminateCopy=false;Check(!ReadGemLabels(nullptr,&localization),"unterminated copied text refused");terminateCopy=true;
    sampleLabels[0]=std::string(1024,'a');Check(!ReadGemLabels(nullptr,&localization),"oversized translation refused");
    sampleLabels[0]=std::string("\xee\x81\xbe")+"U"+Gems[0].name;
    const auto layout=Layout(*labels);
    Check(layout.find("\xee\x81\xbe")==std::string::npos&&layout.find("@d2r:")==std::string::npos,"emitted native layout contains copied plain names");
    Check(JsonText("\"\\\n\t")=="\"\\\"\\\\\\u000a\\u0009\"","localized text is JSON escaped");
    for(unsigned n=0;n<8;++n)Check(layout.find("Gem"+std::to_string(n))!=std::string::npos&&layout.find(Ui::Art[n])!=std::string::npos,"all eight inventory sprites referenced");
    for(const auto size:{std::pair{1920,1080},std::pair{3840,2160},std::pair{2560,1080},std::pair{1280,800},std::pair{1280,720}}) {
        const auto [w,h]=size;
        for(double scale:{0.25,0.5,0.75,1.0,1.25})for(const auto point:{std::pair{0,0},std::pair{w-1,0},std::pair{0,h-1},std::pair{w-1,h-1},std::pair{w/2,h/2}}) {
            // Nonzero existing origin/local values catch adding the parent
            // origin twice. Scale is measured, not inferred from resolution.
            const Ui::Rect previous{100,90,Ui::Card.width,Ui::Card.height};
            const auto moved=Ui::Placement(previous,57,69,scale,w,h,point.first,point.second,false);
            Check(moved.has_value(),"cursor popup fits representative viewport");
            const auto x=57+(moved->x-previous.x)*scale,y=69+(moved->y-previous.y)*scale;
            Check(x>=7&&y>=7&&x+Ui::Card.width*scale<=w-7&&y+Ui::Card.height*scale<=h-7,"all cursor corners clamp inside actual viewport");
            if(point.first==w/2&&point.second==h/2)Check(std::abs(x-(point.first+8))<=scale/2+0.01&&std::abs(y-(point.second+8))<=scale/2+0.01,"popup starts beside the cursor");
            for(unsigned n=0;n<8;++n)for(unsigned other=0;other<8;++other) {
                const auto r=Ui::GemRect(n),o=Ui::GemRect(other);
                Check(o.Contains(r.x+r.width/2.0,r.y+r.height/2.0)==(n==other),"every gem has a distinct icon hit rectangle");
            }
        }
    }
    Check(!Ui::Placement(Ui::Card,0,0,0,1920,1080,0,0,false)&&!Ui::Placement(Ui::Card,0,0,1,50,50,0,0,false),"unknown scale or undersized viewport refused");
    if(argc==2){std::ofstream f(argv[1]);f<<layout;}
    std::printf("%u placement/storage/conservation cases plus failure and gesture checks passed.\n",checks);
}
