#include "stash_feedback_policy.h"
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace {
void Check(bool ok,const char* reason) {
    if(!ok){std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);}
}
}

int main() {
    using InventoryDepositStashFeedback::Policy::Mailbox;
    Mailbox mailbox{};
    Check(!mailbox.Observe(10,20) && mailbox.count==0,
        "observations outside an accepted batch cannot create feedback");
    mailbox.Begin();
    Check(mailbox.batch && mailbox.Observe(10,20) && mailbox.Observe(11,20) &&
        mailbox.Observe(12,40) && mailbox.count==3 && mailbox.next==0,
        "each different item is retained even when several share one class");
    Check(mailbox.batchId==1 && mailbox.batches[0]==1 && mailbox.batches[1]==1 && mailbox.batches[2]==1,
        "each copied confirmation retains its originating batch identity");
    Check(!mailbox.Observe(10,99) && mailbox.count==3 && mailbox.classes[0]==20,
        "repeated removal identity cannot replace its original copied class");
    mailbox.next=2;
    Check(!mailbox.Observe(10,20) && !mailbox.Observe(11,20) && mailbox.count==3,
        "drained observations stay deduplicated throughout the same batch");
    mailbox.End();
    Check(!mailbox.batch && !mailbox.Observe(13,50) && mailbox.count==3 &&
        mailbox.next==2 && mailbox.classes[2]==40,
        "ending a batch retains pending confirmations but refuses new callbacks");
    mailbox.Begin();
    Check(mailbox.batch && mailbox.count==1 && mailbox.classes[0]==40 && mailbox.next==0 &&
        mailbox.Observe(10,99) && mailbox.count==2 && mailbox.classes[1]==99,
        "a rapid new batch retains undrained successful confirmations while allowing reused runtime identities");
    Check(mailbox.batchId==2 && mailbox.batches[0]==1 && mailbox.batches[1]==2,
        "mailbox compaction preserves the old batch tag and stamps only new confirmations with the new batch");
    mailbox.Clear();
    Check(!mailbox.batch && mailbox.count==0 && mailbox.next==0 && mailbox.batchId==0 &&
        mailbox.batches[0]==0 && mailbox.batches[1]==0 && !mailbox.Observe(10,20),
        "session or shutdown cancellation clears pending work and ends observation");

    mailbox.Begin();
    for(std::uint32_t i=0;i<256;++i)
        Check(mailbox.Observe(i,1000+i),"the full supported inventory snapshot fits");
    Check(!mailbox.Observe(999,4000) && mailbox.count==256,
        "mailbox capacity is bounded and overflow is refused without overwriting entries");
    for(std::uint32_t i=0;i<256;++i)
        Check(mailbox.ids[i]==i && mailbox.classes[i]==1000+i && mailbox.batches[i]==1,
            "capacity rejection preserves all previously copied identities, classes and batch tags");
    mailbox.End();mailbox.Begin();
    Check(mailbox.count==256 && mailbox.next==0 && mailbox.seenCount==0 && !mailbox.Observe(999,4000) &&
        mailbox.classes[0]==1000 && mailbox.classes[255]==1255 &&
        mailbox.batches[0]==1 && mailbox.batches[255]==1 && mailbox.batchId==2,
        "a full undrained preceding batch keeps its confirmations and gracefully refuses excess feedback");
    mailbox.next=mailbox.count;mailbox.Begin();
    Check(mailbox.count==0 && mailbox.Observe(0,1000) && mailbox.batches[0]==3,
        "draining the preceding full batch makes capacity available to a new batch");
    mailbox.next=mailbox.count;
    Check(!mailbox.Observe(0,1000) && mailbox.count==1,
        "dedupe persists after every observation has been consumed");
    mailbox.Begin();
    constexpr auto unknown=(std::numeric_limits<std::uint32_t>::max)();
    Check(mailbox.Observe(unknown,unknown) && mailbox.ids[0]==unknown && mailbox.classes[0]==unknown,
        "mailbox does not guess native validity or classify data on the game thread");
    mailbox.Clear();
    const auto batchLimit=(std::numeric_limits<std::uint64_t>::max)();
    mailbox.batchId=batchLimit-1;mailbox.Begin();
    Check(mailbox.Observe(10,20) && mailbox.batches[0]==batchLimit,
        "the final available batch tag remains usable");
    mailbox.End();mailbox.Begin();
    Check(!mailbox.batch && mailbox.batchId==batchLimit && !mailbox.Observe(11,30) &&
        mailbox.count==1 && mailbox.next==0 && mailbox.classes[0]==20 && mailbox.batches[0]==batchLimit,
        "batch-tag exhaustion refuses new feedback without wrapping or discarding pending confirmations");
    mailbox.Clear();mailbox.Begin();
    Check(mailbox.batchId==1 && mailbox.Observe(10,20) && mailbox.batches[0]==1,
        "full cancellation restores feedback after exhausted batch tags");

    using InventoryDepositStashFeedback::Policy::Timers;
    Timers timers{};
    Check(!timers.Any() && !timers.Active(2,0),"empty timers never request native updates");
    Check(timers.Mark(2,1,1000,100,2) && timers.Mark(3,1,1050,100,1) && timers.Mark(4,1,1150,200,1),
        "every advanced destination can be flashed independently");
    Check(!timers.Mark(2,1,1100,200,3) && timers.tabs[2].started==1000 &&
        timers.tabs[2].fadeMs==100 && timers.tabs[2].cycles==2,
        "another item in the same batch cannot restart a tab or replace its existing animation settings");
    Check(timers.Alpha(2,1100)==1.0f && timers.Alpha(3,1100)==0.5f && !timers.Active(4,1100),
        "tab pulse phase follows its own start time instead of a shared last-deposit clock");
    Check(timers.Alpha(2,1200)==0.0f && timers.Alpha(2,1300)==1.0f &&
        timers.Alpha(2,1350)==0.5f && !timers.Active(2,1400),
        "each pulse rises and falls and stops at its configured duration");
    Check(timers.Active(2,1300) && !timers.Active(3,1300) && timers.Active(4,1300),
        "one tab can expire while the other affected destinations still animate");
    Check(timers.Marked(3) && timers.Any() && timers.NeedsUpdate(),
        "expired timers retain pending native cleanup until acknowledged");
    timers.Clear(3);
    Check(!timers.Marked(3) && timers.Marked(2) && timers.Marked(4),
        "clearing one destination cannot erase another tab's feedback");
    Check(!timers.Mark(3,1,1400,100,1) && !timers.Marked(3),
        "late same-batch confirmations cannot replay a tab after its completed animation was cleared");
    Check(timers.Mark(2,2,1450,100,1) && timers.Active(2,1600) && timers.Alpha(2,1500)==0.5f &&
        timers.tabs[4].started==1150,
        "a new batch restarts only its own destination's animation");
    Check(!timers.Mark(2,0,2000,100,1) && timers.tabs[2].started==1450,
        "a missing batch identity cannot replace existing feedback");
    for(unsigned tab:{0u,1u,5u,unknown})
        Check(!timers.Mark(tab,3,1000,100,1) && !timers.Marked(tab) && !timers.Active(tab,1100) &&
            timers.Alpha(tab,1100)==0.0f,"unknown and ordinary tabs fail open without an array access");
    for(const auto fade:{0u,99u,2001u,unknown})
        Check(!timers.Mark(2,3,2000,fade,1) && timers.tabs[2].started==1450,
            "invalid native fade data cannot replace existing feedback");
    for(const auto cycles:{0u,11u,unknown})
        Check(!timers.Mark(2,3,2000,100,cycles) && timers.tabs[2].started==1450,
            "invalid native cycle data cannot create unbounded feedback");
    const auto clockLimit=(std::numeric_limits<std::uint64_t>::max)();
    Check(timers.Mark(2,3,clockLimit-200,100,1) && timers.Active(2,clockLimit-1) &&
        !timers.Active(2,clockLimit) && !timers.Active(2,0),
        "elapsed-time comparison avoids overflowing an absolute expiry timestamp");
    timers.Clear();
    Check(!timers.Any() && !timers.NeedsUpdate(),"session cancellation removes all timer ownership");
    Check(timers.Mark(2,1,1000,100,1),
        "full cancellation also resets batch deduplication for a fresh mailbox generation");
    std::puts("Stash feedback identity, batch tags, bounded capacity, once-per-batch animation and cancellation passed.");
}
