#include "../chat_config.h"
#include "../../floating-damage/src/config.h"
#include "../../item-roll-ranges/src/display_config.h"
#include "../../always-show-sockets/src/scale.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>

void Check(bool value,const char* why) {if(!value){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
struct FileConfig {
    std::filesystem::path path;
    mutable unsigned reads{},writes{};
    bool failRead{},failWrite{};
    std::string Read() const {std::ifstream input(path,std::ios::binary);return {std::istreambuf_iterator<char>(input),{}};}
    void Seed(std::string_view text) const {std::ofstream output(path,std::ios::binary|std::ios::trunc);output.write(text.data(),static_cast<std::streamsize>(text.size()));}
    bool ReadConfig(char* output,unsigned size,unsigned* required) const noexcept {
        ++reads;if(failRead)return false;
        try {const auto text=Read();*required=static_cast<unsigned>(text.size()+1);
            if(size<*required)return false;std::memcpy(output,text.c_str(),*required);return true;
        } catch(...) {return false;}
    }
    bool WriteConfig(const char* text) const noexcept {
        ++writes;if(failWrite)return false;
        try {Seed(text);return true;} catch(...) {return false;}
    }
};
int main() {
    const auto directory=std::filesystem::temp_directory_path()/
        ("d2r-chat-settings-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    FileConfig storage{directory/"settings.toml"};
    const std::string original="\xef\xbb\xbf# preferences\r\n[floating-damage]\r\nenabled = true # show\r\nfalling_motion = true\r\ntext_color = \"#C0FFEE\"\r\n[other]\r\nkeep = 7\r\n";
    storage.Seed(original);
    Check(ChatConfig::Save(&storage,"floating-damage",{{"enabled",false},{"falling_motion",false}},16384),"save both preferences through file-backed configuration API");
    const auto saved=storage.Read();unsigned rejected{};
    Check(saved=="\xef\xbb\xbf# preferences\r\n[floating-damage]\r\nenabled = false # show\r\nfalling_motion = false\r\ntext_color = \"#C0FFEE\"\r\n[other]\r\nkeep = 7\r\n","retain BOM, CRLF, comments, colors and unrelated section bytes");
    FileConfig restarted{storage.path};const auto damage=Damage::Parse(restarted.Read(),rejected);
    Check(!rejected && !damage.enabled && !damage.fallingMotion,"fresh startup reader restores saved damage settings from disk");
    storage.Seed("[other]\nkeep = 7");
    Check(ChatConfig::Save(&storage,"display",{{"always_show_ranges",true}},65537),"insert missing display section without losing unrelated text");
    const auto ranges=RollRanges::ParseDisplayConfig(restarted.Read());
    Check(ranges.valid && ranges.alwaysShowRanges,"fresh range startup reader restores always-show");
    storage.Seed("[always-show-sockets]\nring_opacity_percent = 57\n[other]\nkeep = 7\n");
    Check(ChatConfig::Save(&storage,"always-show-sockets",{{"enabled",false},{"always_visible",true}},4096),"save socket mode as one document write");
    const auto sockets=Sockets::ParseConfig(restarted.Read(),rejected);
    Check(!rejected && !sockets.enabled && sockets.alwaysVisible && sockets.rimOpacityPercent==57,"fresh socket startup reader restores off and prior styled mode");
    for(const char* malformed:{"[display]\nalways_show_ranges=true\nalways_show_ranges=false\n",
                              "[display]\n[display]\n", "[display]\n'always_show_ranges'=true\n",
                              "['display']\nalways_show_ranges=true\n", "[ display ]\nalways_show_ranges=true\n",
                              "[display]\nalways_show_ranges='true'\n", "[display]\nalways_show_ranges # missing value\n",
                              "[other]\nnote=\"\"\"multiline\n[display]\n\"\"\"\n"}) {
        storage.Seed(malformed);const auto before=storage.writes;
        Check(!ChatConfig::Save(&storage,"display",{{"always_show_ranges",false}},65537) && storage.writes==before && storage.Read()==malformed,"ambiguous input is left untouched");
    }
    storage.Seed("[display]\nalways_show_ranges = false\n");const auto beforeFailure=storage.Read();
    storage.failRead=true;const auto writes=storage.writes;
    Check(!ChatConfig::Save(&storage,"display",{{"always_show_ranges",true}},65537) && storage.writes==writes,"failed read cannot overwrite the config");
    storage.failRead=false;storage.failWrite=true;
    Check(!ChatConfig::Save(&storage,"display",{{"always_show_ranges",true}},65537) && storage.Read()==beforeFailure,"failed write is reported without changing the file");
    storage.failWrite=false;storage.Seed(std::string(4096,'#'));const auto beforeOversize=storage.writes;
    Check(!ChatConfig::Save(&storage,"display",{{"always_show_ranges",true}},4096) && storage.writes==beforeOversize,"oversized config is never truncated");
    const auto resolved=std::filesystem::weakly_canonical(directory);
    Check(resolved.parent_path()==std::filesystem::weakly_canonical(std::filesystem::temp_directory_path()) &&
        resolved.filename().string().starts_with("d2r-chat-settings-"),"cleanup stays inside the named temporary fixture directory");
    std::filesystem::remove_all(resolved);
    std::puts("Chat settings persist through actual temporary TOML files and fresh startup parsers; errors preserve existing documents.");
}
