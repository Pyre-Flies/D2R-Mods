#include "display_config.h"
#include "hint_text.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
void check(bool value){if(!value)std::exit(1);}
int main(int argc,char** argv){
    using RollRanges::ParseDisplayConfig;
    check(!ParseDisplayConfig("").alwaysShowRanges);
    check(ParseDisplayConfig("[display]\nalways_show_ranges = true\n").alwaysShowRanges);
    check(ParseDisplayConfig("\xef\xbb\xbf[display]\r\n always_show_ranges = true # comment\r\n").alwaysShowRanges);
    check(!ParseDisplayConfig("[display]\nalways_show_ranges = false\n").alwaysShowRanges);
    check(!ParseDisplayConfig("[unrelated]\nalways_show_ranges = true\n").alwaysShowRanges);
    check(!ParseDisplayConfig("always_show_ranges = true\n").alwaysShowRanges);
    check(!ParseDisplayConfig("[display]\nother_key = true\n").alwaysShowRanges);
    for(const auto bad:{"TRUE","1","\"true\"","true false",""}){
        const auto result=ParseDisplayConfig(std::string("[display]\nalways_show_ranges = ")+bad);
        check(!result.valid && !result.alwaysShowRanges);
    }
    check(!ParseDisplayConfig("[display]\nalways_show_ranges = true\nalways_show_ranges = false").valid);
    check(!ParseDisplayConfig("[display]\nalways_show_ranges").valid);
    check(!ParseDisplayConfig(std::string(65537,'x')).valid);
    check(argc==2);std::ifstream input(argv[1]);check(input.good());
    const std::string defaults{std::istreambuf_iterator<char>(input),{}};
    const auto settings=ParseDisplayConfig(defaults);check(settings.valid && !settings.alwaysShowRanges);
    const std::string hint="Hold Ctrl To Show Ranges";
    const std::string before="Ctrl + Left Click To Drop\nHold Ctrl To Show Ranges\nHold Shift To Compare";
    check(RollRanges::RemoveRangeHint(before,hint)=="Ctrl + Left Click To Drop\nHold Shift To Compare");
    check(RollRanges::RemoveRangeHint(before+'\n',hint)=="Ctrl + Left Click To Drop\nHold Shift To Compare\n");
    check(RollRanges::RemoveRangeHint("\xee\x81\xbe" "5Hold Ctrl To Show Ranges\xee\x81\xbe" "0",hint).empty());
    check(RollRanges::RemoveRangeHint(before+"\n"+hint,hint)==before+"\n"+hint);
    check(RollRanges::RemoveRangeHint(before,"")==before);
    check(RollRanges::RemoveRangeHint(before,hint+"\nOther hint")==before);
    check(RollRanges::RemoveRangeHint("Ctrl + Left Click To Drop",hint)=="Ctrl + Left Click To Drop");
    check(RollRanges::RemoveRangeHint("Vergleichen\nStrg fur Bereiche", "Strg fur Bereiche")=="Vergleichen");
    std::puts("Display config: hold-mode defaults, opt-in, strict bools, comments, schema isolation and refusals pass.");
}
