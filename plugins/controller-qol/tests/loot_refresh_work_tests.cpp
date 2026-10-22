#include <windows.h>
#include "client_loot_policy.h"
#include "loot_performance.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

using namespace QolClientLoot;
std::recursive_mutex mutex;
std::atomic<bool> active{true};
bool ready=true,sessionReady=true,wasHeld{};
uint64_t epoch=1,fixtureTick=1000;
int queuedApproach{};
Slots slots{};
std::unordered_map<uint32_t,uint64_t> visible;
std::unordered_map<uint32_t,Candidate> nativeItems;
std::unordered_set<uint32_t> hiddenItems;
unsigned reads{},hiddenReads{},checks{};
void Log(const char*) {}
struct OwnedContext {void(*LogInfo)(const char*)=&Log;};
OwnedContext ownedContext;
OwnedContext* context=&ownedContext;
bool Ready() noexcept {return ready;}
bool Session(void*& player) noexcept {player=&ownedContext;return sessionReady;}
bool Read(void*,uint32_t guid,uint32_t radius,Candidate& item) noexcept {
    ++reads;const auto found=nativeItems.find(guid);
    if(found==nativeItems.end() || !Eligible(4,3,found->second.distance,radius,0))return false;
    item=found->second;return true;
}
bool HiddenCandidate(const Candidate& item) noexcept {
    if(!item.guid)return false;++hiddenReads;return hiddenItems.contains(item.guid);
}
ULONGLONG Clock() noexcept {return fixtureTick;}
#define GetTickCount64 Clock
#include "loot_refresh_runtime.inc"
#undef GetTickCount64
void Check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
bool SameAssignments(const Slots& expected){for(unsigned i=0;i<slots.size();++i)if(slots[i].guid!=expected[i].guid)return false;return true;}
int main() {
    for(uint32_t id=1;id<=100;++id){visible[id]=1000;nativeItems[id]={id,id,2,1,static_cast<int>(id)};}
    Check(Refresh(10,true) && reads==100,"first hold evaluates the full visible candidate set");
    const auto snapshot=slots;Check(snapshot[0].guid==100 && snapshot[6].guid==94,"original priority and seven-slot limit retained");
    reads=hiddenReads=0;fixtureTick=2000;visible[1000]=0;
    for(auto& [id,item]:nativeItems){(void)id;item.rank=-item.rank;}
    (void)Refresh(10,true);
    Check(reads==0,"steady hold does not repeat native candidate reads whose assignments are discarded");
    Check(hiddenReads==7 && SameAssignments(snapshot),"all held targets still get live hidden checks");
    Check(QolPerf::Counters[QolPerf::Refresh].Read().units==100,
        "refresh report counts first-hold reads and excludes redundant steady reads");
    Check(!visible.contains(1000),"expired observation pruning continues while held");
    hiddenItems.insert(snapshot[0].guid);reads=0;
    Check(Refresh(10,true) && !slots[0].guid && reads==0,"a newly hidden held target is pruned");
    visible[999]=fixtureTick;nativeItems[999]={999,999,7,1,9999};
    (void)Refresh(10,true);Check(!slots[0].guid,"new loot never fills a held hole");
    const auto oldEpoch=epoch;Check(Refresh(10,false) && epoch==oldEpoch+1 && !wasHeld,"release clears assignment and expires queued identity");
    reads=0;Check(Refresh(10,true) && slots[0].guid==999 && reads==101,"release/repress refreshes the candidate set");
    sessionReady=false;reads=0;(void)Refresh(10,true);
    Check(!wasHeld && !slots[0].guid && reads==0,"invalid session clears the held snapshot");sessionReady=true;
    visible.clear();nativeItems.clear();(void)Refresh(10,true);
    visible[77]=fixtureTick;nativeItems[77]={77,77,2,1,1};(void)Refresh(10,true);
    Check(!slots[0].guid,"an initially empty hold stays empty");
    (void)Refresh(10,false);(void)Refresh(10,true);Check(slots[0].guid==77,"a new hold can select newly observed loot");
    const auto before=slots;ready=false;reads=0;(void)Refresh(10,true);
    Check(reads==0 && SameAssignments(before),"unavailable native profile does no new work");
    std::printf("%u production hold-refresh work and identity checks passed.\n",checks);
}
