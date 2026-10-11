// Test-only DLL adapter. Include the implementation to inject the fake game's
// entry points without exposing a backend override in the public SDK.
#include "../vendor/src/native_commands.cpp"

static D2RLChat::NativeCommands commands;
extern "C" __declspec(dllexport) bool Start(const D2RL::PluginContext* context, void* getter, void* registration) {
    D2RLChat::getParser = reinterpret_cast<D2RLChat::GetParser>(getter);
    D2RLChat::registerCommand = reinterpret_cast<D2RLChat::RegisterCommand>(registration);
    return commands.Start(context);
}
extern "C" __declspec(dllexport) bool Register(const char* name, D2RLChat::NativeCommandCallback callback,
    void* userData, const char* description, D2RLChat::NativeCommandCallback help) {
    return commands.Register(name, callback, userData, description, help);
}
extern "C" __declspec(dllexport) D2RLChat::NativeCommandStatus Status(const char* name) { return commands.Status(name); }
extern "C" __declspec(dllexport) void Stop() { commands.Stop(); }
