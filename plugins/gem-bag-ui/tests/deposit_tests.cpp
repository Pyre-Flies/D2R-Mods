#include "deposit.h"
#include "deposit_fixture.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
using namespace GemBag;
void Check(bool b,const char* message){if(!b){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
Catalog::Definition Build(const TestData::Fixture& f) {
    Catalog::Definitions out;std::string why;Check(Catalog::Build(f.Views(),f.bank,out,why)&&out.size()==1,"withdrawal discovery remains available");return out[0];
}
int main(int argc,char** argv) {
    TestData::Fixture f;TestData::AddDeposits(f);
    const auto d=Build(f);Check(d.deposits.size()==2,"gem type and exact cluster discovered");
    Check(d.deposits[0].minimum==1&&d.deposits[1].minimum==20&&d.deposits[1].maximum==30,"actual captured credit ranges");
    for(unsigned n=0;n<9;++n) {
        auto bad=f;
        switch(n){case 0:bad.Row(5,9)[1]=1;break;case 1:bad.Row(5,9)[2]=1;break;
        case 2:bad.Row(5,9)[0x81]=99;break;case 3:bad.Set(5,9,0x7f,int16_t{19});break;
        case 4:bad.Row(5,9)[0x25]=2;break;case 5:bad.Set(5,9,0x83,1u);break;
        case 6:bad.Row(5,9)[0xb5]=1;break;case 7:bad.Row(5,9)[3]=17;break;
        case 8:bad.Set(5,9,0x20,f.output[0]);break;}
        Check(Build(bad).deposits.empty(),"unsupported/ambiguous deposits refuse independently of withdrawals");
    }
    Info bag{},item{};bag.runtimeId=1;bag.container=Container::SharedStash;bag.inventoryPage=4;bag.sharedStashPage=2;
    item.handle=2;item.runtimeId=2;item.classId=692;item.code=Gems[0].code;item.container=bag.container;item.inventoryPage=4;item.sharedStashPage=2;item.quality=D2RL::Items::Quality::Normal;item.quantity=1;
    Check(DepositPage(bag,item),"same storage page eligible");item.sharedStashPage=3;Check(!DepositPage(bag,item),"other shared page protected");item.sharedStashPage=2;
    item.socketedItemCount=1;Check(!DepositPage(bag,item),"socket contents protected");item.socketedItemCount=0;item.prefixIds[0]=1;Check(!DepositPage(bag,item),"modified item protected");
    struct Backend {
        unsigned mode{},credits=10,items=1;bool halted{},restored{};
        bool Credit(){if(mode==1)return false;credits+=25;return true;}
        bool Consume(){if(mode==2||mode==3||mode==4)return false;items=0;return true;}
        bool Unchanged(){return mode!=3;}
        bool Restore(){if(mode==4)return false;credits-=25;restored=true;return true;}
        bool Verify(){return mode!=5;}
        bool Publish(){return mode!=6;}
        void Halt(){halted=true;}
    };
    for(unsigned mode=0;mode<=6;++mode) {
        Backend b{mode};const auto ok=DepositOne(b);
        Check(ok==(mode==0),"success only after verified consumption and publication");
        if(mode==0)Check(b.credits==35&&!b.items&&!b.halted,"one consumption one credit addition");
        if(mode==1||mode==2)Check(b.credits==10&&b.items==1&&!b.halted,"clean failure preserves both");
        if(mode>=3)Check(b.halted&&!b.restored,"uncertain or committed failure never blindly refunds");
    }
    if(argc==2)for(unsigned bank:{2u,3u}) {
        TestData::Fixture recorded;recorded.bank=bank;const char* names[]={"Items","ItemTypes","Gems","Properties","ItemStatCost","CubeMain"};
        for(unsigned i=0;i<6;++i){std::ifstream input(std::filesystem::path(argv[1])/(std::to_string(bank)+"-"+names[i]+".bin"),std::ios::binary);Check(input.good(),"captured file");recorded.bytes[i]={std::istreambuf_iterator<char>(input),{}};}
        const auto definition=Build(recorded);
        Check(!definition.deposits.empty(),"actual compiled normal-package deposits discovered");
        bool cluster{};for(const auto& rule:definition.deposits)if(rule.code==D2RL::Items::MakeItemCode("1gc")){Check(rule.minimum==20&&rule.maximum==30,"actual cluster recipe");cluster=true;}
        Check(cluster,"actual cluster is included");std::printf("Bank %u: %zu deposit item classes.\n",bank,definition.deposits.size());
    }
    std::puts("Deposit recipe, page exclusion, and compensation cases passed.");
}
