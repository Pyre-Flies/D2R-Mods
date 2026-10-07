#include "client_identify_policy.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
using namespace QolClientIdentify;
void Check(bool value,const char* why){if(!value){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
unsigned calls{};
uint8_t expectedUse{};
uint8_t expectedSourcePage{},expectedTargetPage{};
void __fastcall Request(uint8_t use,uint32_t source,int32_t mode,uint8_t sourcePage,uint32_t sourceCell,
    uint32_t target,int32_t targetType,uint8_t targetPage,uint32_t targetCell,const uint32_t* ids,const uint32_t* xs,const uint32_t* ys) noexcept {
    ++calls;
    Check(use==expectedUse && source==81 && target==25,"native use flag and ordered source/target identities preserved");
    Check(mode==0 && sourcePage==expectedSourcePage && targetType==4 && targetPage==expectedTargetPage,"stored source and target pages preserved independently");
    Check(sourceCell==0x00030002 && targetCell==0x00000008,"packed XY cells preserved independently");
    Check(!ids && !xs && !ys,"equipment arrays excluded for stored-item request");
}
int main() {
    Submit(Request,0,81,0x00030002,25,0x00000008);Check(calls==1,"false native use flag is valid and submits once");
    expectedUse=1;Submit(Request,1,81,0x00030002,25,0x00000008);Check(calls==2,"true native use flag is preserved and submits once");
    for(uint8_t sourcePage:{0,4})for(uint8_t targetPage:{0,4}) {
        expectedSourcePage=sourcePage;expectedTargetPage=targetPage;
        Submit(Request,1,81,0x00030002,25,0x00000008,sourcePage,targetPage);
    }
    Check(calls==6,"Inventory and Personal source/target combinations submit once each");
    for(uint8_t rejected:{1,2,3,255}) {
        Submit(Request,1,81,0x00030002,25,0x00000008,rejected,0);
        Submit(Request,1,81,0x00030002,25,0x00000008,0,rejected);
    }
    Submit(nullptr,1,81,0x00030002,25,0x00000008);
    Check(calls==6,"Cube, equipment, unknown pages and missing builder cannot submit");
    Check(Cell(2,3,0x00030002),"native packed coordinates agree with SDK source");
    Check(!Cell(3,2,0x00030002),"swapped coordinates rejected");
    Check(!Cell(-1,0,0xffff) && !Cell(256,0,256),"coordinates that request builder would truncate rejected");
    Check(Confirmed(true,1,0),"last charge confirms without deleting tome");
    Check(Confirmed(true,20,19),"exact single charge confirms");
    Check(!Confirmed(false,20,19),"charge change alone is not identification");
    Check(!Confirmed(true,20,20),"ID flag alone is not completed tome use");
    Check(!Confirmed(true,20,18) && !Confirmed(true,0,-1),"unexpected quantity or invalid initial quantity does not confirm");
    Check(LooseScrollQuantity(0) && LooseScrollQuantity(1),"ordinary loose-scroll quantity conventions admitted");
    Check(!LooseScrollQuantity(-1) && !LooseScrollQuantity(2),"unknown quantity and unqualified modded stacks excluded");
    Check(ScrollConfirmed(true,false,false),"exact scroll removal and target identification confirm");
    Check(!ScrollConfirmed(false,false,false) && !ScrollConfirmed(true,true,false) && !ScrollConfirmed(true,false,true),"missing target flag or either surviving source prevents confirmation");
    std::puts("direct remote identification ABI and evidence regressions passed");
}
