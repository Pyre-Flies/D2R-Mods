#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <D2RLChat/native_commands.h>
#include <D2RLImage/image_scan.h>
#include <D2RLPlugin/api.h>
#include <array>
#include <cstring>
#include <mutex>
#include <utility>
#include <vector>

namespace D2RLChat {
namespace {
using Execute = void(__fastcall*)(const char*);
using Translate = void(__fastcall*)(const char*, char*, size_t);
// Native public registration record, verified against the decrypted reader.
struct Definition {
    uint32_t version = 1;
    char name[64]{};
    char description[64]{};
    char aliases[8][32]{};
    Execute execute = nullptr;
    Execute help = nullptr;
    void* completion = nullptr;
    Translate translate = nullptr;
    const Definition* children = nullptr;
    uint64_t childCount = 0;
    uint8_t flags = 0;
};
static_assert(sizeof(Definition) == 0x1C0);
static_assert(offsetof(Definition, execute) == 0x188);
static_assert(offsetof(Definition, translate) == 0x1A0);
using GetParser = uint64_t*(__fastcall*)();
using RegisterCommand = uint32_t(__fastcall*)(uint64_t, const Definition*);
GetParser getParser{};
RegisterCommand registerCommand{};

constexpr size_t Capacity = 64; // Per DLL; retained slots cannot be reused.
struct Slot {
    NativeCommands* owner{};
    const D2RL::PluginContext* context{};
    Definition definition;
    NativeCommandCallback callback{}, help{};
    void* userData{};
    NativeCommandStatus status;
    bool used = false;
    bool queued = false;
    const D2RL::ThreadService* threads{};
    const D2RL::SharedEventService* events{};
    D2RL::SharedEvents::ListenerHandle listener{};
};
// Pinned module lifetime matches the parser's retained callbacks. Nothing with
// a C++ ABI crosses plugin boundaries; each module owns all of its own storage.
// Intentionally process-lifetime: global NativeCommands objects may be destroyed
// after this translation unit's globals. Retained thunks must never observe a
// destroyed mutex/table during DLL or process teardown.
auto& slots = *new std::array<Slot, Capacity>{};
auto& lock = *new std::recursive_mutex;

void __fastcall Literal(const char* text, char* output, size_t capacity) noexcept {
    if (!output || !capacity) return;
    const size_t length = text ? (std::strlen(text) < capacity - 1 ? std::strlen(text) : capacity - 1) : 0;
    if (length) std::memcpy(output, text, length);
    output[length] = 0;
}

template<size_t Index, bool Help>
void __fastcall Dispatch(const char* args) noexcept {
    const std::lock_guard guard(lock);
    auto& slot = slots[Index];
    if (slot.status.state != NativeCommandState::Registered || !slot.context) return;
    const auto callback = Help ? slot.help : slot.callback;
    if (!callback) return;
    if (!Help) ++slot.status.calls;
    callback(Help ? slot.definition.description : (args ? args : ""), slot.userData);
}
template<bool Help, size_t... Index>
constexpr auto Callbacks(std::index_sequence<Index...>) {
    return std::array<Execute, sizeof...(Index)>{ &Dispatch<Index, Help>... };
}
constexpr auto executeCallbacks = Callbacks<false>(std::make_index_sequence<Capacity>{});
constexpr auto helpCallbacks = Callbacks<true>(std::make_index_sequence<Capacity>{});

void __cdecl RegisterOnUi(const D2RL::PluginContext*, void*) noexcept;
bool Queue(Slot& slot) noexcept {
    const auto result = slot.threads->runOnUiThread(slot.context, RegisterOnUi, &slot);
    slot.status.queueResult = static_cast<uint32_t>(result);
    slot.queued = result == D2RL::Threads::Result::Success;
    return slot.queued;
}

D2RL::SharedEvents::UiMessageAction __cdecl OnUiMessage(const D2RL::PluginContext*,
    const D2RL::SharedEvents::UiMessageEvent*, void* data) noexcept {
    const std::lock_guard guard(lock);
    auto& slot = *static_cast<Slot*>(data);
    if (slot.context && slot.status.state == NativeCommandState::Pending && !slot.queued) Queue(slot);
    return D2RL::SharedEvents::UiMessageAction::Continue;
}

void __cdecl RegisterOnUi(const D2RL::PluginContext*, void* data) noexcept {
    std::unique_lock guard(lock);
    auto& slot = *static_cast<Slot*>(data);
    if (slot.status.state != NativeCommandState::Pending || !slot.context) return;
    // Run only through the SDK's UI queue, alongside the game's own parser use.
    const auto* parser = getParser();
    if (!parser || !*parser) {
        slot.queued = false; // The next UI event retries after parser creation.
        return;
    }
    const uint32_t result = registerCommand(*parser, &slot.definition);
    slot.status.nativeResult = result;
    slot.status.state = result == 0 ? NativeCommandState::Registered
        : result == 7 ? NativeCommandState::Duplicate : NativeCommandState::Failed;
    D2RL::LogInfoF(slot.context, "chat-sdk: native /%s registration result=%u", slot.definition.name, result);
    const auto* events = slot.events;
    const auto* context = slot.context;
    const auto listener = slot.listener;
    slot.listener = 0;
    guard.unlock();
    // Unregistration can drain another listener callback: never hold our lock.
    if (listener) events->unregisterUiMessageListener(context, listener);
}

template<size_t N>
uintptr_t Find(const std::vector<D2RLImage::Section>& sections, uintptr_t base,
    const uint8_t (&bytes)[N], const uint8_t (&mask)[N]) {
    const auto* code = D2RLImage::FindSection(sections, ".text");
    if (!code) return 0;
    std::vector<uint32_t> hits;
    D2RLImage::FindPattern(*code, bytes, mask, N, 2, hits);
    uint32_t begin{}, end{};
    if (hits.size() != 1 || !D2RLImage::PrimaryFunctionRange(sections, base, hits[0], begin, end) || begin != hits[0]) return 0;
    return base + begin;
}

bool Resolve(const D2RL::PluginContext* context) {
    if (getParser && registerCommand) return true;
    std::vector<D2RLImage::Section> sections;
    if (!D2RLImage::ReadSections(context->exeBase, sections)) return false;
    // Captured getter and public RegisterCommand entry; wildcard only RIP
    // displacements. No code is patched or relocated.
    constexpr uint8_t getter[] = {0x48,0x83,0xEC,0x28,0x65,0x48,0x8B,0x04,0x25,0x58,0,0,0,0x8B,0x0D,0,0,0,0,0xBA,0x6C,0x12,0,0,0x48,0x8B,0x0C,0xC8};
    uint8_t getterMask[sizeof(getter)]; std::memset(getterMask, 0xFF, sizeof(getterMask));
    std::memset(getterMask + 15, 0, 4);
    constexpr uint8_t registration[] = {0x41,0x56,0x48,0x83,0xEC,0x40,0x4C,0x8B,0x05,0,0,0,0,0x4C,0x8B,0xF2,0x48,0x8B,0xD1,0x4D,0x85,0xC0};
    uint8_t registrationMask[sizeof(registration)]; std::memset(registrationMask, 0xFF, sizeof(registrationMask));
    std::memset(registrationMask + 9, 0, 4);
    // TLS singleton getters share a prologue. Identify this one through the
    // command-definition loader's validated call site, then verify its entry.
    constexpr uint8_t caller[] = {0x84,0xC0,0x74,0x43,0xE8,0,0,0,0,0x48,0x8B,0xD6,0x48,0x8B,0xC8,0xE8,0,0,0,0,0xEB,0x31};
    uint8_t callerMask[sizeof(caller)]; std::memset(callerMask, 0xFF, sizeof(callerMask));
    std::memset(callerMask + 5, 0, 4); std::memset(callerMask + 16, 0, 4);
    const auto* code = D2RLImage::FindSection(sections, ".text");
    if (!code) return false;
    std::vector<uint32_t> callers;
    D2RLImage::FindPattern(*code, caller, callerMask, sizeof(caller), 2, callers);
    if (callers.size() != 1) return false;
    int32_t displacement{};
    std::memcpy(&displacement, reinterpret_cast<void*>(context->exeBase + callers[0] + 5), 4);
    const auto getterAddress = context->exeBase + callers[0] + 9 + displacement;
    uint32_t begin{}, end{};
    if (!D2RLImage::Readable(reinterpret_cast<void*>(getterAddress), sizeof(getter))
        || !D2RLImage::PrimaryFunctionRange(sections, context->exeBase, static_cast<uint32_t>(getterAddress - context->exeBase), begin, end)
        || begin != getterAddress - context->exeBase) return false;
    const auto* actual = reinterpret_cast<const uint8_t*>(getterAddress);
    for (size_t i = 0; i < sizeof(getter); ++i) if (getterMask[i] && actual[i] != getter[i]) return false;
    const auto registrationAddress = Find(sections, context->exeBase, registration, registrationMask);
    if (!getterAddress || !registrationAddress) return false;
    // Fail closed if the public record reader has changed layout, even when
    // its entry prologue still matches. These are mov rdx,[r14+field] reads.
    constexpr size_t fieldInstructions[] = {0x1BB, 0x1CA, 0x1D9, 0x1E8};
    constexpr uint32_t fields[] = {0x188, 0x190, 0x198, 0x1A0};
    for (size_t i = 0; i < std::size(fields); ++i) {
        const auto* instruction = reinterpret_cast<const uint8_t*>(registrationAddress + fieldInstructions[i]);
        uint32_t offset{};
        if (!D2RLImage::Readable(instruction, 7) || instruction[0] != 0x49 || instruction[1] != 0x8B || instruction[2] != 0x96) return false;
        std::memcpy(&offset, instruction + 3, sizeof(offset));
        if (offset != fields[i]) return false;
    }
    getParser = reinterpret_cast<GetParser>(getterAddress);
    registerCommand = reinterpret_cast<RegisterCommand>(registrationAddress);
    D2RL::LogInfoF(context, "Floating Damage: chat getter RVA=0x%X registration RVA=0x%X caller RVA=0x%X; layout guards matched.",
        static_cast<uint32_t>(getterAddress-context->exeBase), static_cast<uint32_t>(registrationAddress-context->exeBase), callers[0]);
    return true;
}
}

NativeCommands::~NativeCommands() { Stop(); }

bool NativeCommands::Start(const D2RL::PluginContext* context) {
    const std::lock_guard guard(lock);
    if (context_) { reason_ = "already started"; return false; }
    const D2RL::ThreadService* threads{};
    if (!context || context->QueryService(&threads) != D2RL::ServiceQueryResult::Success
        || !D2RL::HasThreadServiceField(threads, D2RL::ThreadServiceRequiredSize) || !threads->runOnUiThread) {
        reason_ = "UI thread service unavailable"; return false;
    }
    if (!Resolve(context)) { reason_ = "native parser signatures not found uniquely; unsupported game build"; return false; }
    HMODULE module{};
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&Literal), &module)) {
        reason_ = "could not pin native callback module"; return false;
    }
    context_ = context;
    reason_.clear();
    return true;
}

bool NativeCommands::Register(const std::string& name, NativeCommandCallback callback, void* userData,
    const std::string& description, NativeCommandCallback help) {
    const std::lock_guard guard(lock);
    if (!context_) { reason_ = "not started"; return false; }
    if (!callback || name.empty() || name.size() > 31 || name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-") != std::string::npos
        || description.size() > 63 || description.find('\0') != std::string::npos) {
        reason_ = "invalid name, description, or callback"; return false;
    }
    for (const auto& slot : slots) {
        if (slot.used && name == slot.definition.name) { reason_ = "name already reserved by this DLL"; return false; }
    }
    size_t index = 0;
    while (index < Capacity && slots[index].used) ++index;
    if (index == Capacity) { reason_ = "64 native command slots exhausted in this DLL"; return false; }
    auto& slot = slots[index];
    slot.used = true;
    slot.owner = this;
    slot.context = context_;
    slot.callback = callback; slot.help = help; slot.userData = userData;
    std::memcpy(slot.definition.name, name.c_str(), name.size() + 1);
    std::memcpy(slot.definition.description, description.c_str(), description.size() + 1);
    slot.definition.execute = executeCallbacks[index];
    slot.definition.help = helpCallbacks[index];
    slot.definition.translate = Literal;
    slot.status.state = NativeCommandState::Pending;
    const D2RL::ThreadService* threads{};
    const auto query = context_->QueryService(&threads);
    if (query != D2RL::ServiceQueryResult::Success || !D2RL::HasThreadServiceField(threads, D2RL::ThreadServiceRequiredSize)
        || !threads->runOnUiThread) {
        slot = Slot{};
        reason_ = "UI thread service unavailable at registration"; return false;
    }
    slot.threads = threads;
    // Listener registration is supported during plugin load; queuing UI work
    // may not be. Keep the command pending until a later event accepts it.
    const D2RL::SharedEventService* events{};
    if (context_->QueryService(&events) != D2RL::ServiceQueryResult::Success
        || !D2RL::HasSharedEventServiceField(events, D2RL::SharedEventServiceRequiredSize)
        || !events->registerUiMessageListener || !events->unregisterUiMessageListener) {
        slot = Slot{};
        reason_ = "UI event service unavailable for deferred registration"; return false;
    }
    slot.events = events;
    const D2RL::SharedEvents::UiMessageListener listener{sizeof(listener),0,0,0,OnUiMessage,&slot};
    const auto listenResult = events->registerUiMessageListener(context_, &listener, &slot.listener);
    if (listenResult != D2RL::SharedEvents::Result::Success) {
        slot = Slot{};
        reason_ = "could not subscribe for deferred registration: result=" + std::to_string(static_cast<uint32_t>(listenResult));
        return false;
    }
    if (!Queue(slot)) D2RL::LogInfoF(context_, "chat-sdk: native /%s pending; UI queue result=%u (retry on UI event)", name.c_str(), slot.status.queueResult);
    reason_.clear();
    return true;
}

NativeCommandStatus NativeCommands::Status(const std::string& name) const {
    const std::lock_guard guard(lock);
    for (const auto& slot : slots) if (slot.owner == this && name == slot.definition.name) return slot.status;
    return {};
}
std::string NativeCommands::Reason() const { const std::lock_guard guard(lock); return reason_; }
void NativeCommands::Stop() noexcept {
    std::unique_lock guard(lock);
    struct Removal { const D2RL::SharedEventService* events; const D2RL::PluginContext* context; uint64_t handle; };
    std::array<Removal, Capacity> removals{};
    size_t count = 0;
    for (auto& slot : slots) if (slot.owner == this) {
        if (slot.listener) removals[count++] = {slot.events, slot.context, slot.listener};
        slot.listener = 0;
        slot.status.state = NativeCommandState::Stopped;
        slot.callback = nullptr; slot.help = nullptr; slot.userData = nullptr; slot.context = nullptr;
        // Clear ownership so a new object at this address cannot inherit entries.
        slot.owner = nullptr;
    }
    context_ = nullptr;
    guard.unlock();
    for (size_t i = 0; i < count; ++i) removals[i].events->unregisterUiMessageListener(removals[i].context, removals[i].handle);
}
}
