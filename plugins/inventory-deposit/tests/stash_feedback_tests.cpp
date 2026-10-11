#include "stash_feedback.h"
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <string>
#include <vector>

namespace {
namespace Feedback=InventoryDepositStashFeedback;
using Service=D2RL::ServiceId;
using Kind=D2RL::Lifecycle::GameplayEventKind;
void Check(bool ok,const char* why) {
    if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}
}
struct Task {
    const D2RL::PluginContext* context{};
    D2RL::Threads::Callback callback{};
    void* data{};
};
struct Write {void* tabs;unsigned tab;bool enable;float alpha;bool isAlpha;};
struct Fixture;
Fixture* fixture{};
struct Fixture {
    D2RL::PluginApi api{};
    D2RL::PluginContext context{};
    D2RL::ThreadService threads{};
    D2RL::WidgetService widgets{};
    D2RL::LifecycleService lifecycle{};
    std::map<std::uint64_t,D2RL::Lifecycle::GameplayEventListener> listeners;
    std::deque<Task> tasks;
    std::vector<Write> writes;
    std::vector<std::uint32_t> classified;
    std::vector<std::string> warnings;
    std::map<std::uint32_t,std::uint8_t> categories{{20,std::uint8_t{1}},{30,std::uint8_t{2}},{40,std::uint8_t{3}}};
    Feedback::Detail::Frame frame{reinterpret_cast<void*>(0x1000),reinterpret_cast<void*>(0x2000),5,100,1,true};
    Service missing=static_cast<Service>(0);
    bool uiThread{},observerThread{},admitted=true,readable=true,rejectQueue{};
    unsigned queues{},reads{},admissions{},registrationAttempts{},failRegistration{};
    std::uint64_t now{1000},nextListener{1};

    Fixture();
    ~Fixture() {
        uiThread=true;Feedback::Shutdown();uiThread=false;
        Check(listeners.empty(),"shutdown unregisters feedback session listeners");fixture=nullptr;
    }
    bool Start() {return Feedback::Initialize(&context);}
    void Run(const Task& task) {
        Check(!uiThread && !observerThread,"UI callbacks run separately from observers");
        uiThread=true;task.callback(task.context,task.data);uiThread=false;
    }
    void Pump() {
        unsigned budget=32;
        while(!tasks.empty()) {
            Check(budget--!=0,"a refresh does not reschedule endlessly inside the UI pump");
            const auto task=tasks.front();tasks.pop_front();Run(task);
        }
    }
    void Tick(std::uint64_t time) {now=time;Feedback::Refresh();Pump();}
    void Begin() {observerThread=true;Feedback::BeginBatch();observerThread=false;}
    void Removed(std::uint32_t id,std::uint32_t classId) {
        D2RL::Items::ItemInfo info{};info.structSize=D2RL::Items::ItemInfoSize;
        info.runtimeId=id;info.classId=classId;info.container=D2RL::Items::ItemContainer::Inventory;
        observerThread=true;Feedback::ItemRemoved(info);observerThread=false;
    }
    void End() {observerThread=true;Feedback::EndBatch();observerThread=false;}
    void Session(Kind kind) {
        const auto copy=listeners;
        D2RL::Lifecycle::GameplayEvent event{};event.structSize=sizeof(event);event.kind=kind;
        uiThread=true;
        for(const auto& [id,entry]:copy) {
            (void)id;if(entry.kind==kind)entry.callback(&context,&event,entry.userData);
        }
        uiThread=false;
    }
    unsigned Enables(unsigned tab,bool enabled) const {
        unsigned count{};for(const auto& write:writes)if(!write.isAlpha && write.tab==tab && write.enable==enabled)++count;
        return count;
    }
    bool HasAlpha(unsigned tab,float alpha) const {
        for(const auto& write:writes)if(write.isAlpha && write.tab==tab && write.alpha==alpha)return true;
        return false;
    }
    float LastAlpha(unsigned tab) const {
        for(auto it=writes.rbegin();it!=writes.rend();++it)
            if(it->isAlpha && it->tab==tab)return it->alpha;
        return -1.0f;
    }
    unsigned NativeCalls() const {return reads+static_cast<unsigned>(classified.size()+writes.size());}
};

D2RL::ServiceQueryResult __cdecl Query(const D2RL::PluginContext*,Service id,std::uint32_t version,const void** value) noexcept {
    Check(!fixture->observerThread,"batch callbacks never request services");
    Check(value && version==1,"feedback requests the supported SDK service ABI");*value=nullptr;
    if(id==fixture->missing)return D2RL::ServiceQueryResult::Unavailable;
    switch(id) {
        case Service::Thread:*value=&fixture->threads;break;
        case Service::Widget:*value=&fixture->widgets;break;
        case Service::Lifecycle:*value=&fixture->lifecycle;break;
        default:return D2RL::ServiceQueryResult::UnknownService;
    }
    return D2RL::ServiceQueryResult::Success;
}
D2RL::Threads::Result __cdecl Queue(const D2RL::PluginContext* context,D2RL::Threads::Callback callback,void* data) noexcept {
    Check(!fixture->observerThread,"game-thread observers only copy POD; the refresh worker owns scheduling");
    Check(callback!=nullptr,"feedback task has a valid UI callback");++fixture->queues;
    if(fixture->rejectQueue)return D2RL::Threads::Result::Unavailable;
    fixture->tasks.push_back({context,callback,data});return D2RL::Threads::Result::Success;
}
D2RL::Lifecycle::Result __cdecl Register(const D2RL::PluginContext*,const D2RL::Lifecycle::GameplayEventListener* listener,std::uint64_t* handle) noexcept {
    Check(listener && listener->structSize>=D2RL::Lifecycle::GameplayEventListenerRequiredSize && listener->callback && handle,
        "session registration honors the SDK contract");
    Check(listener->kind==Kind::GameJoined || listener->kind==Kind::GameLeft,"only session boundaries subscribe");
    if(++fixture->registrationAttempts==fixture->failRegistration)return D2RL::Lifecycle::Result::Unavailable;
    *handle=fixture->nextListener++;fixture->listeners.emplace(*handle,*listener);return D2RL::Lifecycle::Result::Success;
}
D2RL::Lifecycle::Result __cdecl Unregister(const D2RL::PluginContext*,std::uint64_t handle) noexcept {
    Check(fixture->listeners.erase(handle)==1,"only owned session registrations are released");
    return D2RL::Lifecycle::Result::Success;
}
D2RL::Widgets::Result __cdecl FindPanel(const D2RL::PluginContext*,const char*,std::uint64_t*) noexcept {
    Check(false,"test native frame facade owns widget lookup");return D2RL::Widgets::Result::Unavailable;
}
D2RL::Widgets::Result __cdecl FindWidget(const D2RL::PluginContext*,std::uint64_t,const char*,std::uint64_t*) noexcept {
    Check(false,"test native frame facade owns widget lookup");return D2RL::Widgets::Result::Unavailable;
}
D2RL::Widgets::Result __cdecl GetRect(const D2RL::PluginContext*,std::uint64_t,D2RL::Widgets::Rect*) noexcept {
    Check(false,"test native frame facade owns widget lookup");return D2RL::Widgets::Result::Unavailable;
}
void __cdecl Warn(const D2RL::PluginContext*,const char* message) noexcept {
    if(message)fixture->warnings.emplace_back(message);
}
Fixture::Fixture() {
    Check(fixture==nullptr,"fixtures execute serially");fixture=this;
    api.apiSize=sizeof(api);api.queryService=Query;api.logWarn=Warn;
    context.contextSize=sizeof(context);context.api=&api;context.exeBase=0x400000;
    threads.serviceSize=sizeof(threads);threads.serviceVersion=1;threads.runOnUiThread=Queue;
    widgets.serviceSize=sizeof(widgets);widgets.serviceVersion=1;
    widgets.findPanel=FindPanel;widgets.findWidget=FindWidget;widgets.getWidgetRect=GetRect;
    lifecycle.serviceSize=sizeof(lifecycle);lifecycle.serviceVersion=1;
    lifecycle.registerGameplayEventListener=Register;lifecycle.unregisterGameplayEventListener=Unregister;
}

void AdmissionTests() {
    {Fixture f;Check(!Feedback::Initialize(nullptr),"null context disables only optional feedback");}
    for(const auto service:{Service::Thread,Service::Widget,Service::Lifecycle}) {
        Fixture f;f.missing=service;Check(!f.Start() && f.listeners.empty(),"missing SDK service rolls back feedback registration");
        f.Begin();f.Removed(1,20);f.End();f.Tick(1000);Check(f.NativeCalls()==0,"unsupported feedback ignores batch callbacks");
    }
    for(unsigned invalid=0;invalid<6;++invalid) {
        Fixture f;
        std::uint32_t* sizes[]{&f.threads.serviceSize,&f.widgets.serviceSize,&f.lifecycle.serviceSize};
        std::uint32_t* versions[]{&f.threads.serviceVersion,&f.widgets.serviceVersion,&f.lifecycle.serviceVersion};
        if(invalid<3)*sizes[invalid]=8;else *versions[invalid-3]=99;
        Check(!f.Start() && f.listeners.empty(),"incomplete or incompatible SDK ABI refuses feedback cleanly");
    }
    for(unsigned failure=1;failure<=2;++failure) {
        Fixture f;f.failRegistration=failure;
        Check(!f.Start() && f.listeners.empty(),"partial session listener registration rolls back");
    }
    for(unsigned missing=0;missing<6;++missing) {
        Fixture f;
        switch(missing) {
            case 0:f.widgets.findPanel=nullptr;break;
            case 1:f.widgets.findWidget=nullptr;break;
            case 2:f.widgets.getWidgetRect=nullptr;break;
            case 3:f.threads.runOnUiThread=nullptr;break;
            case 4:f.lifecycle.registerGameplayEventListener=nullptr;break;
            case 5:f.lifecycle.unregisterGameplayEventListener=nullptr;break;
        }
        Check(!f.Start() && f.listeners.empty(),"missing SDK operation cleanly refuses only optional feedback");
    }
    {Fixture f;f.admitted=false;Check(!f.Start() && f.listeners.empty(),"failed native guard prevents feedback admission");
        f.Begin();f.Removed(1,20);f.End();f.Tick(1000);Check(f.writes.empty(),"guard refusal performs no native visual writes");}
}

void ConfirmationTests() {
    Fixture f;Check(f.Start(),"complete SDK and native facade admit feedback");
    f.Removed(99,20);f.Tick(1000);Check(f.classified.empty(),"unowned observations cannot flash stash tabs");
    f.Begin();f.Removed(1,20);f.Removed(2,30);f.Removed(3,40);f.Removed(1,20);f.End();
    Check(f.NativeCalls()==0 && f.tasks.empty(),"all observer callbacks are POD-only and do not touch native UI or queue UI work");
    f.Tick(1000);
    Check(f.classified==std::vector<std::uint32_t>({20,30,40}),"each confirmed identity is classified once on the UI thread");
    for(unsigned tab=2;tab<=4;++tab)Check(f.Enables(tab,true)>0,"all affected destinations receive independent native highlights");
    f.Tick(1100);
    for(unsigned tab=2;tab<=4;++tab)Check(f.HasAlpha(tab,1.0f),"each affected tab reaches visible full-alpha feedback");
    f.Tick(1200);
    for(unsigned tab=2;tab<=4;++tab)Check(f.Enables(tab,false)>0,"completed custom highlights are cleared independently");
    const auto calls=f.NativeCalls();f.Tick(1250);Check(f.NativeCalls()==calls,"fully drained feedback stops scheduling native work");
}

void UnknownCategoryTests() {
    Fixture f;Check(f.Start(),"unknown category fixture starts");
    f.categories={{1,std::uint8_t{0}},{2,std::uint8_t{4}},{3,std::uint8_t{5}},{4,std::uint8_t{255}}};
    f.Begin();for(unsigned i=1;i<=4;++i)f.Removed(i,i);f.End();f.Tick(1000);
    Check(f.classified.size()==4 && f.writes.empty(),"unknown and ordinary categories do not guess a destination or change its tab");
}

void OncePerBatchTests() {
    Fixture f;Check(f.Start(),"once-per-batch fixture starts");
    f.frame.cycles=3;
    for(std::uint32_t classId=30;classId<36;++classId)f.categories[classId]=2;
    f.Begin();f.Removed(1,30);f.Tick(1000);
    Check(f.LastAlpha(3)==0.0f,"first confirmed material starts the existing animation");
    f.Removed(2,31);f.Tick(1100);
    Check(f.LastAlpha(3)==1.0f,"a different material class does not reset the first rise");
    f.Removed(3,32);f.Tick(1250);
    Check(f.LastAlpha(3)==0.5f,"a later distinct material preserves the second cycle's phase");
    f.Removed(4,33);f.Tick(1400);
    Check(f.LastAlpha(3)==0.0f,"further materials retain the original third-cycle boundary");
    f.Tick(1500);
    Check(f.LastAlpha(3)==1.0f && f.Enables(3,false)==0,
        "once-per-batch retains all three configured cycles instead of becoming a single fade");
    f.Tick(1600);
    Check(f.Enables(3,false)==1,"the existing animation ends at its original expiry");
    const auto writesAfterExpiry=f.writes.size();
    f.Removed(5,34);f.Tick(1700);f.Removed(6,35);f.Tick(1900);f.End();
    Check(f.classified==std::vector<std::uint32_t>({30,31,32,33,34,35}) &&
        f.writes.size()==writesAfterExpiry,
        "all six material classes are confirmed but late same-batch items cannot replay an expired animation");
    const auto calls=f.NativeCalls();f.Tick(2100);
    Check(f.NativeCalls()==calls,"an expired dedupe record does not keep scheduling native work");
    const auto enables=f.Enables(3,true);
    f.Begin();f.Removed(1,30);f.Tick(2200);f.End();
    Check(f.Enables(3,true)==enables+1 && f.LastAlpha(3)==0.0f,
        "a new batch can flash the same destination again even when a runtime identity is reused");
    f.Tick(2300);Check(f.LastAlpha(3)==1.0f,"the new batch owns a fresh animation clock");
    f.Tick(2800);Check(f.Enables(3,false)==2,"the new batch completes all configured cycles and cleans up");
}

void IndependentBatchTests() {
    Fixture f;Check(f.Start(),"independent batch fixture starts");
    f.Begin();f.Removed(1,20);f.Tick(1000);
    f.Removed(2,30);f.Tick(1050);f.Removed(3,40);f.Tick(1100);
    f.Removed(4,20);f.Removed(5,30);f.Removed(6,40);f.Tick(1150);f.End();
    Check(f.LastAlpha(2)==0.5f && f.LastAlpha(3)==1.0f && f.LastAlpha(4)==0.5f,
        "later same-batch items preserve each destination's independent first-confirmation phase");
    f.Begin();f.Removed(1,20);f.Tick(1175);f.End();
    Check(f.LastAlpha(2)==0.0f && f.LastAlpha(3)==0.75f && f.LastAlpha(4)==0.75f,
        "a new batch restarts only its affected tab while other tab animations continue");
    f.Tick(1250);
    Check(f.Enables(3,false)==1 && f.Enables(2,false)==0 && f.Enables(4,false)==0,
        "materials still expires on its original clock after a new gem batch");
    f.Tick(1300);
    Check(f.Enables(4,false)==1 && f.Enables(2,false)==0,
        "runes also expires independently of the restarted gem animation");
    f.Tick(1375);Check(f.Enables(2,false)==1,"the new gem batch receives its own final cleanup");
}

void CancellationTests() {
    for(const auto kind:{Kind::GameJoined,Kind::GameLeft}) {
        Fixture f;Check(f.Start(),"session fixture starts");f.Begin();f.Removed(1,20);f.End();Feedback::Refresh();
        Check(!f.tasks.empty(),"feedback work is queued before session transition");
        f.Session(kind);f.Pump();f.Tick(1050);
        Check(f.classified.empty() && f.writes.empty(),"queued old-session confirmations are invalidated before native work");
        f.Begin();f.Removed(1,30);f.End();f.Tick(1100);
        Check(f.Enables(3,true)>0,"fresh-session batch can reuse an old runtime identity");
    }
    {Fixture f;Check(f.Start(),"close fixture starts");f.Begin();f.Removed(1,20);f.End();
        f.frame.open=false;f.Tick(1000);f.frame.open=true;f.Tick(1100);
        Check(f.writes.empty(),"closing the stash cancels pending confirmations instead of replaying after reopen");}
    {Fixture f;Check(f.Start(),"active session cleanup fixture starts");f.Begin();f.Removed(1,20);f.End();f.Tick(1000);
        const auto calls=f.NativeCalls();f.Session(Kind::GameLeft);
        Check(f.NativeCalls()==calls,"session listeners invalidate state without directly calling native widget operations");
        f.Tick(1050);Check(f.Enables(2,false)>0,"next UI refresh clears custom highlights after a session boundary");
        f.Begin();f.Removed(1,20);f.End();f.Tick(1100);
        Check(f.Enables(2,true)==2 && f.LastAlpha(2)==0.0f,
            "session cancellation resets dedupe so the same tab and runtime identity can start fresh");}
    {Fixture f;Check(f.Start(),"session cleanup with fresh batch fixture starts");f.Begin();f.Removed(1,20);f.End();f.Tick(1000);
        f.Session(Kind::GameLeft);f.Begin();f.Removed(2,30);f.End();f.Tick(1050);
        Check(f.Enables(2,false)>0 && f.Enables(3,true)>0 && f.classified==std::vector<std::uint32_t>({20,30}),
            "deferred old-session cleanup preserves a fresh batch confirmed before the next UI callback");}
    {Fixture f;Check(f.Start(),"shutdown fixture starts");f.Begin();f.Removed(1,20);f.End();Feedback::Refresh();
        Feedback::Shutdown();Check(f.listeners.empty(),"shutdown releases listeners before old tasks run");f.Pump();
        Check(f.NativeCalls()==0,"queued feedback does not retain usable native pointers after shutdown");
        Check(f.Start(),"feedback can initialize again after shutdown");f.Begin();f.Removed(2,30);f.End();f.Tick(1100);
        Check(f.Enables(3,true)>0,"new plugin generation accepts new feedback independently");}
    {Fixture f;Check(f.Start(),"stale owner fixture starts");f.Begin();f.Removed(1,20);f.End();Feedback::Refresh();
        const auto staleTask=f.tasks.front();f.tasks.clear();const auto staleListener=f.listeners.begin()->second;
        Feedback::Shutdown();Check(f.Start(),"new owner generation registers after stale work was captured");
        f.Begin();f.Removed(2,30);f.End();Feedback::Refresh();f.Run(staleTask);
        D2RL::Lifecycle::GameplayEvent event{};event.structSize=sizeof(event);event.kind=staleListener.kind;
        f.uiThread=true;staleListener.callback(&f.context,&event,staleListener.userData);f.uiThread=false;f.Pump();
        Check(f.classified==std::vector<std::uint32_t>{30} && f.Enables(3,true)>0,
            "old callbacks cannot classify old items or cancel the new plugin owner's pending work");}
}

void RapidBatchTests() {
    {Fixture f;Check(f.Start(),"rapid batch fixture starts");
        f.Begin();f.Removed(1,20);f.End();f.Begin();f.Removed(1,40);f.End();
        Feedback::Refresh();Feedback::Refresh();Feedback::Refresh();
        Check(f.tasks.size()==1,"multiple refresh requests coalesce into one outstanding UI task");f.Pump();
        Check(f.classified==std::vector<std::uint32_t>({20,40}) && f.Enables(2,true)>0 && f.Enables(4,true)>0,
            "rapid new batches preserve undrained previous confirmations and dedupe independently");}
    {Fixture f;Check(f.Start(),"partially drained batch fixture starts");
        f.Begin();f.Removed(1,20);f.Tick(1000);f.Removed(2,30);f.End();
        f.Begin();f.Removed(1,40);f.Tick(1100);
        Check(f.classified==std::vector<std::uint32_t>({20,30,40}) &&
            f.LastAlpha(2)==1.0f && f.LastAlpha(3)==0.0f && f.LastAlpha(4)==0.0f,
            "a new batch preserves an old undrained destination without disturbing an already running tab");
        f.Removed(2,30);f.Tick(1150);f.End();
        Check(f.LastAlpha(2)==0.5f && f.LastAlpha(3)==0.0f && f.LastAlpha(4)==0.5f,
            "retained old confirmations keep their batch identity so the new batch can restart that destination");
        f.Tick(1300);
        Check(f.Enables(4,false)==1 && f.Enables(3,false)==0,
            "rapid-batch compaction preserves the separate rune and restarted material expiry times");
        f.Tick(1350);Check(f.Enables(3,false)==1,"the new material batch expires once");}
}

void SchedulingAndGuardTests() {
    {Fixture f;Check(f.Start(),"scheduler failure fixture starts");f.Begin();f.Removed(1,20);f.End();
        f.rejectQueue=true;f.Tick(1000);f.rejectQueue=false;f.Tick(1050);
        Check(f.classified.empty() && f.writes.empty(),"failed UI scheduling discards stale confirmations");
        f.Begin();f.Removed(2,30);f.End();f.Tick(1100);
        Check(f.Enables(3,true)>0,"scheduling failure is isolated and a later batch can display feedback");}
    {Fixture f;Check(f.Start(),"runtime guard fixture starts");f.Begin();f.Removed(1,20);f.End();
        f.admitted=false;f.Tick(1000);f.admitted=true;f.Tick(1100);
        Check(f.writes.empty() && f.classified.empty(),"changed native guard discards pending work without calling native classification or widgets");}
    {Fixture f;Check(f.Start(),"scheduler cleanup with fresh batch fixture starts");f.Begin();f.Removed(1,20);f.End();f.Tick(1000);
        f.rejectQueue=true;f.Tick(1050);f.Begin();f.Removed(2,30);f.End();f.rejectQueue=false;f.Tick(1100);
        Check(f.Enables(2,false)>0 && f.Enables(3,true)>0 && f.classified==std::vector<std::uint32_t>({20,30}),
            "scheduler recovery clears old visuals without discarding confirmations from a newly accepted batch");}
    {Fixture f;Check(f.Start(),"native tab ownership fixture starts");f.frame.nativeTab=3;
        f.Begin();f.Removed(1,20);f.Removed(2,30);f.End();f.Tick(1000);f.Tick(1200);
        Check(f.Enables(2,true)>0 && f.Enables(2,false)>0 && f.Enables(3,false)==0,
            "custom cleanup leaves the current native flash tab under native ownership");}
}
}

namespace InventoryDepositStashFeedback::Detail {
bool Admit(const D2RL::PluginContext* context) noexcept {
    Check(!fixture->observerThread,"native guard work is forbidden in a batch observer");
    Check(context==&fixture->context,"native admission uses the owning plugin context");++fixture->admissions;
    return fixture->admitted;
}
bool ReadFrame(const D2RL::PluginContext* context,const D2RL::WidgetService* widgets,Frame& output) noexcept {
    Check(fixture->uiThread && !fixture->observerThread,"native frame resolution occurs only on the UI thread");
    Check(context==&fixture->context && widgets==&fixture->widgets,"UI resolution uses current owning services");
    ++fixture->reads;if(!fixture->readable)return false;output=fixture->frame;return true;
}
std::uint8_t Category(const D2RL::PluginContext*,std::uint32_t classId) noexcept {
    Check(fixture->uiThread && !fixture->observerThread,"item classification runs only on the UI thread");
    fixture->classified.push_back(classId);
    const auto found=fixture->categories.find(classId);return found==fixture->categories.end()?255:found->second;
}
void Enable(const D2RL::PluginContext*,void* tabs,unsigned tab,bool enabled) noexcept {
    Check(fixture->uiThread && !fixture->observerThread,"native highlight enable writes run only on the UI thread");
    Check(tabs==fixture->frame.tabs && tab>=2 && tab<=4,"native writes resolve the current tab object and a supported destination");
    fixture->writes.push_back({tabs,tab,enabled,0,false});
}
void Alpha(const D2RL::PluginContext*,void* tabs,unsigned tab,float alpha) noexcept {
    Check(fixture->uiThread && !fixture->observerThread,"native highlight alpha writes run only on the UI thread");
    Check(tabs==fixture->frame.tabs && tab>=2 && tab<=4 && alpha>=0 && alpha<=1,
        "native alpha write has current object, supported tab and bounded opacity");
    fixture->writes.push_back({tabs,tab,false,alpha,true});
}
std::uint64_t Clock() noexcept {
    Check(!fixture->observerThread,"batch observers do not establish UI animation timing");return fixture->now;
}
}

int main() {
    AdmissionTests();ConfirmationTests();UnknownCategoryTests();OncePerBatchTests();IndependentBatchTests();
    CancellationTests();RapidBatchTests();SchedulingAndGuardTests();
    std::puts("Stash feedback SDK admission, observer isolation, once-per-batch animation, scheduling and lifecycle passed.");
}
