#pragma once
#include "aim/config_sections.h"
#include <charconv>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <unordered_set>

namespace QolConfig {
struct Settings {
    bool enabled=true,quickIdentify=true,identifyAll=true,nativeIdentify=false;
    bool quickMove=true,cubeTapOpen=true,cubeTransmuteShortcut=true,quickDeposit=true,groundPickup=true;
    std::uint32_t groundPickupDistance=14,portalPriorityDistance=10;
    char groundPickupButton[16]="lb";
    bool requireTomeOrScroll=true,consumeTomeOrScroll=true,requireModifier=true;
    char modifier[16]="bumper";
    std::uint8_t triggerThreshold=30;
    bool debugLogging=false,blockFilteredPickup=true;
    bool prioritizePortals=true,prioritizeStashBoxes=true,prioritizeWaypoints=true,prioritizeShrines=true,prioritizeChests=true;
    bool portalDiagnostics=false;
};

inline bool Button(std::string_view value,bool modifier) noexcept {
    for(const auto name:{"lb","l1","bumper","rb","r1","l3","r3","lt","l2","trigger","rt","r2","[",
                         "l4","bracket","leftbracket","]","r4","rightbracket"})if(value==name)return true;
    return (modifier && value=="any") || (value.size()==1 && ((value[0]>='a' && value[0]<='z') || (value[0]>='0' && value[0]<='9')));
}

// The shared file supports the documented QOL/aim schema, not arbitrary TOML.
// Parse into a fresh candidate; never mutate the running settings on failure.
inline bool Parse(std::string_view text,Settings& output,std::string& error) {
    error.clear();
    if(text.starts_with("\xef\xbb\xbf"))text.remove_prefix(3);
    if(text.empty() || text.size()>=65536 || text.find('\0')!=text.npos){error="missing, empty or oversized main configuration";return false;}
    Settings parsed;
    std::string section;
    std::unordered_set<std::string> sections,keys;
    bool haveQol=false;
    while(!text.empty()) {
        const auto end=text.find('\n');auto line=QolAim::Trim(QolAim::WithoutComment(text.substr(0,end)));
        text=end==text.npos?std::string_view{}:text.substr(end+1);
        if(line.empty())continue;
        if(line.front()=='[') {
            if(line.size()<3 || line.back()!=']' || line[1]=='['){error="malformed section";return false;}
            section=std::string(line.substr(1,line.size()-2));
            if((section!="qol" && section!="aim" && !section.starts_with("aim.")) || !sections.insert(section).second){error="unknown or duplicate section: "+section;return false;}
            haveQol=haveQol || section=="qol";continue;
        }
        const auto eq=line.find('=');
        if(section.empty() || eq==line.npos){error="setting outside a section or missing '='";return false;}
        const auto key=QolAim::Trim(line.substr(0,eq)),value=QolAim::Trim(line.substr(eq+1));
        if(key.empty() || value.empty() || !keys.insert(section+"."+std::string(key)).second){error="missing or duplicate setting: "+std::string(key);return false;}
        if(section!="qol")continue; // Aim/profile parsers validate these values.
        bool* flag=nullptr;
#define QOL_BOOL(keyName,member) if(key==keyName)flag=&parsed.member;else
        QOL_BOOL("enabled",enabled)
        QOL_BOOL("quick_identify",quickIdentify)
        QOL_BOOL("identify_all",identifyAll)
        QOL_BOOL("native_identify",nativeIdentify)
        QOL_BOOL("quick_move",quickMove)
        QOL_BOOL("cube_tap_open",cubeTapOpen)
        QOL_BOOL("cube_transmute_shortcut",cubeTransmuteShortcut)
        QOL_BOOL("quick_deposit",quickDeposit)
        QOL_BOOL("ground_pickup",groundPickup)
        QOL_BOOL("require_tome_or_scroll",requireTomeOrScroll)
        QOL_BOOL("consume_tome_or_scroll",consumeTomeOrScroll)
        QOL_BOOL("require_modifier",requireModifier)
        QOL_BOOL("debug_logging",debugLogging)
        QOL_BOOL("block_filtered_pickup",blockFilteredPickup)
        QOL_BOOL("prioritize_portals",prioritizePortals)
        QOL_BOOL("prioritize_stash_boxes",prioritizeStashBoxes)
        QOL_BOOL("prioritize_waypoints",prioritizeWaypoints)
        QOL_BOOL("prioritize_shrines",prioritizeShrines)
        QOL_BOOL("prioritize_chests",prioritizeChests)
        QOL_BOOL("portal_diagnostics",portalDiagnostics)
        {}
#undef QOL_BOOL
        if(flag){if(value!="true" && value!="false"){error="expected true/false: "+std::string(key);return false;}*flag=value=="true";continue;}
        if(key=="modifier" || key=="ground_pickup_button") {
            if(value.size()<3 || value.size()>17 || value.front()!='"' || value.back()!='"'){error="invalid button string: "+std::string(key);return false;}
            std::string button(value.substr(1,value.size()-2));
            for(auto& ch:button)if(ch>='A' && ch<='Z')ch=static_cast<char>(ch-'A'+'a');
            if(!Button(button,key=="modifier")){error="unknown button: "+std::string(key);return false;}
            auto* dest=key=="modifier"?parsed.modifier:parsed.groundPickupButton;
            std::memcpy(dest,button.c_str(),button.size()+1);continue;
        }
        unsigned number{};
        const auto result=std::from_chars(value.data(),value.data()+value.size(),number);
        if((key!="ground_pickup_distance" && key!="portal_priority_distance" && key!="trigger_threshold") ||
           result.ec!=std::errc{} || result.ptr!=value.data()+value.size() || number<1 || number>(key=="trigger_threshold"?255u:20u)) {
            error="unknown setting or invalid integer: "+std::string(key);return false;
        }
        if(key=="ground_pickup_distance")parsed.groundPickupDistance=number;
        else if(key=="portal_priority_distance")parsed.portalPriorityDistance=number;
        else parsed.triggerThreshold=static_cast<std::uint8_t>(number);
    }
    if(!haveQol){error="missing [qol] section";return false;}
    output=parsed;return true;
}

// These values select startup hooks or are copied into startup-only modules.
// A live reload can report their requested changes, but never installs/removes hooks.
inline Settings LiveSettings(const Settings& requested,const Settings& startup,std::string& restart) {
    auto result=requested;restart.clear();
    auto changed=[&](const char* name){if(!restart.empty())restart+=", ";restart+=name;};
#define QOL_STARTUP(member,key) if(requested.member!=startup.member)changed(key);result.member=startup.member;
    QOL_STARTUP(enabled,"qol.enabled")
    QOL_STARTUP(nativeIdentify,"native_identify")
    QOL_STARTUP(cubeTapOpen,"cube_tap_open")
    QOL_STARTUP(cubeTransmuteShortcut,"cube_transmute_shortcut")
    QOL_STARTUP(groundPickup,"ground_pickup")
    QOL_STARTUP(blockFilteredPickup,"block_filtered_pickup")
    QOL_STARTUP(prioritizePortals,"prioritize_portals")
    QOL_STARTUP(prioritizeStashBoxes,"prioritize_stash_boxes")
    QOL_STARTUP(prioritizeWaypoints,"prioritize_waypoints")
    QOL_STARTUP(prioritizeShrines,"prioritize_shrines")
    QOL_STARTUP(prioritizeChests,"prioritize_chests")
    QOL_STARTUP(portalDiagnostics,"portal_diagnostics")
    QOL_STARTUP(portalPriorityDistance,"portal_priority_distance")
#undef QOL_STARTUP
    for(const bool modifier:{true,false}) {
        const auto* from=modifier?startup.modifier:startup.groundPickupButton;
        const auto* desired=modifier?requested.modifier:requested.groundPickupButton;
        if(std::strcmp(from,desired))changed(modifier?"modifier":"ground_pickup_button");
        std::strcpy(modifier?result.modifier:result.groundPickupButton,from);
    }
    return result;
}

struct Stamp {
    bool present{};
    std::uintmax_t size{};
    std::filesystem::file_time_type modified{};
    bool operator==(const Stamp&) const noexcept=default;
};
struct Files {Stamp main,profile;bool operator==(const Files&)const noexcept=default;};
class Watcher {
    Files pending{};std::uint64_t changed{};bool seeded{},unsettled{};
public:
    void Resync(const Files& files) noexcept {pending=files;seeded=true;unsettled=false;changed=0;}
    bool Observe(const Files& files,std::uint64_t now) noexcept {
        if(!seeded){Resync(files);return false;}
        if(!(files==pending)){pending=files;changed=now;unsettled=true;return false;}
        if(!unsettled || now<changed || now-changed<650)return false;
        unsettled=false;return true;
    }
};
}
