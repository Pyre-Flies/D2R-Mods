#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <windows.h>
#include <D2RLPlugin/api.h>
#include <D2RLChat/native_commands.h>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

using Callback = D2RLChat::NativeCommandCallback;
using State = D2RLChat::NativeCommandState;
struct Pending { const D2RL::PluginContext* context; D2RL::Threads::Callback fn; void* data; };
std::vector<Pending> queue;
bool refuseQueue = false;
bool ownerActive = true;
bool onUi = false;
auto __cdecl Queue(const D2RL::PluginContext* ctx, D2RL::Threads::Callback fn, void* data) noexcept -> D2RL::Threads::Result {
    if (refuseQueue) return D2RL::Threads::Result::Busy;
    if (!ownerActive) return D2RL::Threads::Result::OwnerInactive;
    queue.push_back({ctx,fn,data}); return D2RL::Threads::Result::Success;
}
D2RL::ThreadService threads{sizeof(threads),1,Queue,nullptr};
std::map<uint64_t,D2RL::SharedEvents::UiMessageListener> listeners;
uint64_t nextListener = 1;
auto __cdecl Listen(const D2RL::PluginContext*, const D2RL::SharedEvents::UiMessageListener* listener, uint64_t* handle) noexcept -> D2RL::SharedEvents::Result {
    *handle=nextListener++; listeners.emplace(*handle,*listener); return D2RL::SharedEvents::Result::Success;
}
auto __cdecl Unlisten(const D2RL::PluginContext*, uint64_t handle) noexcept -> D2RL::SharedEvents::Result {
    listeners.erase(handle); return D2RL::SharedEvents::Result::Success;
}
D2RL::SharedEventService events{sizeof(events),1,nullptr,nullptr,Listen,Unlisten};
void UiEvent() {
    const auto snapshot=listeners;
    for (const auto& [handle,listener] : snapshot) {
        assert(listener.callback(nullptr,nullptr,listener.userData)==D2RL::SharedEvents::UiMessageAction::Continue);
    }
}
auto __cdecl Query(const D2RL::PluginContext*, D2RL::ServiceId id, uint32_t, const void** out) noexcept -> D2RL::ServiceQueryResult {
    if (id == D2RL::ServiceId::SharedEvent) { *out=&events; return D2RL::ServiceQueryResult::Success; }
    if (id != D2RL::ServiceId::Thread) return D2RL::ServiceQueryResult::UnknownService;
    *out = &threads; return D2RL::ServiceQueryResult::Success;
}
void Flush() {
    const auto pending = std::move(queue); queue.clear(); onUi = true;
    for (const auto& item : pending) item.fn(item.context,item.data);
    onUi = false;
}
uint64_t parserId = 123;
uint64_t* __fastcall Parser() { assert(onUi); return &parserId; }
using NativeCallback = void(__fastcall*)(const char*);
struct Entry { NativeCallback execute, help; std::string description; };
std::map<std::string, Entry> entries;
template<class T> T Read(const unsigned char* bytes, size_t offset) { T value; std::memcpy(&value,bytes+offset,sizeof(value)); return value; }
uint32_t __fastcall RegisterNative(uint64_t parser, const void* record) {
    assert(onUi && parser == parserId);
    const auto* bytes = static_cast<const unsigned char*>(record);
    assert(Read<uint32_t>(bytes,0) == 1);
    using Translator = void(__fastcall*)(const char*,char*,size_t);
    const auto translate = Read<Translator>(bytes,0x1A0);
    char name[32]{};
    translate(reinterpret_cast<const char*>(bytes+4), name, sizeof(name));
    char bounded[3]={'x','x','z'}; translate("hello",bounded,2);
    assert(bounded[0]=='h' && bounded[1]==0 && bounded[2]=='z');
    translate("hello",bounded,0); assert(bounded[0]=='h');
    if (entries.contains(name)) return 7;
    if (std::strcmp(name,"fail")==0) return 4;
    assert(Read<void*>(bytes,0x1A8)==nullptr && Read<uint64_t>(bytes,0x1B0)==0);
    entries.emplace(name, Entry{Read<NativeCallback>(bytes,0x188),Read<NativeCallback>(bytes,0x190),reinterpret_cast<const char*>(bytes+0x44)});
    return 0;
}
struct Module {
    HMODULE handle;
    bool(*start)(const D2RL::PluginContext*,void*,void*);
    bool(*reg)(const char*,Callback,void*,const char*,Callback);
    D2RLChat::NativeCommandStatus(*status)(const char*);
    void(*stop)();
    explicit Module(const char* path) : handle(LoadLibraryA(path)) {
        assert(handle);
        start=reinterpret_cast<decltype(start)>(GetProcAddress(handle,"Start"));
        reg=reinterpret_cast<decltype(reg)>(GetProcAddress(handle,"Register"));
        status=reinterpret_cast<decltype(status)>(GetProcAddress(handle,"Status"));
        stop=reinterpret_cast<decltype(stop)>(GetProcAddress(handle,"Stop"));
        assert(start && reg && status && stop);
    }
};
struct Answer { unsigned calls=0; std::string args; };
void AnswerCommand(const char* args,void* data) noexcept { auto& a=*static_cast<Answer*>(data); ++a.calls; a.args=args; }
std::atomic<bool> entered=false, releaseHandler=false;
void Blocking(const char*,void*) noexcept { entered=true; while (!releaseHandler) std::this_thread::yield(); }
int main(int argc,char** argv) {
    assert(argc==3);
    Module a(argv[1]), b(argv[2]);
    D2RL::PluginApi api{}; api.apiSize=sizeof(api); api.queryService=Query;
    D2RL::PluginContext context{}; context.contextSize=sizeof(context); context.api=&api;
    assert(a.start(&context,reinterpret_cast<void*>(Parser),reinterpret_cast<void*>(RegisterNative)));
    assert(b.start(&context,reinterpret_cast<void*>(Parser),reinterpret_cast<void*>(RegisterNative)));
    Answer first, second, help;
    assert(!a.reg("/bad",AnswerCommand,&first,"",nullptr));
    assert(!a.reg("UPPER",AnswerCommand,&first,"",nullptr));
    assert(!a.reg("null",nullptr,nullptr,"",nullptr));
    assert(!a.reg(std::string(32,'a').c_str(),AnswerCommand,&first,"",nullptr));
    assert(!a.reg("longhelp",AnswerCommand,&first,std::string(64,'a').c_str(),nullptr));
    refuseQueue=true; assert(a.reg("retry",AnswerCommand,&first,"",nullptr)); refuseQueue=false;
    assert(a.status("retry").state==State::Pending && a.status("retry").queueResult==3);
    ownerActive=false;
    assert(a.reg("deaths",AnswerCommand,&first,"Death history",AnswerCommand));
    assert(b.reg("weather",AnswerCommand,&second,"Weather",nullptr));
    assert(b.reg("deaths",AnswerCommand,&second,"duplicate",nullptr));
    assert(a.reg("fail",AnswerCommand,&first,"",nullptr));
    assert(a.status("deaths").state==State::Pending && entries.empty());
    assert(a.status("deaths").queueResult==4 && queue.empty());
    UiEvent(); assert(queue.empty()); // Still loading: keep pending, no native calls.
    ownerActive=true;
    UiEvent(); const auto queued=queue.size(); UiEvent(); assert(queue.size()==queued);
    parserId=0; Flush(); assert(entries.empty()); // Parser creation can lag the UI.
    parserId=123; UiEvent();
    Flush();
    assert(listeners.empty()); // Remove retry listeners after terminal outcomes.
    assert(a.status("deaths").state==State::Registered);
    assert(b.status("deaths").state==State::Duplicate && b.status("deaths").nativeResult==7);
    assert(a.status("fail").state==State::Failed && a.status("fail").nativeResult==4);
    assert(entries.at("deaths").execute != entries.at("weather").execute);
    entries.at("deaths").execute("log 5"); entries.at("weather").execute("today");
    assert(first.calls==1 && first.args=="log 5" && second.calls==1 && second.args=="today");
    entries.at("deaths").help("unused native input"); assert(first.args=="Death history");
    assert(a.status("deaths").calls==1);
    assert(b.reg("pending",AnswerCommand,&second,"",nullptr));
    b.stop(); Flush(); assert(!entries.contains("pending"));
    assert(listeners.empty());
    entries.at("weather").execute("after stop"); assert(second.calls==1);
    assert(a.reg("blocking",Blocking,nullptr,"",nullptr)); Flush();
    std::thread executing([]{entries.at("blocking").execute("");});
    while (!entered) std::this_thread::yield();
    std::atomic<bool> stopped=false, stopping=false;
    std::thread stoppingThread([&]{stopping=true; a.stop(); stopped=true;});
    while (!stopping) std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); assert(!stopped);
    releaseHandler=true; executing.join(); stoppingThread.join(); assert(stopped);
    entries.at("deaths").execute("after stop"); assert(first.calls==2);
    FreeLibrary(a.handle); FreeLibrary(b.handle);
    entries.at("deaths").execute("after FreeLibrary"); assert(first.calls==2);
    std::cout << "Native commands: two independent DLLs, arguments/help, collisions, validation, queue failure, cancellation, callback drain and pinned lifetime passed.\n";
}
