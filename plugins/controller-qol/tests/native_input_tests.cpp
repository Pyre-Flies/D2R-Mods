#include "native_input_policy.h"
#include <cstdio>
#include <vector>
#include <utility>
#define CHECK(x) do {if(!(x)) {std::printf("Failed line %d: %s\n",__LINE__,#x);return 1;}} while(0)
int main() {
    using namespace QolNativeInput;
    AcceptedPress advancedA;unsigned requests=0;bool eligible=false;
    auto single=[&]() noexcept {++requests;return eligible;};
    CHECK(!advancedA.Update(false,true,single) && requests==0);
    CHECK(!advancedA.Update(true,true,single) && requests==1); // ordinary A preserved
    CHECK(!advancedA.Update(true,true,single) && requests==1);
    advancedA.Update(false,false,single);eligible=true;
    CHECK(advancedA.Update(true,true,single) && requests==2);
    CHECK(advancedA.Update(false,true,single)); // release LB first
    CHECK(advancedA.Update(true,true,single) && requests==2); // no second potion
    CHECK(!advancedA.Update(false,false,single));
    CHECK(advancedA.Update(true,true,single) && requests==3);
    // Real DualShock observation: L1+Square must remain visible to QOL,
    // while the native consumer must not also drop/cast with Square.
    auto raw=Decode(0x100|0x4000);
    auto filtered=FilterModified(raw,true,false);
    CHECK(raw.valid && raw.providers==Provider && raw.buttons==0x4100);
    CHECK(Encode(filtered)==0x100);
    CHECK(Encode(FilterModified(raw,false,false))==0x4100);
    // L1 must not suppress, replace or skip ordinary directional navigation.
    for(const unsigned direction:{1u,2u,4u,8u})
        CHECK(Encode(FilterModified(Decode(0x100|direction),true,false))==(0x100|direction));
    // Neutral Cross, including modifier+Cross, remains a native interaction.
    CHECK(Encode(FilterModified(Decode(0x1100),true,false))==0x1100);
    // Inventory A remains native; world A is delivered only to the slot poller.
    GroundAPress groundA;
    CHECK(!groundA.Update(true,false)); // inventory LB+A
    CHECK(groundA.Update(true,true)); // ground LB+A
    CHECK(groundA.Update(true,false)); // LB released first: no native pickup leaks
    CHECK(!groundA.Update(false,false));
    CHECK(!groundA.Update(true,false)); // next neutral A works
    // Trigger keys are distinct from buttons; Shared needs both digital edges.
    raw=Decode(0xd00);
    CHECK(raw.buttons==0x100 && raw.leftTrigger==255 && raw.rightTrigger==255);
    CHECK(Encode(FilterModified(raw,true,false))==0x100);
    CHECK(Encode(FilterModified(raw,true,true))==0xd00);
    Probe::SharedPageGesture page;
    CHECK(page.Handle(true,true,true,7,true,false)==19);
    CHECK(page.Handle(true,true,false,7,true,true)==-1);
    CHECK(page.Handle(true,true,false,7,false,false)==-1);
    // No native evidence after expiry; an idle state is still a valid state.
    CHECK(Decode(0).valid && Encode(Decode(0))==0);
    CHECK(Fresh(1200,1000) && !Fresh(1251,1000) && !Fresh(900,1000) && !Fresh(1000,0));
    CHECK(!KnownKey(0) && !KnownKey(3) && !KnownKey(0x10000) && KnownKey(0x8000));
    // Native event forwarding: no duplicate shortcut press, no release for a
    // consumed key, and ordinary repeat presses retain native repeat behavior.
    unsigned delivered=0;
    std::vector<std::pair<unsigned,bool>> events;
    auto emit=[&](unsigned key,bool down){events.emplace_back(key,down);};
    const auto captured=Decode(0x1100);
    auto game=captured;groundA={};
    if(groundA.Update((game.buttons&0x1000)!=0,true))game.buttons&=~0x1000;
    Deliver(delivered,Encode(game),0x1000,true,emit);
    CHECK(captured.buttons==0x1100 && events.size()==1 && events[0]==std::make_pair(0x100u,true));
    game=Decode(0x1000);
    if(groundA.Update(true,false))game.buttons&=~0x1000;
    Deliver(delivered,Encode(game),0x100,false,emit);
    CHECK(events.size()==2 && events.back()==std::make_pair(0x100u,false));
    CHECK(!groundA.Update(false,false));
    events.clear();delivered=0;
    Deliver(delivered,0x100,0x100,true,emit);
    CHECK(events.size()==1 && events.back()==std::make_pair(0x100u,true));
    Deliver(delivered,0x100,0x4000,true,emit);
    Deliver(delivered,0x100,0x4000,false,emit);
    CHECK(events.size()==1);
    Deliver(delivered,0,0x100,false,emit);
    CHECK(events.size()==2 && events.back()==std::make_pair(0x100u,false));
    Deliver(delivered,0x1000,0x1000,true,emit);
    Deliver(delivered,0x1000,0x1000,true,emit);
    CHECK(events.size()==4 && events.back()==std::make_pair(0x1000u,true));
    // A newly applied modifier releases an already delivered chord button
    // before pressing another allowed key, so held actions do not get stuck.
    events.clear();delivered=0x4000;
    Deliver(delivered,0x100,0x100,true,emit);
    CHECK(events.size()==2 && events[0]==std::make_pair(0x4000u,false));
    CHECK(events[1]==std::make_pair(0x100u,true));
    // Enumerate all normalized states to check conversion preserves each key,
    // including L3/R3, without aliasing digital triggers into face buttons.
    for(unsigned keys=0;keys<65536;++keys) CHECK(Encode(Decode(keys))==keys);
    std::puts("Native state/chord isolation, Shared trigger ownership and expiry passed.");
}
