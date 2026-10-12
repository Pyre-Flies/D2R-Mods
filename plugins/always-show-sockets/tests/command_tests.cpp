#include "../src/plugin.cpp"
#include <cstdlib>
void Check(bool value,const char* why) {if(!value){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
int main() {
    const D2RL::PluginApi api{.apiSize=D2RL::PluginApiSize};
    const D2RL::PluginContext context{.contextSize=sizeof(D2RL::PluginContext),.abiVersion=D2RL_PLUGIN_ABI_VERSION,.api=&api};
    ctx=&context;ready.store(true);enabled.store(true);
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
