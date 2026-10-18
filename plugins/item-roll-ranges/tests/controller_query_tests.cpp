// Compose the production Item Roll Ranges adapter with QOL's exact caller policy.
#include <windows.h>
#include "../../controller-qol/src/range_input_policy.h"
static bool testForegroundActive=true;
static HWND WINAPI TestForeground() {return testForegroundActive?reinterpret_cast<HWND>(1):nullptr;}
static DWORD WINAPI TestWindowProcess(HWND,DWORD* process) {*process=GetCurrentProcessId();return 1;}
#define GetForegroundWindow TestForeground
#define GetWindowThreadProcessId TestWindowProcess
#include "../src/plugin.cpp"
#undef GetForegroundWindow
#undef GetWindowThreadProcessId
#include <cstdio>
#include <cstdlib>
#include <source_location>

namespace {
unsigned checks{},calls{},physical{},seenUser{},seenMask{};
void* seenDevice{};
std::uint64_t nativeHigh{};
std::uintptr_t nativeCaller{},qolCaller{},tooltipCaller{};
bool calibrate=true,qolEnabled=true,panel=true;
volatile unsigned completed{};
void check(bool value,std::source_location at=std::source_location::current()) {
    ++checks;
    if(!value){std::fprintf(stderr,"FAIL controller query at line %u\n",at.line());std::exit(1);}
}
bool __fastcall Panel(int id){return panel && id==1;}
__declspec(noinline) std::uint64_t __fastcall NativeButtons(void* device,unsigned user,unsigned mask){
    ++calls;seenDevice=device;seenUser=user;seenMask=mask;
    nativeCaller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    return nativeHigh | ((physical&mask)!=0?1u:0u);
}
__declspec(noinline) std::uint64_t __fastcall QolButtons(void* device,unsigned user,unsigned mask){
    qolCaller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if(calibrate)tooltipCaller=qolCaller;
    return NativeButtons(device,user,QolRangePolicy::Mask(qolEnabled,qolCaller,tooltipCaller,user,mask));
}
__declspec(noinline) std::uint64_t TooltipCall(ButtonQueryFn slot,void* device,unsigned user,unsigned mask){
    const auto value=slot(device,user,mask);completed=completed+1;return value;
}
__declspec(noinline) std::uint64_t OtherCall(ButtonQueryFn slot,void* device,unsigned user,unsigned mask){
    const auto value=slot(device,user,mask);completed=completed+2;return value;
}
std::uint64_t Query(unsigned mask=0x800,unsigned user=0,void* device=reinterpret_cast<void*>(1)){
    const auto before=calls;
    const auto value=TooltipCall(&AlwaysButtonAdapter,device,user,mask);
    check(calls==before+1);check(seenDevice==device && seenUser==user);
    return value;
}
}
int main(){
    active=true;alwaysReady=true;alwaysEnabled=false;testPanel=&Panel;
    originalButtons=&QolButtons;physical=0x200;
    check(TooltipCall(&QolButtons,reinterpret_cast<void*>(1),0,0x800)==1);
    calibrate=false;buttonReturn=tooltipCaller;
    check(Query()==1); // Regression: rev.52's intermediate CALL loses QOL caller.
    check(qolCaller==tooltipCaller && seenMask==0x200);
    physical=0;check(Query()==0);
    physical=0x800;check(Query()==0); // QOL keeps the tooltip's R1 binding.
    alwaysEnabled=true;physical=0;check(Query()==1);
    nativeHigh=0x12340000;check(Query()==0x12340001);nativeHigh=0;
    testForegroundActive=false;check(Query()==0);testForegroundActive=true;
    panel=false;check(Query()==0);panel=true;
    active=false;check(Query()==0);active=true;
    alwaysReady=false;check(Query()==0);alwaysReady=true;
    check(Query(0x800,8)==0);check(Query(0x800,0,nullptr)==0);
    physical=0x100;check(Query(0x100)==1 && seenMask==0x100);
    physical=0;check(OtherCall(&AlwaysButtonAdapter,reinterpret_cast<void*>(1),0,0x800)==0);
    physical=0x800;check(OtherCall(&AlwaysButtonAdapter,reinterpret_cast<void*>(1),0,0x800)==1);
    check(qolCaller!=tooltipCaller && seenMask==0x800);
    alwaysEnabled=false;qolEnabled=false;
    check(Query()==1 && seenMask==0x800);physical=0x200;check(Query()==0);
    originalButtons=&NativeButtons;physical=0x800;
    check(Query()==1 && nativeCaller==tooltipCaller);
    physical=0x200;check(Query()==0);
    alwaysEnabled=true;physical=0;check(Query()==1);
    alwaysReady=false;nativeHigh=0xfedcba9876543210ull;
    check(Query()==nativeHigh && nativeCaller==tooltipCaller);
    void* owned=reinterpret_cast<void*>(&AlwaysButtonAdapter);buttonSlot=&owned;
    modifierSlot=widgetSlot=nullptr;RestoreAlways();
    check(owned==reinterpret_cast<void*>(&NativeButtons));
    owned=reinterpret_cast<void*>(&QolButtons);RestoreAlways();
    check(owned==reinterpret_cast<void*>(&QolButtons));buttonSlot=nullptr;
    std::printf("Controller query composition: %u checks passed; R1/RT, always mode, caller/arguments/result, one native call and ownership.\n",checks);
}
