#include <D2RLPlugin/api.h>
#include <D2RLChat/native_commands.h>
#include "hash.h"
#include <atomic>
#include <string_view>

namespace {
const D2RL::PluginContext* ctx{};
std::atomic<bool> ready{},enabled{};
D2RLChat::NativeCommands chatCommands;
using Draw=void(__fastcall*)(void*,void*);
void* iconOriginal{};
void* socketOriginal{};
// Native caller ignores return registers; both consumers supply widget, item.
constexpr unsigned char IconBytes[]={0x48,0x85,0xd2,0x0f,0x84,0xd7,0x00,0x00,0x00,0x56,0x57,0x48,0x83,0xec,0x68};
constexpr unsigned char SocketBytes[]={0x40,0x55,0x53,0x41,0x54,0x48,0x8d,0xac,0x24,0x70,0xff,0xff,0xff,0x48,0x81,0xec,0x90,0x01,0x00,0x00};
thread_local void* lastWidget{};
thread_local void* lastItem{};
void __fastcall IconHook(void* widget,void* item) noexcept {
    reinterpret_cast<Draw>(iconOriginal)(widget,item);
    lastWidget=nullptr;lastItem=nullptr;
    if(ready.load() && enabled.load() && widget && item) {
        reinterpret_cast<Draw>(socketOriginal)(widget,item);
        lastWidget=widget;lastItem=item;
    }
}
void __fastcall SocketHook(void* widget,void* item) noexcept {
    // Both existing selected-item paths draw the icon immediately before sockets.
    // Consume only a matching duplicate, preserving other native invocations.
    if(ready.load() && enabled.load() && widget==lastWidget && item==lastItem) {
        lastWidget=nullptr;lastItem=nullptr;return;
    }
    reinterpret_cast<Draw>(socketOriginal)(widget,item);
}
D2RL::ConsoleCommandResult __cdecl Command(D2R::Game::Client*,const D2RL::ConsoleCommandContext* command,void*) noexcept {
    if(!command || command->contextSize<D2RL::ConsoleCommandContextSize || command->plugin!=ctx ||
       command->argsLength>16 || (!command->args && command->argsLength)) return D2RL::ConsoleCommandResult::Failed;
    const std::string_view arg(command->args?command->args:"",command->argsLength);
    if(arg!="on" && arg!="off" && arg!="status" && arg!="toggle" && !arg.empty()) return D2RL::ConsoleCommandResult::InvalidArguments;
    if(!ready.load()) {ctx->WriteConsoleMessage("Sockets: unsupported build or hook unavailable.",D2RL::ConsoleMessageKind::Warning);return D2RL::ConsoleCommandResult::Failed;}
    if(arg=="on") enabled.store(true);
    if(arg=="off") enabled.store(false);
    if(arg.empty() || arg=="toggle") enabled.store(!enabled.load());
    ctx->WriteConsoleMessage(enabled.load()?"Sockets: always visible.":"Sockets: native selection behavior.");
    if(arg!="status") ctx->LogInfo(enabled.load()?"Sockets: always visible ON.":"Sockets: always visible OFF.");
    return D2RL::ConsoleCommandResult::Handled;
}
void ChatCommand(const char* args,void*) noexcept {
    if(!ctx) return;
    const auto length=args?strnlen_s(args,17):0;
    if(length>16) return;
    const D2RL::ConsoleCommandContext command{.contextSize=D2RL::ConsoleCommandContextSize,
        .argsLength=static_cast<std::uint32_t>(length),.args=args,.plugin=ctx};
    Command(nullptr,&command,nullptr);
}
}
D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    static constexpr D2RL::PluginInfo info{.infoSize=D2RL::PluginInfoSize,.abiVersion=D2RL_PLUGIN_ABI_VERSION,
        .id="always-show-sockets",.name="Always Show Sockets",.version="0.1.0-beta.1",.author="PyreFly",
        .description="Experimental native empty and filled socket display.",.flags=D2RL::PluginFlags::Client|D2RL::PluginFlags::NativeHooks};
    return &info;
}
D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* context) noexcept -> bool {
    if(!context || !context->GetApi()) return false;
    ctx=context;
    if(!ctx->RegisterConsoleCommand("sockets",Command,"Toggle sockets; on, off, status.")) return false;
    if(!Sockets::Hash(reinterpret_cast<HMODULE>(ctx->exeBase),"93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10") ||
       !Sockets::Hash(GetModuleHandleW(L"D2RCore.dll"),"2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2") ||
       !ctx->CheckExpectedBytes(0x2a72d0,IconBytes,sizeof(IconBytes)) ||
       !ctx->CheckExpectedBytes(0x2a73c0,SocketBytes,sizeof(SocketBytes))) {
        ctx->LogWarn("Sockets: exact-build guard refused; native display preserved.");return true;
    }
    if(!ctx->InstallInlineHook(0x2a73c0,SocketBytes,sizeof(SocketBytes),reinterpret_cast<void*>(SocketHook),&socketOriginal) || !socketOriginal) return true;
    if(!ctx->InstallInlineHook(0x2a72d0,IconBytes,sizeof(IconBytes),reinterpret_cast<void*>(IconHook),&iconOriginal) || !iconOriginal) return true;
    ready.store(true);enabled.store(true);
    try {
        if(!chatCommands.Start(ctx) || !chatCommands.Register("sockets",ChatCommand,nullptr,"Toggle sockets; on, off, status."))
            ctx->LogWarn("Sockets: native chat registration unavailable; debug-console command remains available.");
    } catch(...) {ctx->LogWarn("Sockets: native chat registration failed; debug-console command remains available.");}
    ctx->LogInfo("Sockets probe: native icon/socket hooks admitted; always visible ON. /sockets toggles; /sockets off restores selection behavior.");
    return true;
}
D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {chatCommands.Stop();enabled.store(false);ready.store(false);}
