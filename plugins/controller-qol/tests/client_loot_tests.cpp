#include "client_loot_policy.h"
#include <cstdio>
#define CHECK(x) do {if(!(x)){std::printf("Failure line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main() {
    using namespace QolClientLoot;
    Candidate rune{42,123,2,6,2000}, sword{43,456,7,2,700}, potion{44,789,2,1,150};
    auto slots=Assign({}, {potion,sword,rune});
    CHECK(slots[0].guid==42 && slots[1].guid==43 && slots[2].guid==44);
    // Distances change and a better item appears: retain all displayed mappings.
    rune.distance=9;sword.distance=1;
    Candidate better{45,999,7,1,3000};
    slots=Assign(slots,{better,potion,sword,rune});
    CHECK(slots[0].guid==42 && slots[1].guid==43 && slots[3].guid==45);
    const auto queued=slots[1];
    slots=Assign(slots,{better,potion,rune});
    CHECK(slots[1].guid==0 && slots[3].guid==45);
    Candidate other{46,456,7,2,700};
    slots=Assign(slots,{better,potion,rune,other});
    CHECK(slots[1].guid==46 && !SameItem(queued,slots[1]));
    // Reused GUID with a different item cannot satisfy an old request.
    other.guid=queued.guid;other.code=55;
    CHECK(!SameItem(queued,other));
    Request request{queued,2,1000};
    CHECK(CurrentRequest(request,2,1500,true));
    CHECK(!CurrentRequest(request,2,1501,true));
    CHECK(!CurrentRequest(request,3,1100,true)); // new game/hold epoch
    CHECK(!CurrentRequest(request,2,1100,false)); // released modifier/menu open
    CHECK(!CurrentRequest({},2,1100,true)); // no displayed assignment
    CHECK(Eligible(4,3,10,10,0));
    CHECK(!Eligible(4,3,11,10,0) && !Eligible(4,3,-1,10,0));
    CHECK(!Eligible(4,0,1,10,0) && !Eligible(1,3,1,10,0) && !Eligible(4,3,1,10,1));
    // Empty/invisible/departed candidates clear labels without claiming pickup success.
    slots=Assign(slots,{});
    for(auto s:slots)CHECK(!s.guid);
    // Deterministic ties, at most seven mappings.
    std::vector<Candidate> many;
    for(unsigned i=10;i>0;--i)many.push_back({i,1,2,1,20});
    slots=Assign({},many);
    CHECK(slots[0].guid==1 && slots[6].guid==7);
    std::puts("Client loot: stable slots, stale identities, eligibility, removal and bounded ordering passed.");
}
