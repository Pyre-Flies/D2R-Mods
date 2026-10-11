#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace InventoryDepositStashFeedback::Policy {

// The batch observer can run on the game thread. Retain only copied identity
// and class numbers here; classification and native widget work belong to the
// later UI callback. Consumed slots remain in the dedupe set until the next
// batch so repeated verification cannot queue an item twice. Batch tags survive
// compaction, allowing the UI to coalesce destinations across multiple updates.
struct Mailbox {
    std::array<std::uint32_t,256> classes{};
    std::array<std::uint32_t,256> ids{};
    std::array<std::uint64_t,256> batches{};
    std::uint64_t batchId{};
    std::size_t count{},next{},seenCount{};
    bool batch{};

    void Clear() noexcept {
        classes={};ids={};batches={};batchId=0;count=next=seenCount=0;batch=false;
    }
    void Begin() noexcept {
        // Never recycle a live batch tag. Exhaustion disables new feedback
        // until cancellation; deposit submission is independent of this queue.
        if(batchId==(std::numeric_limits<std::uint64_t>::max)()){batch=false;return;}
        ++batchId;
        // A rapid new click can start before the UI consumes the preceding
        // successful removals. Keep those confirmations while resetting only
        // this batch's identity set. Session cancellation uses Clear instead.
        const auto pending=count-next;
        for(std::size_t i=0;i<pending;++i){classes[i]=classes[next+i];batches[i]=batches[next+i];}
        count=pending;next=seenCount=0;ids={};batch=true;
    }
    bool Observe(std::uint32_t id,std::uint32_t classId) noexcept {
        if(!batch)return false;
        for(std::size_t i=0;i<seenCount;++i)if(ids[i]==id)return false;
        if(count==classes.size() || seenCount==ids.size())return false;
        ids[seenCount++]=id;classes[count]=classId;batches[count++]=batchId;return true;
    }
    // A successful batch may finish before the UI drains its final removals.
    void End() noexcept {batch=false;}
};

// Native stash tabs 2, 3 and 4 are the supported advanced destinations. Each
// confirmed destination owns its own clock. Its first removal in a batch starts
// the animation; later removals cannot restart it, even after expiry/cleanup.
// A new batch can start that destination again. A completed timer stays marked
// until the UI clears its visual state, including its final zero-alpha frame.
struct Timers {
    struct Timer {
        std::uint64_t started{};
        std::uint32_t fadeMs{},cycles{};
        bool marked{};
    };
    std::array<Timer,5> tabs{};
    std::array<std::uint64_t,5> lastBatches{};

    static bool Supported(unsigned tab) noexcept {return tab>=2 && tab<=4;}
    bool Mark(unsigned tab,std::uint64_t batchId,std::uint64_t now,std::uint32_t fadeMs,std::uint32_t cycles) noexcept {
        if(!Supported(tab) || !batchId || fadeMs<100 || fadeMs>2000 || cycles<1 || cycles>10 ||
            lastBatches[tab]==batchId)return false;
        lastBatches[tab]=batchId;
        tabs[tab]={now,fadeMs,cycles,true};return true;
    }
    bool Marked(unsigned tab) const noexcept {return Supported(tab) && tabs[tab].marked;}
    bool Active(unsigned tab,std::uint64_t now) const noexcept {
        if(!Marked(tab))return false;
        const auto& value=tabs[tab];
        return now>=value.started && now-value.started<2ull*value.fadeMs*value.cycles;
    }
    float Alpha(unsigned tab,std::uint64_t now) const noexcept {
        if(!Active(tab,now))return 0.0f;
        const auto& value=tabs[tab];
        const auto phase=(now-value.started)%(2ull*value.fadeMs);
        const auto distance=phase<=value.fadeMs?phase:2ull*value.fadeMs-phase;
        return static_cast<float>(distance)/static_cast<float>(value.fadeMs);
    }
    bool Any() const noexcept {
        for(unsigned tab=2;tab<=4;++tab)if(tabs[tab].marked)return true;
        return false;
    }
    bool NeedsUpdate() const noexcept {return Any();}
    void Clear(unsigned tab) noexcept {if(Supported(tab))tabs[tab]={};}
    void Clear() noexcept {tabs={};lastBatches={};}
};

}
