#include "skill_discovery.h"
#include "skill_profile_policy.h"
#include "config_file.h"
#include <mutex>

namespace {
const D2RL::PluginContext* context{};
Aim::LiveSkills* live{};
Aim::SkillSettings base{};
std::vector<Aim::Discovery::Skill> source;
std::filesystem::path profilePath;
const D2RL::DataTableService* tables{};
std::atomic<bool> active{}, stopped{true};
std::mutex tableMutex;

void ConfirmTables(const D2RL::PluginContext*,void*) noexcept {
    std::lock_guard guard(tableMutex);
    if(stopped.load() || !tables || !live)return;
    // Some remote clients cannot run this callback. Visible General/Class tree
    // IDs provide independent confirmation on the already guarded UI path.
    for(const auto& skill:source) {
        if(!live->NeedsConfirmation(skill.id))continue;
        for(const auto bank:{D2RL::DataTables::Bank::Rotw,D2RL::DataTables::Bank::Lod,D2RL::DataTables::Bank::Classic}) {
            D2RL::DataTables::RowView row{};row.structSize=sizeof(row);
            if(tables->findRowById(context,bank,D2RL::DataTables::TableId::Skills,static_cast<unsigned>(skill.id),&row)==D2RL::DataTables::Result::Success &&
                D2RL::DataTables::HasRowViewField(&row,D2RL::DataTables::RowViewRequiredSize) && row.tableId==D2RL::DataTables::TableId::Skills && row.bank==bank && row.row && row.rowSize && row.revision) {
                live->Confirm(skill.id);break;
            }
        }
    }
}
void TablesLoaded(const D2RL::PluginContext* ctx,const D2RL::Lifecycle::DataTablesLoadedEvent*,void*) noexcept {
    ConfirmTables(ctx,nullptr);
}
}

bool QolSkillDiscovery::Prepare(const D2RL::PluginContext* ctx,const char* mainConfig,Aim::LiveSkills& skills) noexcept {
    active.store(false);stopped.store(true);
    if(!D2RL::HasContext(ctx) || !ctx->pluginConfigPath || !mainConfig)return false;
    try {
        HMODULE module{};
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&QolSkillDiscovery::Prepare),&module))return false;
        const auto resource=FindResourceW(module,MAKEINTRESOURCEW(4102),MAKEINTRESOURCEW(10));
        const auto data=resource?static_cast<const char*>(LockResource(LoadResource(module,resource))):nullptr;
        if(!data)return false;
        const std::string shipped(data,SizeofResource(module,resource));
        const bool modded=ctx->activeMod && *ctx->activeMod;
        const std::string mod=modded?ctx->activeMod:"Vanilla";
        if(mod.find_first_of("/\\:")!=mod.npos || mod=="." || mod=="..")return false;
        const std::filesystem::path mainPath=ctx->pluginConfigPath;
        const auto root=modded && ctx->modDirectory?std::filesystem::path(ctx->modDirectory):mainPath.parent_path();
        std::vector<Aim::Discovery::Skill> candidate;
        std::string sourceText;
        if(modded && ctx->modDirectory) {
            for(const auto& path:{root/L"data/global/excel/skills.txt",root/Aim::ConfigFile::Utf8Path(mod+".mpq")/L"data/global/excel/skills.txt"}) {
                if(!std::filesystem::is_regular_file(path))continue;
                if(!Aim::ConfigFile::Read(path,sourceText,8*1024*1024) || !Aim::Discovery::ReadSource(sourceText,candidate))
                    ctx->LogWarn("[QOL/Skills] Mod source unavailable or unrecognized; existing/manual skill profile retained.");
                break;
            }
        }
        auto identity=std::filesystem::weakly_canonical(root).generic_u8string();
        std::string key(reinterpret_cast<const char*>(identity.data()),identity.size());key+='|';key+=mod;
        for(auto& ch:key)if(ch>='A' && ch<='Z')ch=static_cast<char>(ch-'A'+'a');
        const auto folder=mainPath.parent_path()/L"controller-qol-skills";
        const auto stem=Aim::ConfigFile::Utf8Path(Aim::Discovery::Fingerprint(key));
        auto profile=folder/stem;profile+=L".overrides.toml";
        auto catalog=folder/stem;catalog+=L".catalog.toml";
        const auto archive=folder/L"migration/legacy-skills.toml";
        const auto parts=Aim::Profiles::Split(mainConfig);
        const bool hasLegacy=!parts.skills.empty();
        std::string archived,overrides;
        if(!Aim::ConfigFile::Read(archive,archived,Aim::Discovery::MaxText,true) ||
            !Aim::ConfigFile::Read(profile,overrides,Aim::Discovery::MaxText,true))return false;
        const bool currentProfile=overrides.starts_with("# Controller QOL skill profile, schema 2.");
        const std::string legacy=hasLegacy?parts.skills:currentProfile?std::string{}:archived;
        Aim::SkillSettings defaults,inherited,effective,verified;
        if(!Aim::Discovery::Defaults(candidate,defaults) ||
            !Aim::Discovery::ProfileOnly(legacy) || !Aim::Discovery::ProfileOnly(overrides) ||
            !Aim::ParseSkills(legacy.empty()?shipped:legacy,inherited,&defaults) ||
            !Aim::ParseSkills(overrides,effective,&inherited))return false;
        // Preserve beta overrides and original comment bytes where keys are copied.
        std::string updated=overrides;
        const std::string oldHeader="# Controller QOL mod skill overrides, schema 1. User-owned; never regenerated.\n# Mod: "+Aim::Discovery::Comment(mod)+
            "\n# R3 saves here. Copy individual keys/sections from the matching .catalog.toml.\n# Priority: built-in/discovered defaults < main TOML < this file.\n# Only aim skill sections are allowed here; cursor/color settings stay in main TOML.\n";
        if(updated.starts_with(oldHeader))updated.erase(0,oldHeader.size());
        if(!updated.starts_with("# Controller QOL skill profile, schema 2."))updated=Aim::Profiles::Header(mod)+updated;
        updated=Aim::Profiles::FillMissing(std::move(updated),legacy.empty()?shipped:legacy);
        updated=Aim::Profiles::FillMissing(std::move(updated),Aim::Profiles::Snapshot(effective,candidate));
        if(updated.size()>=Aim::Discovery::MaxText || !Aim::ParseSkills(updated,verified,&defaults) || !Aim::Profiles::Equivalent(effective,verified))return false;
        // A migration archive seeds other mod profiles later. Never replace an
        // earlier, different legacy snapshot silently.
        if(hasLegacy && !archived.empty()) {
            Aim::SkillSettings previous,current;
            if(!Aim::ParseSkills(archived,previous,&defaults) || !Aim::ParseSkills(parts.skills,current,&defaults) || !Aim::Profiles::Equivalent(previous,current)) {
                ctx->LogWarn("[QOL/Skills] Conflicting legacy skill sections; migration stopped and files retained.");return false;
            }
        }
        std::string diskMain;
        if(!Aim::ConfigFile::Read(mainPath,diskMain,Aim::Discovery::MaxText) || diskMain!=mainConfig)return false;
        if(hasLegacy && archived.empty() && !Aim::ConfigFile::Write(archive,"",parts.skills))return false;
        if(!Aim::ConfigFile::Write(profile,overrides,updated))return false;
        std::string saved;
        if(!Aim::ConfigFile::Read(profile,saved,Aim::Discovery::MaxText) || saved!=updated ||
            !Aim::ParseSkills(saved,verified,&defaults) || !Aim::Profiles::Equivalent(effective,verified))return false;
        if(hasLegacy) {
            const std::string shared="# Skills moved to config/controller-qol-skills/. Edit the active mod profile there.\n"
                "# Shared controls below are unchanged. Original skill sections are backed up.\n"+parts.shared;
            if(!Aim::Profiles::Split(shared).skills.empty() || !Aim::ConfigFile::Write(mainPath,mainConfig,shared))return false;
        }
        // Generated reference is advisory; inability to refresh it must not undo
        // a verified user-profile migration or prevent the profile from loading.
        auto labels=candidate;
        if(labels.empty())for(const auto& entry:Aim::SkillCatalog)labels.push_back({entry.id,entry.name,false,!entry.snap});
        const auto generated=Aim::Discovery::Catalog(labels,mod,Aim::Discovery::Fingerprint(sourceText));
        std::string oldCatalog;
        if(generated.size()<Aim::Discovery::MaxText && Aim::ConfigFile::Read(catalog,oldCatalog,Aim::Discovery::MaxText,true))
            (void)Aim::ConfigFile::Write(catalog,oldCatalog,generated,false);
        for(const auto& skill:candidate)if(!Aim::FindCatalogSkill(skill.id))skills.RequireConfirmation(skill.id);
        skills.baseline=effective;
        context=ctx;live=&skills;base=defaults;source=std::move(candidate);profilePath=std::move(profile);
        stopped.store(false);active.store(true);
        ctx->LogInfo(hasLegacy?"[QOL/Skills] Skill settings migrated and verified; shared controls unchanged. R3 now saves only to the active mod skill profile.":
            "[QOL/Skills] Skill profile ready in config/controller-qol-skills. Shared controls remain in the main TOML.");
        return true;
    } catch(...) {return false;}
}
void QolSkillDiscovery::Register(const D2RL::PluginContext* ctx,const D2RL::LifecycleService* lifecycle,const D2RL::ThreadService* threads) noexcept {
    if(!active.load())return;
    tables=nullptr;
    if(ctx->QueryService(&tables)!=D2RL::ServiceQueryResult::Success || !D2RL::HasDataTableServiceField(tables,D2RL::DataTableServiceRequiredSize) || !tables->findRowById){tables=nullptr;return;}
    if(lifecycle->registerDataTablesLoadedListener) {
        const D2RL::Lifecycle::DataTablesLoadedListener listener{sizeof(listener),0,&TablesLoaded,nullptr};
        D2RL::Lifecycle::ListenerHandle handle{};
        (void)lifecycle->registerDataTablesLoadedListener(ctx,&listener,&handle);
    }
    if(threads->runOnGameThread)(void)threads->runOnGameThread(ctx,&ConfirmTables,nullptr);
}
void QolSkillDiscovery::Stop() noexcept {stopped.store(true);active.store(false);}
bool QolSkillDiscovery::Active() noexcept {return active.load();}
bool QolSkillDiscovery::Save(int id,bool enabled) noexcept {
    if(!active.load() || stopped.load())return false;
    try {
        std::string text,updated;
        return Aim::ConfigFile::Read(profilePath,text,Aim::Discovery::MaxText) &&
            Aim::Discovery::RewriteOverride(text,base,id,enabled,updated) && Aim::ConfigFile::WriteRolling(profilePath,text,updated);
    } catch(...) {return false;}
}
