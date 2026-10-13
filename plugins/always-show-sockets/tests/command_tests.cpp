#include "../src/plugin.cpp"
#include <cstdlib>
void Check(bool value,const char* why) {if(!value){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
extern "C" int TestPostIconGate(void*,void*);
extern "C" void TestPostIconContinuation();
void* observedWidget{};
void* observedItem{};
unsigned draws{};
void __fastcall CaptureDraw(void* widget,void* item) {observedWidget=widget;observedItem=item;++draws;}
int main() {
    const D2RL::PluginApi api{.apiSize=D2RL::PluginApiSize};
    const D2RL::PluginContext context{.contextSize=sizeof(D2RL::PluginContext),.abiVersion=D2RL_PLUGIN_ABI_VERSION,.api=&api};
    ctx=&context;ready.store(true);enabled.store(true);
    socketOriginal=reinterpret_cast<void*>(CaptureDraw);
    SocketPostIconOriginal=reinterpret_cast<void*>(TestPostIconContinuation);
    int widget{},item{};
    Check(TestPostIconGate(&widget,&item)==1,"post-icon gateway preserves volatile GPR/XMM/flags and stack alignment");
    Check(observedWidget==&widget && observedItem==&item && draws==1,"native frame widget/item forwarded exactly once");
    SocketHook(&widget,&item);Check(draws==1,"selected native socket call does not draw twice");
    enabled.store(false);
    Check(TestPostIconGate(&widget,&item)==1 && draws==1,"disabled gateway preserves state without added draw");
    SocketHook(&widget,&item);Check(draws==2,"disabled mode preserves native socket call");
    enabled.store(true);
    ChatCommand("",nullptr);Check(!enabled.load(),"bare slash arguments toggle off");
    ChatCommand(nullptr,nullptr);Check(enabled.load(),"null slash arguments toggle on");
    ChatCommand("off",nullptr);Check(!enabled.load(),"explicit off");
    ChatCommand("status",nullptr);Check(!enabled.load(),"status preserves state");
    ChatCommand("on",nullptr);Check(enabled.load(),"explicit on");
    ChatCommand("invalid",nullptr);Check(enabled.load(),"invalid arguments preserve state");
    ChatCommand("off followed by overlong arguments",nullptr);Check(enabled.load(),"overlong arguments rejected");
    ready.store(false);ChatCommand("off",nullptr);Check(enabled.load(),"unsupported build cannot alter feature");
    ready.store(true);
    D2RL::ConsoleCommandContext command{.contextSize=D2RL::ConsoleCommandContextSize,.argsLength=6,.args="toggle",.plugin=&context};
    Check(Command(nullptr,&command,nullptr)==D2RL::ConsoleCommandResult::Handled && !enabled.load(),"console shares toggle handler");
    command.args=nullptr;command.argsLength=1;
    Check(Command(nullptr,&command,nullptr)==D2RL::ConsoleCommandResult::Failed,"malformed borrowed arguments refused");
    D2RLoaderUnloadPlugin();ctx=nullptr;
    std::puts("Slash/console socket toggle, explicit state, status, bounds and unsupported refusal passed.");
}
