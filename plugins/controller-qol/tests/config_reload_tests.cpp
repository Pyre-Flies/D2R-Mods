// Actual file-watching/transaction/command implementation, private module seams.
// No game process, native hook, installed config or network endpoint is used.
#include "../src/config_reload.cpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <source_location>
#include <vector>

namespace Fixture {
std::filesystem::path folder,mainPath,profile;
std::mutex messagesMutex;
std::vector<std::string> messages;
std::atomic<bool> held{},busy{},rejectCommit{};
std::atomic<unsigned> commits{},delegated{};
Aim::MotionSettings motion;
bool skillEnabled=true;
unsigned serial{};
void Check(bool valid,std::source_location at=std::source_location::current()) {
    if(!valid){std::fprintf(stderr,"Reload check failed at %u\n",at.line());std::exit(1);}
}
void Log(const char* text) {std::lock_guard lock(messagesMutex);messages.emplace_back(text?text:"");}
void __cdecl Info(const D2RL::PluginContext*,const char* text) noexcept {Log(text);}
void __cdecl Console(const D2RL::PluginContext*,const char* text,D2RL::ConsoleMessageKind) noexcept {Log(text);}
bool Has(std::string_view text) {std::lock_guard lock(messagesMutex);for(const auto& message:messages)if(message.find(text)!=message.npos)return true;return false;}
std::size_t MessageCount() {std::lock_guard lock(messagesMutex);return messages.size();}
void Write(const std::filesystem::path& path,const std::string& text) {
    std::ofstream file(path,std::ios::binary|std::ios::trunc);file<<text;file.close();Check(bool(file));
    std::filesystem::last_write_time(path,std::filesystem::file_time_type::clock::now()+std::chrono::seconds(++serial));
}
std::string Main(unsigned distance=6,unsigned threshold=30,const std::string& extra="") {
    return "[qol]\nground_pickup_distance="+std::to_string(distance)+"\ntrigger_threshold="+std::to_string(threshold)+"\n"+extra+
        "[aim]\nenabled=true\ndeadzone=0.4\n";
}
bool Idle() noexcept {return !busy.load();}
void Updated(const QolConfig::Settings&) noexcept {++commits;}
void PolicyTests() {
    QolConfig::Settings parsed;std::string error;
    std::string shipped;Aim::MotionSettings aim;bool enabled{};
    Check(Aim::ConfigFile::Read(std::filesystem::path(__FILE__).parent_path().parent_path()/"controller-qol-updates.toml",shipped,65535) &&
        QolConfig::Parse(shipped,parsed,error) && Aim::ParseQolSettings(shipped,aim,enabled));
    Check(QolConfig::Parse("\xef\xbb\xbf[qol]\r\n# enabled=false\r\nenabled=true # yes\r\nground_pickup_button=\"LB\"\r\nground_pickup_distance=12\r\ntrigger_threshold=120\r\n[aim]\nenabled=true\n",parsed,error));
    Check(parsed.enabled && parsed.groundPickupDistance==12 && parsed.triggerThreshold==120 && std::string(parsed.groundPickupButton)=="lb");
    for(const auto bad:{"", "[aim]\nenabled=true", "[qol]\nenabled=true\nenabled=false", "[qol]\nenabled=true junk", "[qol]\nground_pickup_distance=2x", "[qol]\ntrigger_threshold=256", "[qol]\ntrigger_threshold=-1", "[qol]\ntrigger_threshold=0", "[qol]\ntrigger_threshold=999999999999999999999", "[qol]\nmodifier=\"nope\"", "[qol]\nunknown=true", "[qol]\n[aim]\n[aim]", "[qol]\n[aim", "[typo]\nenabled=true"}) {
        Check(!QolConfig::Parse(bad,parsed,error));Check(!error.empty());
        Check(parsed.groundPickupDistance==12 && parsed.triggerThreshold==120); // no partial output
    }
    QolConfig::Settings baseline,desired=baseline;desired.enabled=false;desired.groundPickup=false;
    desired.groundPickupDistance=15;desired.triggerThreshold=80;desired.quickIdentify=false;
    std::string restart;const auto live=QolConfig::LiveSettings(desired,baseline,restart);
    Check(live.enabled && live.groundPickup && live.groundPickupDistance==15 && live.triggerThreshold==80 && !live.quickIdentify);
    Check(restart.find("qol.enabled")!=restart.npos && restart.find("ground_pickup")!=restart.npos);
    QolConfig::Watcher watch;QolConfig::Files a,b;b.main.present=true;b.main.size=3;
    Check(!watch.Observe(a,0) && !watch.Observe(b,1) && !watch.Observe(b,650));
    Check(watch.Observe(b,651) && !watch.Observe(b,9999));
}
}
namespace ControllerQoL {
bool ApplyConfigurationWhenIdle(ConfigurationUpdate callback,void* data) noexcept {
    if(Fixture::held.load())return false;callback(data);return true;
}
}
namespace QolNavigation {
D2RL::ConsoleCommandResult HandleCommand(D2R::Game::Client*,const D2RL::ConsoleCommandContext*,void*) noexcept {
    ++Fixture::delegated;return D2RL::ConsoleCommandResult::Handled;
}
}
namespace QolSkillDiscovery {
std::filesystem::path ProfilePath(){return Fixture::profile;}
}
namespace QolAim {
void ReportStatus(const D2RL::PluginContext* logger) noexcept {
    if(logger)logger->LogInfo("QOL_AIM_STATE fixture=1");
}
bool PrepareReload(std::string_view text,Reload& next,std::string& error) noexcept {
    auto settings=std::make_shared<Aim::MotionSettings>();bool configured{};
    auto skills=std::make_shared<Aim::SkillSettings>();std::string profileText;
    if(!Aim::ParseQolSettings(text,*settings,configured) ||
       !Aim::ConfigFile::Read(Fixture::profile,profileText,65535) || profileText.empty() ||
       !Aim::Discovery::ProfileOnly(profileText) || !Aim::ParseSkills(profileText,*skills)) {
        error="invalid aim/profile fixture";return false;
    }
    next.motion=std::move(settings);next.skills=std::move(skills);return true;
}
bool ApplyReload(const Reload& next) noexcept {
    if(Fixture::rejectCommit.exchange(false))return false;
    Fixture::motion=*next.motion;Fixture::skillEnabled=next.skills->Enabled(12);return true;
}
}
int main() {
    using namespace Fixture;
    PolicyTests();
    folder=std::filesystem::temp_directory_path()/(L"qol-reload-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
    Check(!std::filesystem::exists(folder));std::filesystem::create_directories(folder);
    mainPath=folder/L"controller-qol-updates.toml";profile=folder/L"existing.overrides.toml";
    Write(mainPath,Main());Write(profile,"[aim.amazon]\n\"12\"=true\n");
    D2RL::PluginApi api{};api.apiSize=sizeof(api);api.logInfo=&Info;api.logWarn=&Info;api.writeConsoleMessage=&Console;
    D2RL::PluginContext context{};context.contextSize=sizeof(context);context.api=&api;context.pluginConfigPath=mainPath.c_str();
    owner=&context;startup={};published.store(std::make_shared<const QolConfig::Settings>(startup));
    idle=&Idle;apply=&Updated;watcher.Resync(Files());running.store(true);
    Write(mainPath,Main(12,120));Poll(0);Poll(649);Check(QolReload::Snapshot()->groundPickupDistance==6);
    Poll(650);Check(QolReload::Snapshot()->groundPickupDistance==12 && motion.deadzone==0.4f && commits==1);
    Write(mainPath,Main(9,80,"enabled=false\nground_pickup=false\n"));Poll(700);Poll(1350);
    Check(QolReload::Snapshot()->enabled && QolReload::Snapshot()->groundPickup && QolReload::Snapshot()->groundPickupDistance==9);
    Check(Has("Restart required for:") && Has("Active hooks unchanged"));
    Write(mainPath,"[qol]\nground_pickup_distance=7oops\n[aim]\nenabled=true");Poll(1400);Poll(2050);
    const auto refused=MessageCount();Poll(9999);Check(MessageCount()==refused && QolReload::Snapshot()->groundPickupDistance==9);
    Check(Has("Last valid settings retained"));
    D2RL::ConsoleCommandContext command{};command.args="reload";command.argsLength=6;
    Write(mainPath,Main(10,90));Write(profile,"[aim.amazon]\n\"12\"=garbage\n");
    Check(QolReload::Command(nullptr,&command,nullptr)==D2RL::ConsoleCommandResult::Handled);Poll(10000);
    Check(QolReload::Snapshot()->groundPickupDistance==9 && skillEnabled);
    Write(profile,"[aim.amazon]\n\"12\"=false\n");Poll(10100);Poll(10750);
    Check(QolReload::Snapshot()->groundPickupDistance==10 && !skillEnabled);
    std::filesystem::remove(profile);Poll(10800);Poll(11450);Check(QolReload::Snapshot()->groundPickupDistance==10);
    Write(profile,"[aim.amazon]\n\"12\"=true\n");held=true;Write(mainPath,Main(11,91));Poll(12000);Poll(12650);
    Check(pending && QolReload::Snapshot()->groundPickupDistance==10);
    Write(mainPath,Main(13,92));Poll(12700);Poll(13350);Check(QolReload::Snapshot()->groundPickupDistance==10);
    held=false;Poll(13400);Check(QolReload::Snapshot()->groundPickupDistance==13 && skillEnabled);
    busy=true;Write(mainPath,Main(15,95));Check(QolReload::Request());Poll(13500);
    Check(pending && QolReload::Snapshot()->groundPickupDistance==13);
    busy=false;Poll(13600);Check(QolReload::Snapshot()->groundPickupDistance==15);
    rejectCommit=true;Write(mainPath,Main(14,93));Check(QolReload::Request());Poll(14000);
    Check(QolReload::Snapshot()->groundPickupDistance==15);Poll(14200);Check(QolReload::Snapshot()->groundPickupDistance==14);
    const auto settingsBeforeReport=QolReload::Snapshot();const auto commitsBeforeReport=commits.load();
    command.plugin=&context;command.args="perf";command.argsLength=4;
    Check(QolReload::Command(nullptr,&command,nullptr)==D2RL::ConsoleCommandResult::Handled &&
        Has("QOL_PERF") && delegated==0 && QolReload::Snapshot()==settingsBeforeReport && commits==commitsBeforeReport);
    command.args="status";command.argsLength=6;Check(QolReload::Command(nullptr,&command,nullptr)==D2RL::ConsoleCommandResult::Handled && delegated==1);
    command.args=nullptr;command.argsLength=1;Check(QolReload::Command(nullptr,&command,nullptr)==D2RL::ConsoleCommandResult::InvalidArguments);
    QolReload::Stop();Check(!QolReload::Request());
    // Exercise the real background loop and shutdown, beyond deterministic steps.
    Write(mainPath,Main());Check(QolReload::Start(&context,{},&Idle,&Updated));
    Write(mainPath,Main(17,100));
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while(QolReload::Snapshot()->groundPickupDistance!=17 && std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(10));
    Check(QolReload::Snapshot()->groundPickupDistance==17);QolReload::Stop();
    Check(std::distance(std::filesystem::directory_iterator(folder),std::filesystem::directory_iterator{})==2);
    Check(std::filesystem::weakly_canonical(folder).parent_path()==std::filesystem::weakly_canonical(std::filesystem::temp_directory_path()));
    std::filesystem::remove_all(folder);
    std::puts("Actual reload worker, automatic/manual transactions, error retention, profile changes, held-input deferral, restart-only policy and stop passed.");
}
