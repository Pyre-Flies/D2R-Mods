#include "portal_policy.h"
#include "portal_calls.h"
#include <cstdio>
#include <cstdlib>

void Check(bool ok, const char* label) {
    if (!ok) { std::fprintf(stderr,"FAIL: %s\n",label); std::exit(1); }
}
int main() {
    // Scope and publication failures: never patch a shared entry, preflight all
    // rel32 displacements, and never report a partial installation as active.
    constexpr uintptr_t base=0x140000000, relay=base+0x4000000;
    for (const auto& site : QolPortalCalls::Calls) {
        int32_t displacement{}; std::memcpy(&displacement,site.expected+1,4);
        Check(site.expected[0]==0xe8 && site.rva+5+displacement==QolPortalCalls::Target,
            "all original calls reach the reviewed shared entry");
        Check(QolPortal::IsCandidateContactCaller(site.rva+5),"relay preserves a whitelisted return site");
        Check(site.rva!=QolPortalCalls::Target,"no shared entry patch");
    }
    for (int fail=-1; fail<3; ++fail) {
        int patched=0;
        const bool installed=QolPortalCalls::Publish(base,relay,
            [&](uintptr_t rva,const unsigned char* expected,const unsigned char* replacement) {
                const auto& site=QolPortalCalls::Calls[patched];
                Check(rva==site.rva && expected==site.expected,"SDK receives exact original CALL guard");
                int32_t displacement{}; std::memcpy(&displacement,replacement+1,4);
                Check(replacement[0]==0xe8 && base+rva+5+displacement==relay,"replacement reaches relay");
                return patched++ != fail;
            });
        Check(installed==(fail==-1),"any patch failure prevents activation");
        Check(patched==(fail==-1 ? 3 : fail+1),"publication stops at first failure");
    }
    int attempted=0;
    Check(!QolPortalCalls::Publish(base,base+0x100000000ULL,
        [&](uintptr_t,const unsigned char*,const unsigned char*) {++attempted;return true;}) && attempted==0,
        "out-of-range relay publishes no patches");
    static_assert(QolPortal::LootOwnsPortalSelection(true,true,357,true));
    static_assert(!QolPortal::LootOwnsPortalSelection(true,false,357,true));
    static_assert(!QolPortal::LootOwnsPortalSelection(true,true,357,false));
    static_assert(!QolPortal::LootOwnsPortalSelection(true,true,1,true));
    static_assert(!QolPortal::LootOwnsPortalSelection(false,true,357,true));

    using namespace QolPortal;
    constexpr unsigned char originalPrefix[5]={0x48,0x83,0xec,0x28,0x48};
    constexpr unsigned char detourPrefix[5]={0xe9,0x70,0x77,0xca,0xff};
    constexpr unsigned char unknownPrefix[5]={0xff,0x25,0,0,0};
    Check(AdmitSharedEntryPrefix(originalPrefix,originalPrefix,false),"original shared entry admitted");
    Check(AdmitSharedEntryPrefix(detourPrefix,originalPrefix,true),"executable E9 owner admitted");
    Check(!AdmitSharedEntryPrefix(detourPrefix,originalPrefix,false),"nonexecutable E9 target refused");
    Check(!AdmitSharedEntryPrefix(unknownPrefix,originalPrefix,true),"unknown detour form refused");
    static_assert(ClassifyPriorityObject(59,0x04)==PriorityKind::Portal);
    static_assert(ClassifyPriorityObject(267,0)==PriorityKind::Stash);
    static_assert(ClassifyPriorityObject(119,0x40)==PriorityKind::Waypoint);
    static_assert(ClassifyPriorityObject(159,0)==PriorityKind::None); // hidden stash is not town storage
    static_assert(ClassifyPriorityObject(2,0x01)==PriorityKind::Shrine);
    static_assert(ClassifyPriorityObject(111,0)==PriorityKind::Well);
    static_assert(ClassifyPriorityObject(130,0x20)==PriorityKind::Well);
    static_assert(ClassifyPriorityObject(322,0x20)==PriorityKind::Well);
    static_assert(ClassifyPriorityObject(519,0)==PriorityKind::Well);
    static_assert(ClassifyPriorityObject(5,0x08)==PriorityKind::Chest);
    static_assert(ClassifyPriorityObject(354,0x08)==PriorityKind::None); // quest chest excluded
    static_assert(ClassifyPriorityObject(416,0x08)==PriorityKind::None); // StoneStash excluded
    static_assert(KindEnabled(PriorityKind::Portal,true,false,false));
    static_assert(KindEnabled(PriorityKind::Stash,false,true,false));
    static_assert(KindEnabled(PriorityKind::Waypoint,false,false,true));
    static_assert(KindEnabled(PriorityKind::Shrine,false,false,false,true,false));
    static_assert(KindEnabled(PriorityKind::Well,false,false,false,true,false));
    static_assert(!KindEnabled(PriorityKind::Well,true,true,true,false,true));
    static_assert(KindEnabled(PriorityKind::Chest,false,false,false,false,true));
    static_assert(!KindEnabled(PriorityKind::Chest,true,true,true,true,false));
    static_assert(!KindEnabled(PriorityKind::Stash,true,false,true));
    // A portal rejected BEFORE comparison must be admitted by native scoring.
    // Model only its two independent gates: center distance and facing angle.
    unsigned char native[ScoringControllerBytes]{}, view[ScoringControllerBytes]{};
    for (int profile : {0,7,8}) {
        const auto offset = ScoringProfileOffset(profile);
        const float six = 6;
        std::memcpy(native + offset + 0x68, &six, sizeof(six));
        native[offset+0x30] = 2; // angle curve retained
        native[offset+0xC8] = 1; // weight retained
        float oldLimit=0,newLimit=0;
        Check(PrepareScoringView(view,native,profile,10,oldLimit,newLimit),"candidate beyond original scoring cutoff gets scratch profile");
        auto score=[](float distance,float limit,bool angle) { return distance>limit || !angle ? -1.0f : 1.0f-distance/limit; };
        Check(score(10,oldLimit,true)<0 && score(10,newLimit,true)>=0,"expanded candidate reaches later comparison");
        Check(score(10,newLimit,false)<0,"native angle rejection retained");
        float unchanged=0;
        std::memcpy(&unchanged,native+offset+0x68,4);
        Check(unchanged==6,"shared native profile never mutated");
        for (size_t i=0;i<ScoringProfileBytes;++i) {
            if (i>=0x68 && i<0x6C) continue;
            Check(view[offset+i]==native[offset+i],"every other profile field preserved");
        }
        Check(!PrepareScoringView(view,native,profile,5,oldLimit,newLimit),"in-range angular failure is not rescored");
        Check(!PrepareScoringView(view,native,profile,std::numeric_limits<float>::infinity(),oldLimit,newLimit),"nonfinite position rejected");
    }
    float oldLimit=0,newLimit=0;
    Check(!PrepareScoringView(view,native,-1,10,oldLimit,newLimit),"negative profile fails closed");
    Check(!PrepareScoringView(view,native,9,10,oldLimit,newLimit),"invalid profile fails closed");
    Check(ReadPriorityDistance("[qol]\nground_pickup_distance = 6\n") == 10,"old configs default to ten independently of loot range");
    Check(ReadPriorityDistance("portal_priority_distance = 15 # custom\n") == 15,"custom range parsed");
    Check(ReadPriorityDistance("# portal_priority_distance = 3\nportal_priority_distance = 10\n") == 10,"comment ignored");
    Check(ReadPriorityDistance("portal_priority_distance = invalid\nground_pickup_distance = 6") == 10,"invalid value cannot read next setting");
    Check(ReadPriorityDistance("portal_priority_distance = -2") == 1,"low range clamped");
    Check(ReadPriorityDistance("portal_priority_distance = 99") == 20,"high range clamped");
    Check(PreferPortal(true,false,357,4,2,4,10,10),"ten-unit boundary included");
    Check(!PreferPortal(true,false,357,4,2,4,11,10),"outside ten unchanged");
    Check(PreferPortal(true,false,357,4,2,4,6,6),"six-unit boundary included");
    Check(!PreferPortal(true,false,357,4,2,4,7,6),"outside radius unchanged");
    Check(!PreferPortal(true,false,357,4,2,4,-1,6),"invalid distance unchanged");
    Check(!PreferPortal(true,true,357,4,2,4,2,6),"LB loot unchanged");
    Check(!PreferPortal(false,false,357,4,2,4,2,6),"disabled unchanged");
    Check(!PreferPortal(true,false,358,4,2,4,2,6),"dedicated Loot skill unchanged");
    Check(!PreferPortal(true,false,370,4,2,4,2,6),"dedicated CubeLoot skill unchanged");
    Check(!PreferPortal(true,false,0,4,2,4,2,6),"attack skill unchanged");
    Check(!PreferPortal(true,false,357,1,2,4,2,6),"NPC/monster target unchanged");
    Check(!PreferPortal(true,false,357,4,2,16,2,6),"ordinary nonportal object unchanged");
    Check(PreferObject(true,false,357,4,PriorityKind::Stash,6,6),"stash beats loot at boundary");
    Check(PreferObject(true,false,357,4,PriorityKind::Waypoint,6,6),"waypoint beats loot at boundary");
    Check(PreferObject(true,false,357,4,PriorityKind::Shrine,6,6),"shrine beats loot at boundary");
    Check(PreferObject(true,false,357,4,PriorityKind::Well,6,6),"well beats loot at boundary");
    Check(PreferObject(true,false,357,4,PriorityKind::Chest,6,6),"enabled normal chest beats loot at boundary");
    Check(!PreferObject(true,true,357,4,PriorityKind::Stash,2,6),"LB loot retains ownership over stash");
    Check(!PreferObject(true,false,357,4,PriorityKind::None,2,6),"ordinary object is not promoted");
    Check(!PreferObject(true,false,357,2,PriorityKind::Waypoint,2,6),"object versus object retains native ranking");
    int item=1, portal=2; void* selected=&item; float best=0.9f;
    // Regression: Interact accepted the chest, but later bound-skill arbitration
    // discarded it when a combat target was present. Never recover loot here.
    {
        int queries=0, checks=0;
        void* queried=&portal;
        PriorityKind kind=PriorityKind::Chest;
        int distance=0;
        auto query=[&]() -> void* { ++queries; return queried; };
        auto eligible=[&](void*) { ++checks; return kind!=PriorityKind::None && WithinPriorityDistance(distance,10); };
        Check(RecoverInteraction(true,true,false,nullptr,query,eligible)==&portal && queries==1 && checks==1,
            "accepted nearby chest restored after null combat arbitration");
        for (auto family : {PriorityKind::Portal,PriorityKind::Stash,PriorityKind::Waypoint,PriorityKind::Shrine,PriorityKind::Well,PriorityKind::Chest}) {
            kind=family; distance=10;
            Check(RecoverInteraction(true,true,false,nullptr,query,eligible)==&portal,"enabled priority families recover at inclusive boundary");
        }
        for (int outside : {-1,11}) {
            distance=outside;
            Check(!RecoverInteraction(true,true,false,nullptr,query,eligible),"invalid or distant object keeps combat fallback");
        }
        distance=0; kind=PriorityKind::None;
        Check(!RecoverInteraction(true,true,false,nullptr,query,eligible),"loot, unsupported and disabled object kinds never recovered");
        kind=PriorityKind::Chest;
        queries=checks=0;
        Check(RecoverInteraction(true,true,false,&item,query,eligible)==&item && !queries && !checks,"existing native result not replaced or requeried");
        Check(!RecoverInteraction(false,true,false,nullptr,query,eligible) && !queries,"shutdown or failed admission remains native");
        Check(!RecoverInteraction(true,false,false,nullptr,query,eligible) && !queries,"mouse UI does not query or recover interactions");
        Check(!RecoverInteraction(true,true,true,nullptr,query,eligible) && !queries,"held pickup modifier preserves native arbitration");
        queried=nullptr;
        Check(!RecoverInteraction(true,true,false,nullptr,query,eligible) && queries==1 && !checks,"native selection rejection never fabricates a target");
    }
    int calls=0;
    auto accept=[&](float score) { ++calls; if (score>best) {best=score;selected=&portal;} };
    Check(EvaluatePortal(true,&portal,0.2f,best,selected,accept),"lower-score portal beats item");
    Check(calls==1 && selected==&portal && best==0.2f,"one native call and original portal score restored");
    selected=&item;best=0.9f;calls=0;
    Check(!EvaluatePortal(true,&portal,0.2f,best,selected,[&](float){++calls;}),"native rejection respected");
    Check(calls==1 && selected==&item && best==0.9f,"rejected portal does not block pickup");
    Check(!EvaluatePortal(false,&portal,0.2f,best,selected,accept) && selected==&item,"ordinary ranking retained");
    Check(EvaluatePortal(true,&portal,0.9f,best,selected,accept) && selected==&portal,"tie prefers portal");
    // Regression: TOML parsing alone is insufficient. The native comparison
    // can reject an otherwise higher-scoring portal at its old distance gate.
    int player=3, other=4;
    RangeOverride pending;
    for (int radius : {6,10,20}) {
        for (int distance : {6,7,10,11,20,21}) {
            selected=&item; best=0.9f; calls=0;
            RangeOverride qualified;
            if (WithinPriorityDistance(distance,radius))
                qualified={&player,&portal,distance,static_cast<uint32_t>(radius)};
            {
                RangeScope scope(pending,qualified);
                EvaluatePortal(true,&portal,0.2f,best,selected,[&](float score) {
                    ++calls;
                    const bool nativeRange = distance <= 6;
                    if ((nativeRange || pending.Allows(true,&player,&portal)) && score>best) {
                        selected=&portal; best=score;
                    }
                });
            }
            Check(calls==1,"distance fix forwards native comparison once");
            Check((selected==&portal)==(distance<=radius),"configured radius crosses native six-unit gate including boundary");
            Check(!pending.Allows(true,&player,&portal),"no override survives native comparison");
        }
    }
    {
        RangeScope outer(pending,{&player,&portal,20,20});
        Check(pending.Allows(true,&player,&portal),"portal qualifies even when it is first candidate");
        Check(!pending.Allows(false,&player,&portal),"shutdown disables range override");
        Check(!pending.Allows(true,&other,&portal),"different player keeps native range");
        Check(!pending.Allows(true,&player,&item),"different candidate keeps native range");
        {
            RangeScope nested(pending,{});
            Check(!pending.Allows(true,&player,&portal),"nested ordinary comparison masks override");
        }
        Check(pending.Allows(true,&player,&portal),"nested scope restores parent");
    }
    Check(!RangeOverride{&player,&portal,-1,20}.Allows(true,&player,&portal),"invalid native distance never overrides");
    // Runtime regression: scoring was rescued at distance 6, but the object
    // contact predicate discarded it before Interact could compare it to loot.
    for (uintptr_t caller : {uintptr_t(0x19158E),uintptr_t(0x1922D2),uintptr_t(0x19237D)}) {
        Check(ExtendCandidateContact(true,false,caller,true,6,20),"candidate contact gate uses configured range");
        Check(ExtendCandidateContact(true,false,caller,true,20,20),"contact radius boundary included");
        Check(!ExtendCandidateContact(true,false,caller,true,21,20),"contact radius boundary enforced");
        Check(!ExtendCandidateContact(true,true,caller,true,6,20),"modified looting keeps native contact");
        Check(!ExtendCandidateContact(true,false,caller,false,6,20),"nonportal object keeps native contact");
        Check(!ExtendCandidateContact(false,false,caller,true,6,20),"disabled hook keeps native contact");
        Check(!ExtendCandidateContact(true,false,caller,true,-1,20),"invalid distance keeps native contact");
        selected=&item; best=0.9f; calls=0;
        if (ExtendCandidateContact(true,false,caller,true,6,20)) {
            RangeScope scope(pending,{&player,&portal,6,20});
            EvaluatePortal(true,&portal,0.292f,best,selected,[&](float score) {
                ++calls;
                if (pending.Allows(true,&player,&portal) && score>best) { selected=&portal;best=score; }
            });
        }
        Check(calls==1 && selected==&portal,"rescored portal survives contact gate and beats item");
    }
    for (uintptr_t caller : {uintptr_t(0),uintptr_t(0x19158D),uintptr_t(0x1922D3),uintptr_t(0x34BD19)})
        Check(!ExtendCandidateContact(true,false,caller,true,6,20),"unlisted callers including actual object-use paths retain native contact");
    std::puts("Portal range, modifiers, native eligibility and comparison policy passed.");
}
