#include "range_text.h"
#include <cstdio>
#include <cstdlib>
void check(bool v) { if (!v) { std::fputs("range text failure\n",stderr); std::exit(1); } }
std::string range(const char* s) { return std::string("\xff" "cU")+s+"\xff" "c3"; }
int main() {
    auto result=RangeText::Merge("+118 Defense\n+38% Enhanced Weapon Damage\n+25% Faster Hit Recovery\n",
        "+"+range("(80-120)")+" Defense\n+"+range("(25-50)")+"% Enhanced Weapon Damage\n+25% Faster Hit Recovery\n",1024);
    check(result.annotated==2 && result.text.find("[+80 - +120]\xff" "c3 +118 Defense")!=std::string::npos);
    check(result.text.find("[+25 - +50]\xff" "c3 +38% Enhanced Weapon Damage")!=std::string::npos);
    check(result.text.ends_with("+25% Faster Hit Recovery\n"));
    result=RangeText::Merge("Adds 12 to 35 Fire Damage","Adds "+range("(10-20)")+" to "+range("(30-40)")+" Fire Damage",1024);
    check(result.annotated==1 && result.text.find("[+10 - +20; +30 - +40]")!=std::string::npos);
    check(result.text.ends_with("Adds 12 to 35 Fire Damage"));
    result=RangeText::Merge("5% Chance to cast level 7 Nova on striking","5% Chance to cast level "+range("(5-10)")+" Nova on striking",1024);
    check(result.annotated==1 && result.text.ends_with("5% Chance to cast level 7 Nova on striking"));
    result=RangeText::Merge("+20 All Resistances","+"+range("(10-30)")+" All Resistances",1024);
    check(result.annotated==1);
    result=RangeText::Merge("+20 Fire Resistance\n+30 Cold Resistance", "+"+range("(10-40)")+" All Resistances",1024);
    check(result.annotated==0 && result.text=="+20 Fire Resistance\n+30 Cold Resistance");
    result=RangeText::Merge("+20 Resistance\n+30 Resistance", "+"+range("(10-40)")+" Resistance\n+"+range("(15-35)")+" Resistance",1024);
    check(result.annotated==0); // Ambiguous labels never cross-attach bounds.
    result=RangeText::Merge("+118 Defense","+"+range("(80-120)")+" Defense",16);
    check(result.annotated==0 && result.text=="+118 Defense");
    const std::string utf8="\xc3\xbf" "c";
    result=RangeText::Merge("+118 Verteidigung","+"+utf8+"U(80-120)"+utf8+"3 Verteidigung",1024);
    check(result.annotated==1 && result.text.starts_with(utf8+"U[+80 - +120]"));
    check(RangeText::Bounds("(-10--5)")=="-10 - -5");
    check(RangeText::Bounds("(1.5-2.5)")=="1.5-2.5");
    check(RangeText::Bounds("\xef\xbc\x88" "80-120\xef\xbc\x89")=="+80 - +120");
    // Exact runtime strings captured from the user's Aldur tooltip in 0.3.1.
    const std::string liveActual="+118 Defense\nSlows Target by 15%\n+30% Enhanced Weapon Damage\n+25% Faster Hit Recovery\n+1 to Druid Skill Levels\n";
    const std::string marker="\xee\x81\xbe";
    const std::string liveRanges="+"+marker+"U(80-120)"+marker+"3 Defense\nSlows Target by 15%\n+"+marker+"U(25-50)"+marker+"3% Enhanced Weapon Damage\n+25% Faster Hit Recovery\n+1 to Druid Skill Levels\n";
    const std::string expected=marker+"U[+80 - +120]"+marker+"3 +118 Defense\nSlows Target by 15%\n"+marker+"U[+25 - +50]"+marker+"3 +30% Enhanced Weapon Damage\n+25% Faster Hit Recovery\n+1 to Druid Skill Levels\n";
    result=RangeText::Merge(liveActual,liveRanges,1024);
    check(result.annotated==2 && result.unmatched==0 && result.text==expected);
    const auto key=RangeText::Analyze("+75% Enhanced Damage").key;
    std::vector<RangeText::Label> split={{key,"[A] [T1] [B] [T5]",{{40,60,"[A] [T1]"},{10,30,"[B] [T5]"}}}};
    const auto native="+"+range("(50-90)")+"% Enhanced Damage";
    result=RangeText::Merge("+75% Enhanced Damage",native,2048,split);
    check(result.text.ends_with("+75% Enhanced Damage [Combined]"));
    check(result.text.find("[40%-60%]")!=std::string::npos && result.text.find("?% Enhanced Damage")!=std::string::npos && result.text.find("[Roll unknown]")==std::string::npos);
    check(result.text.find("+50% Enhanced Damage")==std::string::npos); // Never invent 50+25.
    result=RangeText::Merge("+90% Enhanced Damage",native,2048,split);
    check(result.text.find("+60% Enhanced Damage")!=std::string::npos && result.text.find("+30% Enhanced Damage")!=std::string::npos);
    check(result.text.find("Combined")==std::string::npos && result.text.find("unknown")==std::string::npos);
    split[0].parts={{50,50,"[A] [T1]"},{10,30,"[B] [T5]"}};
    result=RangeText::Merge("+75% Enhanced Damage","+"+range("(60-80)")+"% Enhanced Damage",2048,split);
    check(result.text.find("+50% Enhanced Damage")!=std::string::npos && result.text.find("+25% Enhanced Damage")!=std::string::npos);
    result=RangeText::Merge("+75% Enhanced Damage",native,2048,split);
    check(result.text.find("+25% Enhanced Damage")==std::string::npos); // Incomplete bound accounting.
    result=RangeText::Merge("+75% Enhanced Damage",native,20,split);
    check(result.text=="+75% Enhanced Damage");
    check(RangeText::Expand("Adds 10 to 20 Damage",RangeText::Analyze(native),split[0].parts).empty());
    // Live Bramble Song regression: Savage66..80 + Fine21..30, native provider
    // supplies only21..30 for the95 total. Each prefix needs its own line.
    const std::vector<RangeText::Label> bow={{key,"[Prefix] [T4] [Prefix] [T6]",
        {{66,80,"[Prefix] [T4]"},{21,30,"[Prefix] [T6]"}},true}};
    const auto bowRange="+"+range("(21-30)")+"% Enhanced Damage";
    result=RangeText::Merge("+95% Enhanced Damage",bowRange,2048,bow);
    check(result.text.ends_with("+95% Enhanced Damage [Combined]"));
    check(result.text.find("[66%-80%]")!=std::string::npos && result.text.find("[21%-30%]")!=std::string::npos);
    const auto bowLines=RangeText::Lines(result.text);
    check(bowLines.size()==3);
    check(bowLines[1].find("[Prefix] [T4]")!=std::string_view::npos && bowLines[1].find("[T6]")==std::string_view::npos);
    check(bowLines[0].find("[Prefix] [T6]")!=std::string_view::npos && bowLines[0].find("[T4]")==std::string_view::npos);
    check(bowLines[1].find("?% Enhanced Damage")!=std::string_view::npos && bowLines[0].find("?% Enhanced Damage")!=std::string_view::npos && result.text.find("Roll unknown")==std::string::npos);
    check(bowLines[0].starts_with("\xff" "c5") && bowLines[1].starts_with("\xff" "c5"));
    check(bowLines[2].starts_with("\xff" "c3")); // Drawn above the gray children.
    auto disabled=bow; disabled[0].allowPartialNativeRange=false;
    check(RangeText::Expand("+95% Enhanced Damage",RangeText::Analyze(bowRange),disabled[0].parts).empty());
    check(RangeText::Expand("+95% Enhanced Damage",RangeText::Analyze("+"+range("(1-2)")+"% Enhanced Damage"),bow[0].parts,true).empty());
    result=RangeText::Merge("+110% Enhanced Damage",bowRange,2048,bow);
    check(result.text.find("?% Enhanced Damage")!=std::string::npos && result.text.find("[Roll unknown]")==std::string::npos); // No exact inference through an incomplete provider.
    result=RangeText::Merge("+95% Enhanced Damage",bowRange,32,bow);
    check(result.text=="+95% Enhanced Damage");
    check(RangeText::EndpointRange("+6% Faster Run/Walk","+5% Faster Run/Walk","+10% Faster Run/Walk")=="5% - 10%");
    check(RangeText::EndpointRange("-9% to Enemy Magic Resistance","-5% to Enemy Magic Resistance","-10% to Enemy Magic Resistance")=="-10% - -5%");
    check(RangeText::EndpointRange("+41 to Life","+10 to Life","+65 to Life")=="10 - 65");
    check(RangeText::EndpointRange("+6% Faster Run/Walk","+5% Faster Hit Recovery","+10% Faster Hit Recovery").empty());
    check(RangeText::EndpointRange("+60% Faster Run/Walk","+5% Faster Run/Walk","+10% Faster Run/Walk").empty());
    check(RangeText::EndpointRange("Damage Reduced by 5","Damage Reduced by 5","Damage Reduced by 5").empty());
    std::vector<RangeText::Label> unique={{RangeText::Analyze("+6% Faster Run/Walk").key,"[Unique]",{},false,"5% - 10%"}};
    result=RangeText::Merge("+6% Faster Run/Walk","+6% Faster Run/Walk",512,unique);
    check(result.annotated==1 && result.text.find("[5% - 10%]")!=std::string::npos && result.text.find("[Unique]")!=std::string::npos);
    result=RangeText::Merge("+6% Faster Run/Walk","+"+range("(1-20)")+"% Faster Run/Walk",512,unique);
    check(result.text.find("[+1 - +20]")!=std::string::npos && result.text.find("[5% - 10%]")==std::string::npos);
    std::puts("Passed reverse visual order, gray source rows, unique endpoint ranges, native precedence and bounded fallbacks.");
}
