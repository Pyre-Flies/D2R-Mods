// Inject native parser entries only in this executable, as in the existing
// socket command regression. Production resolution/admission stays unchanged.
#include "../src/plugin.cpp"
#include "../vendor/src/native_commands.cpp"
#include "test.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <vector>

namespace Test {
struct Image {
    unsigned char* data{};
    explicit Image(size_t size) {
        data=static_cast<unsigned char*>(VirtualAlloc(nullptr,size,MEM_RESERVE,PAGE_NOACCESS));
        Check(data!=nullptr,"native fixture reservation");
    }
    ~Image(){VirtualFree(data,0,MEM_RELEASE);}
    uintptr_t Base() const {return reinterpret_cast<uintptr_t>(data);}
    void Put(uintptr_t rva,const void* bytes,size_t size) {
        const auto page=rva&~uintptr_t(4095);
        Check(VirtualAlloc(data+page,(rva-page+size+4095)&~size_t(4095),MEM_COMMIT,PAGE_READWRITE)!=nullptr,"native fixture commit");
        std::memcpy(data+rva,bytes,size);
    }
    template<size_t N> void Put(uintptr_t rva,const std::array<unsigned char,N>& bytes){Put(rva,bytes.data(),N);}
    void Pointer(uintptr_t rva,uintptr_t value){Put(rva,&value,sizeof(value));}
    void Code(uintptr_t rva) {DWORD old{};Check(VirtualProtect(data+(rva&~uintptr_t(4095)),4096,PAGE_EXECUTE_READ,&old)!=FALSE,"fixture code protection");}
};
struct Native {
    Image host{0x4000000},core{0x800000},renderer{0x1408a0};
    Native() {
        using namespace DayNight::Profile;
        core.Put(CoreWrapperRva,CoreWrapper);core.Put(NoDebugDescriptorRva,NoDebugDescriptor);core.Put(SetterDescriptorRva,SetterDescriptor);
        host.Put(GameCallerRva,GameCaller);host.Put(GameThunkRva,GameThunk);host.Put(NativeSetterRva,NativeSetter);
        core.Pointer(CoreNoDebugPointerRva,host.Base()+NativeNoDebugRva);
        core.Pointer(CoreSetterPointerRva,host.Base()+NativeSetterRva);
        host.Pointer(GameThunkSlotRva,core.Base()+CoreWrapperRva);
        const unsigned char clear=0,set=1;
        core.Put(CoreSafetyFlagRva,&clear,1);host.Put(NativeNoDebugRva,&set,1);
        host.Put(RendererGetterRva,RendererGetter);host.Put(RendererOverrideControlRva,RendererOverrideControl);
        host.Put(ScriptSetterCallRva,ScriptSetterCall);host.Put(NormalCycleTableRva,NormalCycleTable);
        host.Pointer(RendererSlotRva,renderer.Base());
        const uint64_t initial=(uint64_t{0x3f000000}<<32)|2;
        renderer.Put(RendererEnvironmentOffset+EnvironmentPeriodOffset,&initial,8);
        renderer.Put(RendererOverrideOffset,&clear,1);
        core.Code(CoreWrapperRva);host.Code(NativeSetterRva);
        FlushInstructionCache(GetCurrentProcess(),nullptr,0);
    }
    void Observe(bool enabled) {
        using namespace DayNight::Profile;
        std::array<unsigned char,0x1a0> environment{};environment.fill(0xa5);
        auto expected=environment;
        const float blend=0.5f;const int period=4;
        if(enabled) {
            std::memcpy(expected.data()+EnvironmentBlendOffset,&blend,4);
            std::memcpy(expected.data()+EnvironmentPeriodOffset,&period,4);
        }
        reinterpret_cast<void(__fastcall*)(void*,float,int)>(core.Base()+CoreWrapperRva)(environment.data(),blend,period);
        Check(expected==environment,"command state controls the actual native wrapper and setter");
        Check(host.data[NativeNoDebugRva]==1,"commands preserve normal debug state");
    }
};
std::filesystem::path configFile;
unsigned reads{},writes{};
bool failRead{},failWrite{},oversize{},unterminated{};
std::string lastMessage;
std::string ReadFile() {std::ifstream file(configFile,std::ios::binary);return {std::istreambuf_iterator<char>(file),{}};}
void Seed(std::string_view text) {std::ofstream file(configFile,std::ios::binary|std::ios::trunc);file.write(text.data(),static_cast<std::streamsize>(text.size()));}
bool __cdecl Read(const D2RL::PluginContext*,char* output,uint32_t size,uint32_t* required) noexcept {
    ++reads;if(failRead)return false;
    try {
        const auto text=ReadFile();*required=oversize?DayNight::ConfigCapacity+1:static_cast<uint32_t>(text.size()+1);
        if(size<*required)return false;
        if(unterminated) {std::memset(output,'x',size);return true;}
        std::memcpy(output,text.c_str(),*required);return true;
    }catch(...){return false;}
}
bool __cdecl Write(const D2RL::PluginContext*,const char* text) noexcept {
    ++writes;if(failWrite)return false;
    try {Seed(text);return true;}catch(...){return false;}
}
void __cdecl Message(const D2RL::PluginContext*,const char* text,D2RL::ConsoleMessageKind) noexcept {lastMessage=text;}
D2RL::Threads::Callback queued{};
const D2RL::PluginContext* queuedContext{};
void* queuedData{};
bool queueBusy=true,onUi{};
D2RL::SharedEvents::UiMessageListener listener{};
const D2RLChat::Definition* nativeEntry{};
unsigned unregistered{};
D2RL::Threads::Result __cdecl Queue(const D2RL::PluginContext* ctx,D2RL::Threads::Callback callback,void* data) noexcept {
    if(queueBusy)return D2RL::Threads::Result::Busy;
    queued=callback;queuedContext=ctx;queuedData=data;return D2RL::Threads::Result::Success;
}
D2RL::SharedEvents::Result __cdecl Listen(const D2RL::PluginContext*,const D2RL::SharedEvents::UiMessageListener* value,uint64_t* handle) noexcept {
    listener=*value;*handle=1;return D2RL::SharedEvents::Result::Success;
}
D2RL::SharedEvents::Result __cdecl Unlisten(const D2RL::PluginContext*,uint64_t handle) noexcept {
    Check(handle==1,"remove only owned deferred listener");++unregistered;return D2RL::SharedEvents::Result::Success;
}
const D2RL::ThreadService threads{sizeof(threads),1,Queue,nullptr};
const D2RL::SharedEventService events{.serviceSize=sizeof(events),.serviceVersion=1,.registerUiMessageListener=Listen,.unregisterUiMessageListener=Unlisten};
D2RL::ServiceQueryResult __cdecl Query(const D2RL::PluginContext*,D2RL::ServiceId id,uint32_t,const void** output) noexcept {
    if(id==D2RL::ServiceId::Thread){*output=&threads;return D2RL::ServiceQueryResult::Success;}
    if(id==D2RL::ServiceId::SharedEvent){*output=&events;return D2RL::ServiceQueryResult::Success;}
    return D2RL::ServiceQueryResult::UnknownService;
}
uint64_t* __fastcall Parser(){Check(onUi,"parser resolved on UI thread");static uint64_t parser=123;return &parser;}
uint32_t __fastcall Register(uint64_t parser,const D2RLChat::Definition* value) {
    Check(onUi && parser==123,"native command registered on UI thread");
    Check(!std::strcmp(value->name,"daynight") && std::strlen(value->description)<=63 && value->execute && value->help,"real command record and bounded help");
    nativeEntry=value;return 0;
}
void Drain() {
    if(!queued)return;
    const auto callback=queued;const auto data=queuedData;const auto ctx=queuedContext;queued=nullptr;
    onUi=true;callback(ctx,data);onUi=false;
}
DayNight::Preview::Observation ObserveRenderer() {
    DayNight::Preview::Observation value;Check(preview.Inspect(value),"observe guarded renderer fields");return value;
}
D2RL::ConsoleCommandResult Invoke(const char* args) {
    const D2RL::ConsoleCommandContext value{.contextSize=D2RL::ConsoleCommandContextSize,
        .argsLength=static_cast<uint32_t>(std::strlen(args)),.args=args,.plugin=pluginContext};
    const auto result=Command(nullptr,&value,nullptr);Drain();return result;
}
}

int main() try {
    using namespace Test;
    using Result=D2RL::ConsoleCommandResult;
    const auto directory=std::filesystem::temp_directory_path()/("daynight-command-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);configFile=directory/"day-night-cycle.toml";
    const std::string original="\xef\xbb\xbf# keep\r\n[day-night-cycle]\r\nenabled = true # preference\r\ncolor = \"#AABBCC\"\r\n[other]\r\nkeep = 123\r\n";
    Seed(original);
    const D2RL::PluginApi api{.apiSize=D2RL::PluginApiSize,.readConfig=Read,.writeConfig=Write,.writeConsoleMessage=Message,.queryService=Query};
    D2RL::PluginContext ctx{.contextSize=sizeof(ctx),.abiVersion=D2RL_PLUGIN_ABI_VERSION,.api=&api};
    pluginContext=&ctx;
    Native native;ctx.exeBase=native.host.Base();
    ::uiThreads=&Test::threads;::uiEvents=&Test::events;
    Check(preview.Start(native.host.Base(),native.core.Base()),"preview native admission");
    ::settings=*DayNight::ParseConfig(original);
    Check(runtime.Admit(native.host.Base(),native.core.Base()),"production native runtime admission");
    Check(runtime.SetEnabled(DayNight::ParseConfig(*DayNight::ReadSettings(pluginContext))->enabled),"startup preference enables native patch");
    native.Observe(true);
    D2RLChat::getParser=Parser;D2RLChat::registerCommand=Register;
    Check(RegisterChat() && chatCommands.Status("daynight").state==D2RLChat::NativeCommandState::Pending && !queued,"register safely defers while loader queue is busy");
    queueBusy=false;listener.callback(pluginContext,nullptr,listener.userData);
    Check(queued && !nativeEntry,"registration waits for UI work");
    Drain();
    Check(nativeEntry && unregistered==1 && chatCommands.Status("daynight").state==D2RLChat::NativeCommandState::Registered,"retained native dispatch is active");
    nativeEntry->execute("off");Drain();
    Check(!runtime.Enabled() && writes==1 && lastMessage.find("off (daylight)")!=std::string::npos,"real slash callback applies and saves off");
    native.Observe(false);
    const auto expected=DayNight::UpdateConfig(original,{false,DayNight::Mode::Auto});
    Check(expected && ReadFile()==*expected,"preserve BOM, CRLF, comments, colors and unrelated settings");
    runtime.Stop();Check(runtime.Admit(native.host.Base(),native.core.Base()),"fresh runtime restart");
    const auto restarted=DayNight::ParseConfig(*DayNight::ReadSettings(pluginContext));
    ::settings=*restarted;
    Check(restarted && !restarted->enabled && runtime.SetEnabled(restarted->enabled) && !runtime.Enabled(),"fresh startup restores saved off value from actual TOML file");
    nativeEntry->execute("");Drain();Check(runtime.Enabled() && writes==2,"bare command toggles and persists");native.Observe(true);
    Check(Invoke(" on ")==Result::Handled && writes==3 && runtime.Enabled(),"idempotent explicit command saves preference");
    auto beforeWrites=writes;
    Check(Invoke("status")==Result::Handled && Invoke("help")==Result::Handled && Invoke("wrong")==Result::InvalidArguments && writes==beforeWrites,"status/help/invalid commands never write");
    failRead=true;Check(Invoke("off")==Result::Failed && runtime.Enabled() && writes==beforeWrites,"read failure retains current state");failRead=false;
    oversize=true;Check(Invoke("off")==Result::Failed && runtime.Enabled() && writes==beforeWrites,"oversized document refusal");oversize=false;
    unterminated=true;Check(Invoke("off")==Result::Failed && runtime.Enabled() && writes==beforeWrites,"unterminated document refusal");unterminated=false;
    for(const char* malformed:{"[day-night-cycle]\nenabled=true\nenabled=false\n","[day-night-cycle]\n[day-night-cycle]\n","[day-night-cycle]\nenabled='true'\n","[ day-night-cycle ]\nenabled=true\n","[other]\nnote=\"\"\"multiline\n[day-night-cycle]\n\"\"\"\n"}) {
        Seed(malformed);Check(!DayNight::ParseConfig(ReadFile()),"ambiguous startup preference refused");
        Check(Invoke("off")==Result::Failed && runtime.Enabled() && writes==beforeWrites && ReadFile()==malformed,"malformed config cannot change native state or overwrite text");
    }
    Seed(original);failWrite=true;
    Check(Invoke("off")==Result::Handled && runtime.Enabled() && ReadFile()==original && lastMessage.find("previous setting restored")!=std::string::npos,"save failure restores previous actual native patch");native.Observe(true);
    failWrite=false;nativeEntry->execute("off");Drain();native.Observe(false);
    failWrite=true;Check(Invoke("on")==Result::Handled && !runtime.Enabled(),"save failure also rolls newly enabled patch back off");native.Observe(false);failWrite=false;
    Seed("[other]\nkeep=123");Check(Invoke("on")==Result::Handled && DayNight::ParseConfig(ReadFile())->enabled && ReadFile().starts_with("[other]\nkeep=123"),"missing section inserted without losing existing text");
    const auto nativeBefore=ObserveRenderer();
    Check(Invoke("day")==Result::Handled && ObserveRenderer().Stage()==2 && ObserveRenderer().Blend()==0 && ObserveRenderer().overrideTime==1,"instant day uses actual native setter and holds noon");
    Check(DayNight::ParseConfig(ReadFile())->mode==DayNight::Mode::Day,"day mode is saved");
    Check(Invoke("night")==Result::Handled && ObserveRenderer().Stage()==5 && ObserveRenderer().Blend()==0 && ObserveRenderer().overrideTime==1,"instant night switches actual renderer fields");
    const auto nightText=ReadFile();const auto nightSettings=*DayNight::ParseConfig(nightText);
    Check(nightSettings.enabled && nightSettings.mode==DayNight::Mode::Night,"fresh config parse restores chosen night mode");
    failWrite=true;
    Check(Invoke("day")==Result::Handled && ObserveRenderer().Stage()==5 && ObserveRenderer().overrideTime==1 && ReadFile()==nightText,"failed mode save restores the actual previous night fields and ownership");
    failWrite=false;
    Check(preview.Stop() && runtime.Stop()==DayNight::PatchResult::Restored,"restart removes prior forced state and patch");
    Check(preview.Start(native.host.Base(),native.core.Base()) && runtime.Admit(native.host.Base(),native.core.Base()) &&
        runtime.SetEnabled(nightSettings.enabled) && preview.Force(nightSettings.mode) && ObserveRenderer().Stage()==5 && ObserveRenderer().overrideTime==1,
        "fresh startup routines restore saved night mode into actual native fields");
    ::settings=nightSettings;
    Check(Invoke("auto")==Result::Handled && ObserveRenderer()==nativeBefore,"auto releases owned override and restores the prior native snapshot");
    Check(Invoke("night")==Result::Handled && Invoke("off")==Result::Handled && !runtime.Enabled() && ObserveRenderer().Stage()==2 && ObserveRenderer().overrideTime==0,"off during night resets daylight before stopping updates");
    Check(Invoke("on")==Result::Handled && runtime.Enabled() && ::settings.mode==DayNight::Mode::Auto,"on resumes natural cycle");
    for(const char* bad:{"[day-night-cycle]\nmode=\"bad\"\n","[day-night-cycle]\nmode=\"day\"\nmode=\"night\"\n","[day-night-cycle]\n\"mode\"=\"day\"\n"}) {
        Seed(bad);const auto unchanged=ObserveRenderer();beforeWrites=writes;
        Check(!DayNight::ParseConfig(ReadFile()) && Invoke("night")==Result::Failed && writes==beforeWrites && ObserveRenderer()==unchanged,"ambiguous mode config is refused without renderer/config changes");
    }
    Seed(original);queueBusy=true;beforeWrites=writes;
    Check(Invoke("night")==Result::Failed && writes==beforeWrites && ObserveRenderer().overrideTime==0,"UI queue refusal cannot claim the renderer or save a mode");queueBusy=false;
    // An existing manual time override belongs to someone else.
    native.renderer.data[DayNight::Profile::RendererOverrideOffset]=1;beforeWrites=writes;
    Check(Invoke("night")==Result::Handled && writes==beforeWrites && ObserveRenderer().Stage()==2 && ObserveRenderer().overrideTime==1,"foreign manual override is preserved");
    native.renderer.data[DayNight::Profile::RendererOverrideOffset]=0;
    native.core.data[DayNight::Profile::CoreSafetyFlagRva]=1;beforeWrites=writes;
    Check(Invoke("night")==Result::Handled && writes==beforeWrites && ObserveRenderer().overrideTime==0,"independent Core gate refuses instant control without config change");
    native.core.data[DayNight::Profile::CoreSafetyFlagRva]=0;
    Check(Invoke("night")==Result::Handled,"forced state for ownership test");
    const auto pairOffset=DayNight::Profile::RendererEnvironmentOffset+DayNight::Profile::EnvironmentPeriodOffset;
    const uint64_t foreignPair=(uint64_t{0x3e800000}<<32)|5;
    std::memcpy(native.renderer.data+pairOffset,&foreignPair,8);beforeWrites=writes;
    Check(Invoke("auto")==Result::Handled && writes==beforeWrites && ObserveRenderer().pair==foreignPair && ObserveRenderer().overrideTime==1,
        "later renderer owner refuses automatic release without clobbering its fields");
    const uint64_t ownedNight=5;std::memcpy(native.renderer.data+pairOffset,&ownedNight,8);
    Check(Invoke("auto")==Result::Handled && Invoke("night")==Result::Handled,"owned fixture cleanup and forced state for unload restoration");
    Check(native.host.data[DayNight::Profile::NativeNoDebugRva]==1,"all instant commands preserve global debug mode");
    nativeEntry->execute("day");Check(queued && pending.active,"pending UI command before unload");
    DayNight::OwnedPatch successor;auto later=DayNight::Profile::EnabledBlock;later[15]=0x91;
    const auto address=native.core.Base()+DayNight::Profile::CoreBlockRva;
    Check(successor.Install(address,DayNight::Profile::EnabledBlock,later)==DayNight::PatchResult::Applied,"later owner fixture publication");
    beforeWrites=writes;Check(Invoke("off")==Result::Failed && writes==beforeWrites && DayNight::Bytes(address,later),"foreign modification refuses command without config write or clobber");
    D2RLoaderUnloadPlugin();const auto afterStop=writes;nativeEntry->execute("on");Drain();
    Check(native.renderer.data[DayNight::Profile::RendererOverrideOffset]==0,"unload releases owned manual override");
    Check(writes==afterStop && DayNight::Bytes(address,later),"stopped retained callback is inert and unload preserves later owner");
    successor.Stop();
    Check(directory.is_absolute() && (directory.parent_path()/"scope")== (std::filesystem::temp_directory_path()/"scope") &&
        directory.filename().string().starts_with("daynight-command-") && configFile.parent_path()==directory,
        "fixture cleanup remains in the named temporary directory");
    // Delete only the known file and then the empty directory. Canonical-path
    // handle queries are denied by some Windows sandboxes; no recursion needed.
    Check(std::filesystem::remove(configFile) && std::filesystem::remove(directory),"remove only owned fixture file and empty directory");
    std::puts("PASS: actual native chat dispatch, native toggles and instant preview, TOML preservation/restart, failure rollback and owned shutdown.");
}
catch(const std::exception& error) {std::fprintf(stderr,"FAIL: command fixture exception: %s\n",error.what());return 1;}
