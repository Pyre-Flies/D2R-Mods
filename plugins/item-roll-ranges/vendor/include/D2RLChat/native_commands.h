#pragma once

#include <cstdint>
#include <string>

namespace D2RL { struct PluginContext; }
namespace D2RLChat {

// args is borrowed for the duration of the callback. Execution runs on the
// game's UI thread. Do not throw or call Stop from inside a command callback.
using NativeCommandCallback = void(*)(const char* args, void* userData) noexcept;

enum class NativeCommandState { Unknown, Pending, Registered, Duplicate, Failed, Stopped };
constexpr const char* NativeCommandStateName(NativeCommandState state) noexcept {
    switch (state) {
    case NativeCommandState::Pending: return "pending";
    case NativeCommandState::Registered: return "registered";
    case NativeCommandState::Duplicate: return "duplicate";
    case NativeCommandState::Failed: return "failed";
    case NativeCommandState::Stopped: return "stopped";
    default: return "unknown";
    }
}
struct NativeCommandStatus {
    NativeCommandState state = NativeCommandState::Unknown;
    uint32_t nativeResult = 0;
    uint64_t calls = 0;
    uint32_t queueResult = 0; // D2RL::Threads::Result from the last attempt.
};

// Registers with D2R's actual slash parser: no JSON, localization table, or
// inline hooks. Each DLL links its own copy; the game arbitrates duplicate names.
// Call Start/Register during plugin load and Stop before releasing callback data.
// Register returning true means accepted/pending; inspect Status for the native
// outcome. UI events retry requests rejected by the queue during plugin load.
// Stop drains callbacks and disables retained pointers. It does not remove the
// native entry: names remain reserved until process exit. Restart to reload.
class NativeCommands {
public:
    NativeCommands() = default;
    ~NativeCommands();
    NativeCommands(const NativeCommands&) = delete;
    NativeCommands& operator=(const NativeCommands&) = delete;

    bool Start(const D2RL::PluginContext* context);
    // Lowercase ASCII name, 1..31 bytes, [a-z0-9_-], without '/'. Description
    // is UTF-8, at most 63 bytes. Optional help receives the description text.
    bool Register(const std::string& name, NativeCommandCallback callback,
        void* userData = nullptr, const std::string& description = "",
        NativeCommandCallback help = nullptr);
    NativeCommandStatus Status(const std::string& name) const;
    std::string Reason() const;
    void Stop() noexcept;

private:
    const D2RL::PluginContext* context_ = nullptr;
    std::string reason_ = "not started";
};
}
