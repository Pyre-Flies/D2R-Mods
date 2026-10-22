#include "catalog.h"
#include <D2RLPlugin/lifecycle_events.h>
#include <cstdio>
#include <mutex>

namespace GemBag::Catalog {
namespace {
std::mutex guard;
std::vector<std::shared_ptr<const Definition>> definitions;
const D2RL::PluginContext* context{};
const D2RL::LifecycleService* lifecycle{};
D2RL::Lifecycle::ListenerHandle listener{};
bool stopping{};
bool Tables(const D2RL::PluginContext* c,unsigned bank,Views& views,uint64_t revision=0) {
    const D2RL::DataTableService* service{};
    if(c->QueryService(&service)!=D2RL::ServiceQueryResult::Success||!D2RL::HasDataTableServiceField(service,D2RL::DataTableServiceRequiredSize)||!service->getTable)return false;
    for(unsigned i=0;i<views.size();++i) {
        auto& v=views[i];v.structSize=D2RL::DataTables::TableViewSize;
        if(service->getTable(c,static_cast<D2RL::DataTables::Bank>(bank),Ids[i],&v)!=D2RL::DataTables::Result::Success||
            (revision&&v.revision!=revision))return false;
    }
    return true;
}
void __cdecl Loaded(const D2RL::PluginContext* c,const D2RL::Lifecycle::DataTablesLoadedEvent* event,void*) noexcept {
    try {if(D2RL::Lifecycle::HasDataTablesLoadedEventField(event,D2RL::Lifecycle::DataTablesLoadedEventRequiredSize))Refresh(c,event->revision);}
    catch(...) {std::lock_guard lock(guard);definitions.clear();c->LogWarn("[GemBag/Data] Definition discovery failed; bag UI remains inactive.");}
}
}
bool Initialize(const D2RL::PluginContext* c) {
    std::lock_guard lock(guard);context=c;stopping=false;definitions.clear();
    if(c->QueryService(&lifecycle)!=D2RL::ServiceQueryResult::Success||!D2RL::HasLifecycleServiceField(lifecycle,D2RL::LifecycleServiceRequiredSize)||
        !lifecycle->registerDataTablesLoadedListener||!lifecycle->unregisterDataTablesLoadedListener)return false;
    const D2RL::Lifecycle::DataTablesLoadedListener registration{.structSize=D2RL::Lifecycle::DataTablesLoadedListenerSize,.callback=Loaded};
    return lifecycle->registerDataTablesLoadedListener(c,&registration,&listener)==D2RL::Lifecycle::Result::Success&&listener;
}
void Stop() {
    D2RL::Lifecycle::ListenerHandle old{};
    {std::lock_guard lock(guard);stopping=true;old=listener;listener=0;definitions.clear();}
    if(old&&lifecycle)lifecycle->unregisterDataTablesLoadedListener(context,old);
}
bool Refresh(const D2RL::PluginContext* c,uint64_t revision) {
    std::vector<std::shared_ptr<const Definition>> next;
    for(unsigned bank=1;bank<=3;++bank) {
        Views views{};if(!Tables(c,bank,views,revision))continue;
        Definitions found;std::string why;const bool ok=Build(views,bank,found,why);
        char message[256];std::snprintf(message,sizeof(message),"[GemBag/Data] bank=%u revision=%llu bags=%zu: %s.",bank,views[0].revision,found.size(),why.c_str());
        if(ok)c->LogInfo(message);else c->LogWarn(message);
        for(auto& d:found)next.push_back(std::make_shared<const Definition>(std::move(d)));
    }
    std::lock_guard lock(guard);if(stopping)return false;definitions=std::move(next);return !definitions.empty();
}
bool Candidate(const Info& item) {
    std::lock_guard lock(guard);
    return !stopping&&std::any_of(definitions.begin(),definitions.end(),[&](const auto& d){return d->bagClass==item.classId&&d->bagCode==item.code;});
}
bool CandidateClass(unsigned id) {
    std::lock_guard lock(guard);
    return !stopping&&std::any_of(definitions.begin(),definitions.end(),[&](const auto& d){return d->bagClass==id;});
}
std::shared_ptr<const Definition> Find(unsigned bank,const Info& item) {
    std::lock_guard lock(guard);if(stopping)return {};
    for(const auto& d:definitions)if((!bank||d->bank==bank)&&d->bagClass==item.classId&&d->bagCode==item.code)return d;
    return {};
}
bool Recheck(const D2RL::PluginContext* c,const Definition& d) {
    Views views{};if(!Tables(c,d.bank,views,d.revision))return false;
    for(unsigned i=0;i<views.size();++i) {
        const auto& v=views[i];
        if(!D2RL::DataTables::HasTableViewField(&v,D2RL::DataTables::TableViewRequiredSize)||v.tableId!=Ids[i]||static_cast<unsigned>(v.bank)!=d.bank||
            v.rowSize!=Sizes[i]||!v.rows||v.rowCount!=d.counts[i]||v.rowCount>Limits[i])return false;
    }
    for(const auto& w:d.witnesses) {
        const auto found=std::find(Ids.begin(),Ids.end(),w.table);if(found==Ids.end())return false;
        const auto& v=views[static_cast<size_t>(found-Ids.begin())];if(w.row>=v.rowCount||w.bytes.size()!=v.rowSize)return false;
        const auto row=static_cast<const unsigned char*>(v.rows)+static_cast<size_t>(w.row)*v.rowSize;
        if(std::memcmp(row,w.bytes.data(),w.bytes.size()))return false;
    }
    return true;
}
}
