#include "config_reload.h"
#include "controller_input.h"
#include "qol_navigation.h"
#include "loot_performance.h"
#include "aim/config_file.h"
#include "aim/skill_discovery.h"
#include "aim/skill_profile_policy.h"
#include "aim/live_settings.h"
#include "aim/controller_aim.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace {
const D2RL::PluginContext* owner{};
QolConfig::Settings startup{};
std::atomic<std::shared_ptr<const QolConfig::Settings>> published{};
std::atomic<bool> running{},requested{};
std::thread worker;
std::condition_variable wake;
std::mutex wakeMutex;
QolReload::Idle idle{};
QolReload::Apply apply{};
QolConfig::Watcher watcher;
std::uint64_t generation{};

struct Prepared {
    std::shared_ptr<const QolConfig::Settings> settings;
    QolAim::Reload aim;
    QolConfig::Files files;
    std::string restart;
    bool manual{},committed{};
};
std::unique_ptr<Prepared> pending;

QolConfig::Stamp FileStamp(const std::filesystem::path& path) noexcept {
    if(path.empty())return {};
    std::error_code error;
    if(!std::filesystem::is_regular_file(path,error) || error)return {};
    const auto size=std::filesystem::file_size(path,error);if(error)return {};
    const auto time=std::filesystem::last_write_time(path,error);if(error)return {};
    return {true,size,time};
}
QolConfig::Files Files() {
    return {FileStamp(owner && owner->pluginConfigPath?owner->pluginConfigPath:std::filesystem::path{}),
            FileStamp(QolSkillDiscovery::ProfilePath())};
}
void Report(const std::string& message,bool manual,bool error=false) noexcept {
    if(!owner)return;
    if(error)owner->LogWarn(message.c_str());else owner->LogInfo(message.c_str());
    if(manual)owner->WriteConsoleMessage(message.c_str(),error?D2RL::ConsoleMessageKind::Warning:D2RL::ConsoleMessageKind::Output);
}
void Commit(void* value) noexcept {
    auto& next=*static_cast<Prepared*>(value);
    if(!QolAim::ApplyReload(next.aim))return; // An intervening R3 save wins.
    if(apply)apply(*next.settings);
    published.store(next.settings,std::memory_order_release);
    next.committed=true;
}
// Deterministic worker step, also exercised by the offline integration tests.
void Poll(std::uint64_t now) {
    const auto files=Files();
    bool manual=requested.exchange(false);
    const bool automatic=watcher.Observe(files,now);
    if(pending && !(pending->files==files)) {
        manual=manual || pending->manual;
        pending.reset();
    }
    if(manual || automatic) {
        auto next=std::make_unique<Prepared>();next->files=files;next->manual=manual;
        std::string text,error;
        QolConfig::Settings settings;
        const bool read=owner && owner->pluginConfigPath &&
            Aim::ConfigFile::Read(owner->pluginConfigPath,text,65535);
        if(text.starts_with("\xef\xbb\xbf"))text.erase(0,3);
        bool valid=read && QolConfig::Parse(text,settings,error);
        if(valid && !Aim::Profiles::Split(text).skills.empty()) {
            error="skill sections belong in the existing active per-mod profile; legacy migration requires restart";valid=false;
        }
        if(valid)valid=QolAim::PrepareReload(text,next->aim,error);
        const auto after=Files();
        if(!(files==after)) {
            // Discard a partial editor/R3 write; the combined watcher will settle.
            watcher.Observe(after,now);
            if(manual)requested.store(true);
            pending.reset();return;
        }
        watcher.Resync(files);
        if(!valid) {
            pending.reset();
            Report("[QOL/Reload] Refused: "+(read?error:"main configuration is missing, unreadable or oversized")+". Last valid settings retained.",manual,true);
            return;
        }
        auto effective=QolConfig::LiveSettings(settings,startup,next->restart);
        if(!next->aim.restart.empty()) {
            if(!next->restart.empty())next->restart+=", ";
            next->restart+=next->aim.restart;
        }
        next->settings=std::make_shared<const QolConfig::Settings>(effective);
        pending=std::move(next);
    }
    if(!pending || (idle && !idle()))return;
    if(!ControllerQoL::ApplyConfigurationWhenIdle(&Commit,pending.get()))return;
    if(!pending->committed) {
        requested.store(true);pending.reset();return;
    }
    ++generation;
    std::string message="[QOL/Reload] Applied settings generation "+std::to_string(generation)+".";
    if(!pending->restart.empty())message+=" Restart required for: "+pending->restart+". Active hooks unchanged.";
    Report(message,pending->manual);
    pending.reset();
}
}

std::shared_ptr<const QolConfig::Settings> QolReload::Snapshot() noexcept {
    return published.load(std::memory_order_acquire);
}
bool QolReload::Request() noexcept {
    if(!running.load())return false;
    requested.store(true);wake.notify_one();return true;
}
D2RL::ConsoleCommandResult QolReload::Command(D2R::Game::Client* client,const D2RL::ConsoleCommandContext* command,void* data) noexcept {
    if(!command || (command->argsLength && !command->args) || command->argsLength>256)return D2RL::ConsoleCommandResult::InvalidArguments;
    const auto args=QolAim::Trim(std::string_view(command->args?command->args:"",command->argsLength));
    if(args=="perf") {
        QolPerf::Report(command->plugin?command->plugin:owner);
        QolAim::ReportStatus(command->plugin?command->plugin:owner);
        return D2RL::ConsoleCommandResult::Handled;
    }
    if(args!="reload")return QolNavigation::HandleCommand(client,command,data);
    if(!Request()) {
        if(owner)owner->WriteConsoleMessage("QOL reload unavailable; restart the game.",D2RL::ConsoleMessageKind::Warning);
        return D2RL::ConsoleCommandResult::Failed;
    }
    owner->WriteConsoleMessage("QOL reload requested. Changes apply after held controls and pending item actions settle.");
    return D2RL::ConsoleCommandResult::Handled;
}
bool QolReload::Start(const D2RL::PluginContext* context,const QolConfig::Settings& settings,Idle canApply,Apply update) noexcept {
    if(running.load() || worker.joinable())return false;
    try {
        owner=context;startup=settings;idle=canApply;apply=update;
        if(!owner || !owner->pluginConfigPath || !*owner->pluginConfigPath)return false;
        published.store(std::make_shared<const QolConfig::Settings>(startup));
        pending.reset();requested.store(false);generation=0;watcher.Resync(Files());
        running.store(true);
        worker=std::thread([] {
            while(running.load()) {
                try {
                    const auto now=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
                    Poll(now);
                }catch(...) {
                    const bool manual=pending && pending->manual;
                    pending.reset();Report("[QOL/Reload] Reload failed. Last valid settings retained.",manual,true);
                }
                std::unique_lock lock(wakeMutex);
                wake.wait_for(lock,std::chrono::milliseconds(200),[]{return !running.load() || requested.load();});
            }
        });
        owner->LogInfo("[QOL/Reload] Automatic main/profile reload enabled (650 ms stable file); hook configuration remains restart-only.");
        return true;
    }catch(...) {running.store(false);if(owner)owner->LogWarn("[QOL/Reload] Watcher unavailable; keep the current settings and restart after edits.");return false;}
}
void QolReload::Stop() noexcept {
    running.store(false);requested.store(false);wake.notify_all();
    if(worker.joinable())worker.join();
    pending.reset(); // Keep published data alive for late resident callbacks.
}
