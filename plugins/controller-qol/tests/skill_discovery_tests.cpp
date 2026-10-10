#include "skill_discovery.h"
#include "skill_profile_policy.h"
#include "config_file.h"
#include "aim_config.h"
#include <cstdlib>
#include <memory>

static void Check(bool ok,const char* message){if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
static const std::string sourceText="skill\t*Id\tskilldesc\tpassive\twarp\tsrvdofunc\n"
    "Multiple Shot\t12\tmultiple\t\t\t8\nWarp\t429\twarp\t\t1\t27\n"
    "Custom passive\t500\tpassive\t1\t\t0\nNew projectile\t501\tprojectile\t\t\t1\n"
    "Internal\t502\t\t\t\t1\nUtility\t359\tnative\t\t\t1\n";
static const std::string mainText="# My settings\r\n[aim]\r\ndeadzone=0.31\r\ninitial_speed=13\r\nmaximum_speed=51\r\nacceleration_seconds=0.62\r\n"
    "ground_reticle_color=\"#123456\"\r\nwhirlwind_pass_through_enabled=false\r\nwhirlwind_pass_through_distance=2.1234567\r\n[aim.amazon]\r\n\"12\"=false # mine\r\n[aim.targeting]\r\n\"12\"=\"ground\"\r\n[aim.leading]\r\n\"12\"=17\r\n";
static D2RL::DataTables::Result __cdecl Find(const D2RL::PluginContext*,D2RL::DataTables::Bank bank,D2RL::DataTables::TableId table,uint32_t id,D2RL::DataTables::RowView* row) noexcept {
    static int dummy;
    if(id!=429)return D2RL::DataTables::Result::NotFound;
    row->row=&dummy;row->rowSize=sizeof(dummy);row->revision=1;row->bank=bank;row->tableId=table;return D2RL::DataTables::Result::Success;
}
static D2RL::DataTableService tables{sizeof(tables),D2RL::DataTableService::AbiVersion,nullptr,nullptr,&Find,nullptr};
static D2RL::ServiceQueryResult __cdecl Query(const D2RL::PluginContext*,D2RL::ServiceId,uint32_t,const void** result) noexcept {*result=&tables;return D2RL::ServiceQueryResult::Success;}
static D2RL::Threads::Result __cdecl Run(const D2RL::PluginContext* ctx,D2RL::Threads::Callback callback,void* data) noexcept {callback(ctx,data);return D2RL::Threads::Result::Success;}
int main(int argc,char** argv) {
    using namespace Aim;
    std::vector<Discovery::Skill> source;
    Check(Discovery::ReadSource(sourceText,source) && source.size()==4,"source excludes native utility and internal unnamed rows");
    Check(!Discovery::ReadSource(sourceText+"Duplicate\t429\tx\t\t\t1\n",source),"duplicate IDs rejected");
    Check(!Discovery::ReadSource("skill\t*Id\nx\t429\n",source),"unknown schema rejected");
    SkillSettings defaults,merged,effective;
    Check(Discovery::Defaults(source,defaults) && !defaults.Enabled(429) && defaults.Targeting(429)==TargetMode::Ground && !defaults.CanToggle(500),"new skill defaults off; reviewed Warp ground; passive locked");
    MotionSettings motion;bool enabled{};
    Check(ParseQolSettings(mainText,motion,enabled,&merged,&defaults) && motion.initialSpeed==13 && motion.maximumSpeed==51 && motion.deadzone==0.31f,"main cursor tuning retained");
    Check(!merged.Enabled(12) && merged.Targeting(12)==TargetMode::Ground && merged.LeadMillisecondsPerTile(12)==17,"main skill overrides win");
    std::string updated;
    const std::string profile="# user note\r\n[aim.leading]\r\n\"12\"=25 # manual\r\n";
    Check(Discovery::RewriteOverride(profile,merged,12,true,updated) && updated.starts_with(profile),"R3 appends only its key, preserving unrelated bytes");
    Check(ParseSkills(updated,effective,&merged) && effective.Mode(12)==TargetMode::Ground && effective.LeadMillisecondsPerTile(12)==25,"profile wins without resetting inherited targeting");
    Check(Discovery::RewriteOverride(updated,merged,429,true,updated) && ParseSkills(updated,effective,&merged) && effective.Mode(429)==TargetMode::Ground,"new custom R3 key persists reviewed targeting");
    const std::string legacy="[aim.custom]\n\"501\"=\"ground\" # keep me\n";
    Check(Discovery::RewriteOverride(legacy,merged,501,false,updated) && ParseSkills(updated,effective,&merged) && !effective.Enabled(501) && effective.Targeting(501)==TargetMode::Ground,"legacy custom ground survives off toggle");
    Check(!Discovery::RewriteOverride("[aim.typo]\n\"429\"=true",merged,429,true,updated),"invalid profile never rewritten");
    Check(!Discovery::RewriteOverride("[aim]\ninitial_speed=22",merged,429,true,updated),"cursor settings cannot leak into skill profile");
    Check(!Discovery::RewriteOverride("",merged,500,true,updated),"passives stay locked");
    Check(!Discovery::RewriteOverride("",merged,555,true,updated),"unknown ID not added by R3");
    Check(!ParseSkills("[aim.whirlwind]\nwhirlwind_pass_through_distance=nan\n",effective) &&
        !ParseSkills("[aim.whirlwind]\nwhirlwind_pass_through_distance=16\n",effective) &&
        !ParseSkills("[aim.whirlwind]\nwhirlwind_pass_through_enabled=true\nwhirlwind_pass_through_enabled=false\n",effective),"Whirlwind profiles reject invalid or duplicate options");
    const auto folder=std::filesystem::temp_directory_path()/(L"qol-discovery-tests-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
    const auto mod=folder/L"Pack";const auto config=folder/L"config/controller-qol-updates.toml";
    Check(ConfigFile::Write(mod/L"Pack.mpq/data/global/excel/skills.txt","",sourceText),"fixture source write");
    Check(ConfigFile::Write(config,"",mainText),"fixture main write");
    auto identity=std::filesystem::weakly_canonical(mod).generic_u8string();
    std::string key(reinterpret_cast<const char*>(identity.data()),identity.size());key+="|Pack";
    for(auto& ch:key)if(ch>='A' && ch<='Z')ch=static_cast<char>(ch-'A'+'a');
    const auto betaProfile=config.parent_path()/L"controller-qol-skills"/ConfigFile::Utf8Path(Discovery::Fingerprint(key)+".overrides.toml");
    Check(ConfigFile::Write(betaProfile,"","# Keep this R3 choice\r\n[aim.custom]\r\n\"429\" = true # Warp on\r\n"),"existing beta R3 profile fixture");
    D2RL::PluginApi api{};api.apiSize=sizeof(api);api.queryService=Query;
    D2RL::PluginContext ctx{};ctx.contextSize=sizeof(ctx);ctx.api=&api;ctx.activeMod="Pack";ctx.modDirectory=mod.c_str();ctx.pluginConfigPath=config.c_str();
    auto live=std::make_unique<LiveSkills>();Check(ParseSkills(mainText,live->baseline),"legacy baseline parses");
    Check(QolSkillDiscovery::Prepare(&ctx,mainText.c_str(),*live),"legacy configuration migrated successfully");
    Check(!live->baseline.whirlwindPassThrough && live->baseline.whirlwindPassThroughDistance==2.1234567f,"Whirlwind options migrated without precision loss");
    Check(QolSkillDiscovery::Active() && live->Contains(429) && !live->CanToggle(429),"automatic ID requires runtime confirmation");
    Check(live->baseline.Enabled(429),"preexisting beta R3 enable survives migration");
    Check(!live->Enabled(429),"runtime mode publication cannot bypass pending confirmation");
    D2RL::LifecycleService lifecycle{};D2RL::ThreadService threads{sizeof(threads),D2RL::ThreadService::AbiVersion,nullptr,Run};
    QolSkillDiscovery::Register(&ctx,&lifecycle,&threads);
    Check(live->CanToggle(429) && !live->CanToggle(501),"SDK confirmation admits only found IDs");
    Check(live->Enabled(429) && live->Targeting(429)==TargetMode::Ground,"migrated beta choice becomes active after confirmation");
    live->Confirm(501);Check(live->CanToggle(501),"guarded UI observation can confirm ID without game scheduler");
    Check(QolSkillDiscovery::Save(429,true) && QolSkillDiscovery::Save(429,false),"two R3 writes succeed");
    std::string preserved;Check(ConfigFile::Read(config,preserved,65535) && Profiles::Split(preserved).skills.empty(),"all per-skill sections moved out of main config");
    const auto sharedText=preserved;
    Check(sharedText.find("whirlwind_pass_through")==sharedText.npos,"no skill-specific Whirlwind options remain in main");
    Check(sharedText.ends_with(Profiles::Split(mainText).shared),"shared setting bytes and comments preserved");
    Check(ParseQolSettings(sharedText,motion,enabled) && motion.initialSpeed==13 && motion.deadzone==0.31f,"shared tuning preserved after migration");
    auto profiles=folder/L"config/controller-qol-skills";
    std::filesystem::path overridePath,catalogPath;unsigned backups=0;
    for(const auto& file:std::filesystem::directory_iterator(profiles)) {
        const auto name=file.path().filename().string();
        if(name.ends_with(".overrides.toml"))overridePath=file.path();
        if(name.ends_with(".catalog.toml"))catalogPath=file.path();
        if(name.ends_with(".bak"))++backups;
    }
    Check(backups==2 && !overridePath.empty() && !catalogPath.empty(),"migration backup and one rolling R3 backup exist");
    std::string before;Check(ConfigFile::Read(overridePath,before,65535),"read saved profile");
    Check(before.find("# Keep this R3 choice")!=before.npos && before.find("# mine")!=before.npos,"profile and migrated setting comments survive");
    Check(!ConfigFile::Write(overridePath,"stale snapshot","bad"),"concurrent edit detected without overwrite");
    auto rolling=overridePath;rolling+=L".r3.bak";
    std::vector<std::pair<std::filesystem::path,std::string>> migrationBackups;
    for(const auto& file:std::filesystem::directory_iterator(profiles))if(file.path().extension()==L".bak" && file.path()!=rolling) {
        std::string saved;Check(ConfigFile::Read(file.path(),saved,65535),"read migration backup");
        migrationBackups.emplace_back(file.path(),saved);
    }
    for(int i=0;i<100;++i)Check(QolSkillDiscovery::Save(429,i%2==0),"rapid R3 toggle save");
    unsigned afterSpam=0;
    for(const auto& file:std::filesystem::directory_iterator(profiles))if(file.path().extension()==L".bak")++afterSpam;
    Check(afterSpam==backups,"100 toggles do not grow the backup count");
    std::string previous;Check(ConfigFile::Read(rolling,previous,65535) && ParseSkills(previous,effective) && effective.Enabled(429),"rolling backup is the immediately preceding setting");
    Check(ConfigFile::Read(overridePath,preserved,65535) && preserved==before,"rapid toggles preserve all other profile bytes");
    for(const auto& [path,saved]:migrationBackups)Check(ConfigFile::Read(path,preserved,65535) && preserved==saved,"R3 never rotates migration recovery backups");
    Check(!ConfigFile::WriteRolling(overridePath,"stale snapshot","bad") && ConfigFile::Read(rolling,preserved,65535) && preserved==previous,"stale R3 writes leave the backup intact");
    Check(QolSkillDiscovery::Save(429,false) && ConfigFile::Read(rolling,preserved,65535) && preserved==previous,"unchanged R3 save does not rotate backup");
    const auto backupHeld=CreateFileW(rolling.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    Check(backupHeld!=INVALID_HANDLE_VALUE,"hold rolling backup against replacement");
    Check(!QolSkillDiscovery::Save(429,true),"unwritable rolling backup rejects R3 save");CloseHandle(backupHeld);
    Check(ConfigFile::Read(overridePath,preserved,65535) && preserved==before,"backup failure preserves current profile");
    QolSkillDiscovery::Stop();Check(!QolSkillDiscovery::Save(429,true),"stopped discovery rejects writes");
    auto restarted=std::make_unique<LiveSkills>();ParseSkills(mainText,restarted->baseline);
    Check(QolSkillDiscovery::Prepare(&ctx,sharedText.c_str(),*restarted),"restart reads migrated profile");restarted->Confirm(429);
    Check(!restarted->Enabled(429) && restarted->Targeting(429)==TargetMode::Ground,"profile survives restart");
    Check(ConfigFile::Read(overridePath,preserved,65535) && preserved==before,"restart does not regenerate user profile");
    Check(!restarted->baseline.whirlwindPassThrough && restarted->baseline.whirlwindPassThroughDistance==2.1234567f,"R3 and restart preserve Whirlwind options");
    const auto another=folder/L"Another";Check(ConfigFile::Write(another/L"data/global/excel/skills.txt","",sourceText),"second source fixture");
    ctx.activeMod="Another";ctx.modDirectory=another.c_str();QolSkillDiscovery::Stop();
    auto other=std::make_unique<LiveSkills>();ParseSkills(mainText,other->baseline);Check(QolSkillDiscovery::Prepare(&ctx,sharedText.c_str(),*other),"new mod seeded from legacy migration archive");
    Check(QolSkillDiscovery::Active() && QolSkillDiscovery::Save(429,true),"second mod has separate profile");
    Check(ConfigFile::Read(overridePath,preserved,65535) && preserved==before,"other mod never changes original overrides");
    QolSkillDiscovery::Stop();
    // Malformed user profile must remain untouched and preserve legacy settings.
    Check(ConfigFile::Write(overridePath,before,"[aim.typo]\n\"429\"=true\n"),"malformed profile fixture");
    ctx.activeMod="Pack";ctx.modDirectory=mod.c_str();
    auto invalid=std::make_unique<LiveSkills>();ParseSkills(mainText,invalid->baseline);
    Check(!QolSkillDiscovery::Prepare(&ctx,sharedText.c_str(),*invalid),"invalid profile rejects aim initialization");
    Check(!QolSkillDiscovery::Active() && !invalid->Contains(429) && !invalid->Enabled(12),"malformed profile leaves the caller baseline untouched");
    Check(ConfigFile::Read(overridePath,preserved,65535) && preserved=="[aim.typo]\n\"429\"=true\n","invalid profile is never replaced");
    ctx.activeMod=nullptr;ctx.modDirectory=nullptr;
    auto vanilla=std::make_unique<LiveSkills>();
    Check(QolSkillDiscovery::Prepare(&ctx,sharedText.c_str(),*vanilla),"vanilla has its own profile without loose source");
    Check(!vanilla->Enabled(12) && vanilla->Targeting(12)==TargetMode::Ground && vanilla->LeadMillisecondsPerTile(12)==17,"legacy preferences preserved for later vanilla profile");
    Check(QolSkillDiscovery::Save(12,true),"vanilla R3 saves to profile");
    Check(ConfigFile::Read(config,preserved,65535) && preserved==sharedText,"vanilla R3 leaves shared config unchanged");
    QolSkillDiscovery::Stop();
    // New installs use shipped skill defaults rather than an empty legacy layer.
    const auto freshConfig=folder/L"fresh/controller-qol-updates.toml";
    Check(ConfigFile::Write(freshConfig,"","[aim]\nenabled=true\n"),"fresh shared config fixture");
    ctx.pluginConfigPath=freshConfig.c_str();
    auto fresh=std::make_unique<LiveSkills>();
    Check(QolSkillDiscovery::Prepare(&ctx,"[aim]\nenabled=true\n",*fresh),"fresh vanilla profile created");
    Check(!fresh->CanToggle(9) && fresh->LeadMillisecondsPerTile(39)==100 && fresh->LeadMaxMilliseconds(39)==1200,"fresh profile uses all shipped skill defaults");
    QolSkillDiscovery::Stop();
    // A main-file replacement failure after saving the profile must be recoverable.
    const auto blockedConfig=folder/L"blocked/controller-qol-updates.toml";
    Check(ConfigFile::Write(blockedConfig,"",mainText),"interrupted migration fixture");
    ctx.pluginConfigPath=blockedConfig.c_str();
    const auto held=CreateFileW(blockedConfig.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    Check(held!=INVALID_HANDLE_VALUE,"hold main file against replacement");
    auto retry=std::make_unique<LiveSkills>();
    Check(!QolSkillDiscovery::Prepare(&ctx,mainText.c_str(),*retry),"failed main replacement stops aim initialization");
    CloseHandle(held);
    Check(ConfigFile::Read(blockedConfig,preserved,65535) && preserved==mainText,"interrupted migration retains original main config");
    Check(QolSkillDiscovery::Prepare(&ctx,mainText.c_str(),*retry),"interrupted migration safely resumes");
    Check(!retry->Enabled(12) && retry->Targeting(12)==TargetMode::Ground && retry->LeadMillisecondsPerTile(12)==17,"resumed migration retains skill settings");
    QolSkillDiscovery::Stop();
    if(argc==2) {
        std::string actual;Check(ConfigFile::Read(ConfigFile::Utf8Path(argv[1]),actual,8*1024*1024),"read supplied active mod source");
        Check(Discovery::ReadSource(actual,source) && Discovery::Defaults(source,defaults),"active mod source schema and capacity validated");
        std::printf("Active source catalog: %zu eligible rows.\n",source.size());
    }
    // Only the unique fixture directory constructed above is removed.
    Check(std::filesystem::weakly_canonical(folder).parent_path()==std::filesystem::weakly_canonical(std::filesystem::temp_directory_path()),"cleanup target is unique fixture under temp");
    std::filesystem::remove_all(folder);
    std::puts("Skill discovery, layering, SDK/UI confirmation, backups and mod isolation passed.");
}
