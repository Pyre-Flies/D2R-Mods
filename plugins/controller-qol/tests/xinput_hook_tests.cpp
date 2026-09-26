#include "xinput_hook.h"
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <thread>
extern "C" int XInputFixture();
extern "C" int XInputFixturePrior();
QolXInput::Record priorRecord;
int Prior(){return 456;}
int PriorDetour(){return reinterpret_cast<int(*)()>(priorRecord.trampoline)();}
using Fn=int(*)();
QolXInput::Record record;
QolXInput::Gate gate;
int calls=0,filtered=0,laterCalls=0;
int Detour(){++calls;const int result=reinterpret_cast<Fn>(record.trampoline)();gate.Run([]{++filtered;});return result;}
int Later(){++laterCalls;return Detour();}
void check(bool b){if(!b){std::fputs("XInput ownership test failed\n",stderr);std::exit(1);}}
void* nearPage(void* target){
 const auto base=reinterpret_cast<uintptr_t>(target)&~uintptr_t(65535);
 for(uintptr_t offset=65536;offset<0x10000000;offset+=65536)
  if(auto p=VirtualAlloc(reinterpret_cast<void*>(base+offset),64,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE))return p;
 return nullptr;
}
int main(){
 auto priorTarget=reinterpret_cast<uint8_t*>(&XInputFixturePrior);
 uint64_t priorBytes{};std::memcpy(&priorBytes,priorTarget,8);
 uint64_t priorJump=priorBytes;auto pb=reinterpret_cast<uint8_t*>(&priorJump);pb[0]=0xe9;
 int32_t priorDelta=static_cast<int32_t>(reinterpret_cast<intptr_t>(&Prior)-(reinterpret_cast<intptr_t>(priorTarget)+5));
 std::memcpy(pb+1,&priorDelta,4);check(QolXInput::Publish(priorTarget,priorBytes,priorJump));
 auto priorRelay=nearPage(priorTarget);check(priorRelay!=nullptr);
 check(QolXInput::Install(priorTarget,reinterpret_cast<void*>(&PriorDetour),priorRelay,priorRecord));
 check(XInputFixturePrior()==456);
 uint8_t out[64]{},unknown[8]={0x48,0x8b,0x05};const void* pred{};
 check(!QolXInput::Plan(unknown,0x1000,out,pred));
 uint8_t branch[8]={0xe9,0xfb,0xff,0xff,0xff};
 check(QolXInput::Plan(branch,0x1000,out,pred) && pred==reinterpret_cast<void*>(0x1000));
 auto target=reinterpret_cast<uint8_t*>(&XInputFixture);
 uint64_t original{};std::memcpy(&original,target,8);
 auto relay=nearPage(target);check(relay!=nullptr);
 check(QolXInput::Install(target,reinterpret_cast<void*>(&Detour),relay,record));
 check(XInputFixture()==123 && calls==1 && filtered==0);
 gate.Enable();check(XInputFixture()==123 && calls==2 && filtered==1);
 uint64_t owned{};std::memcpy(&owned,target,8);
 check(!QolXInput::Publish(target,original,original)); // stale owner cannot overwrite us
 check(!std::memcmp(target,&owned,8));
 // Simulate a later owner whose predecessor is our retained detour.
 auto later=static_cast<uint8_t*>(nearPage(target));check(later!=nullptr);
 QolXInput::Jump(later,reinterpret_cast<void*>(&Later));DWORD old{};
 check(VirtualProtect(later,64,PAGE_EXECUTE_READ,&old)!=FALSE);
 FlushInstructionCache(GetCurrentProcess(),later,64);
 uint64_t next=owned;auto bytes=reinterpret_cast<uint8_t*>(&next);bytes[0]=0xe9;
 int32_t delta=static_cast<int32_t>(later-(target+5));std::memcpy(bytes+1,&delta,4);
 check(QolXInput::Publish(target,owned,next));
 gate.Disable();
 check(!std::memcmp(target,&next,8));
 check(XInputFixture()==123 && laterCalls==1 && calls==3 && filtered==1);
 check(reinterpret_cast<Fn>(record.trampoline)()==123); // remains executable after shutdown
 std::atomic<bool> entered=false,release=false,stopped=false;
 gate.Enable();
 std::thread active([&]{gate.Run([&]{entered=true;while(!release.load())std::this_thread::yield();});});
 while(!entered.load())std::this_thread::yield();
 std::thread shutdown([&]{gate.Disable();stopped=true;});
 check(!stopped.load());release=true;active.join();shutdown.join();check(stopped.load());
 gate.Run([]{check(false);});
 std::puts("Passed verified entry, stale publication, predecessor return, later-owner preservation and drained pass-through shutdown.");
}
