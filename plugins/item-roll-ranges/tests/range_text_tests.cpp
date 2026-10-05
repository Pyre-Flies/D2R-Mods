#include "range_text.h"
#include "header_text.h"
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
    check(result.annotated==1 && result.text.find("[+10 - +40]")!=std::string::npos);
    check(result.text.ends_with("Adds 12 to 35 Fire Damage"));
    const auto fireKey=RangeText::Analyze("Adds 12 to 35 Fire Damage").key;
    std::vector<RangeText::Label> stackedFire={{fireKey,"[P] [Smoldering] [T3] [S] [of Flame] [T4]"}};
    stackedFire[0].unknownSources={"[P] [Smoldering] [T3]","[S] [of Flame] [T4]"};
    result=RangeText::Merge("Adds 12 to 35 Fire Damage",
        "Adds "+range("(10-20)")+" to "+range("(30-40)")+" Fire Damage",2048,stackedFire);
    const auto fireLines=RangeText::Lines(result.text);
    check(fireLines.size()==3 && fireLines[0].find("Adds ? to ? Fire Damage")!=std::string_view::npos);
    check(fireLines[0].find("[S] [of Flame] [T4]")!=std::string_view::npos);
    check(fireLines[1].find("[P] [Smoldering] [T3]")!=std::string_view::npos);
    check(fireLines[2].find("[+10 - +40]")!=std::string_view::npos &&
          fireLines[2].find("[Combined]")!=std::string_view::npos);
    result=RangeText::Merge("Adds 12 to 35 Fire Damage","Adds 12 to 35 Fire Damage",2048,stackedFire);
    const auto noNativeRange=RangeText::Lines(result.text);
    check(noNativeRange.size()==3 && noNativeRange[0].find("Adds ? to ? Fire Damage")!=std::string_view::npos);
    check(noNativeRange[2].find("Adds 12 to 35 Fire Damage [Combined]")!=std::string_view::npos &&
          noNativeRange[2].find("[+")==std::string_view::npos);
    result=RangeText::Merge("Adds 12 to 35 Fire Damage",
        "+"+range("(2-5)")+" Weapon Fire Damage",2048,stackedFire);
    const auto partialProvider=RangeText::Lines(result.text);
    check(partialProvider.size()==3 && partialProvider[0].find("Adds ? to ? Fire Damage")!=std::string_view::npos);
    check(partialProvider[2].find("Adds 12 to 35 Fire Damage [Combined]")!=std::string_view::npos &&
          partialProvider[2].find("[+2 - +5]")==std::string_view::npos);
    auto exactFire=stackedFire;
    exactFire[0].key=RangeText::Analyze("Adds 22-53 Weapon Fire Damage").key;
    exactFire[0].pairedParts={{21,21,50,50,"[P] [Elemental] [T1]"},
                              {1,1,2,5,"[S] [of Flame] [T6]"}};
    result=RangeText::Merge("Adds 22-53 Weapon Fire Damage",
        "+"+range("(2-5)")+" Weapon Fire Damage",2048,exactFire);
    const auto exactFireLines=RangeText::Lines(result.text);
    check(exactFireLines.size()==3);
    check(exactFireLines[0].find("Adds 1-3 Weapon Fire Damage")!=std::string_view::npos &&
          exactFireLines[0].find("[S] [of Flame] [T6]")!=std::string_view::npos);
    check(exactFireLines[1].find("Adds 21-50 Weapon Fire Damage")!=std::string_view::npos &&
          exactFireLines[1].find("[P] [Elemental] [T1]")!=std::string_view::npos);
    check(exactFireLines[2].find("Adds 22-53 Weapon Fire Damage [Combined]")!=std::string_view::npos);
    result=RangeText::Merge("Adds 22-56 Weapon Fire Damage",
        "+"+range("(2-5)")+" Weapon Fire Damage",2048,exactFire);
    check(result.text.find("Adds ?-? Weapon Fire Damage")!=std::string::npos &&
          result.text.find("Adds 1-6 Weapon Fire Damage")==std::string::npos);
    check(RangeText::Analyze("Adds "+range("(27-51)")+"-"+range("(63-95)")+" Fire Damage").prefix==
          "+27 - +51; +63 - +95");
    check(RangeText::Analyze("Adds 1-"+range("(6-8)")+" Lightning Damage").prefix=="+6 - +8");
    check(RangeText::Analyze("-"+range("(11-20)")+"% Target Defense").prefix=="-20 - -11");
    check(RangeText::Analyze("Level 5: -"+range("(11-20)")+"% Target Defense").prefix=="-20 - -11");
    check(RangeText::DisplayBounds("+27 - +51; +63 - +95")=="+27 - +95");
    result=RangeText::Merge("-11% Target Defense","-"+range("(11-20)")+"% Target Defense",1024);
    check(result.annotated==1 && result.text.find("[-20 - -11]")!=std::string::npos &&
          result.text.ends_with("-11% Target Defense"));
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
    const std::vector<RangeText::Label> bow={{key,"[P] [Savage] [T4] [P] [Fine] [T6]",
        {{66,80,"[P] [Savage] [T4]"},{21,30,"[P] [Fine] [T6]"}},true}};
    const auto bowRange="+"+range("(21-30)")+"% Enhanced Damage";
    result=RangeText::Merge("+95% Enhanced Damage",bowRange,2048,bow);
    check(result.text.ends_with("+95% Enhanced Damage [Combined]"));
    check(result.text.find("[66%-80%]")!=std::string::npos && result.text.find("[21%-30%]")!=std::string::npos);
    const auto bowLines=RangeText::Lines(result.text);
    check(bowLines.size()==3);
    check(bowLines[1].find("[P] [Savage] [T4]")!=std::string_view::npos && bowLines[1].find("[T6]")==std::string_view::npos);
    check(bowLines[0].find("[P] [Fine] [T6]")!=std::string_view::npos && bowLines[0].find("[T4]")==std::string_view::npos);
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
    check(HeaderText::Merge("Defense: 5\n+28% Enhanced Defense",
        "Defense: "+range("(3-5)")+"\n+28% Enhanced Defense").find("Defense: 5 \xee\x81\xbe" "U(3-5)")!=std::string::npos);
    const auto defenseKey=RangeText::Analyze("Defense: 2").key;
    const auto weaponKey=RangeText::Analyze("One-Hand Damage: 12 to 25").key;
    const auto damage=HeaderText::Damage("One-Hand Damage: 12 to 25","One-Hand Damage: 12 to 25",weaponKey,
        std::pair{9,19});
    check(damage.find("12 to 25")!=std::string::npos && damage.find("Base: 9 - 19")!=std::string::npos);
    check(HeaderText::Damage("One-Hand Damage: "+range("(10 to 28)"),"One-Hand Damage: 12 to 25",weaponKey,
        std::pair{9,19}).find("12 to 25")!=std::string::npos);
    const auto missingDamage=HeaderText::Damage("One-Hand Damage: 12 to 25","One-Hand Damage: 12 to 25",weaponKey,{});
    check(missingDamage.find(HeaderText::Footer)!=std::string::npos);
    check(HeaderText::Damage("One-Hand Damage: 12 to 25\nOne-Hand Damage: 12 to 25",
        "One-Hand Damage: 12 to 25",weaponKey,{})=="One-Hand Damage: 12 to 25\nOne-Hand Damage: 12 to 25");
    check(HeaderText::Damage("Einhandschaden: 12 bis 25","Einhandschaden: 12 bis 25",
        RangeText::Analyze("Einhandschaden: 0 bis 0").key,std::pair{9,19}).find("Base:")!=std::string::npos);
    check(HeaderText::Fixed("Defense: 2",defenseKey,true).find("U(Base: 2 - 2)")!=std::string::npos);
    check(HeaderText::Fixed("Defense: 3",defenseKey,true).find("U(Base: 3 - 3)")!=std::string::npos);
    check(HeaderText::Value("Defense: 25",defenseKey)==25);
    check(HeaderText::Fixed("Defense: 25",defenseKey,true,std::pair{2,2}).find("U(Base: 2 - 2)")!=std::string::npos);
    check(HeaderText::Fixed("Defense: 25",defenseKey,false).find(HeaderText::Footer)!=std::string::npos);
    check(HeaderText::Fixed("Defense: 2\nDefense: 3",defenseKey,true)=="Defense: 2\nDefense: 3");
    check(HeaderText::Fixed("Verteidigung: 2",RangeText::Analyze("Verteidigung: 0").key,true).find("U(Base: 2 - 2)")!=std::string::npos);
    auto missing=unique; missing[0].fallbackRange.clear(); missing[0].rangeExpected=true;
    result=RangeText::Merge("+6% Faster Run/Walk","+6% Faster Run/Walk",1024,missing);
    check(result.text.find("(?) Range Unavailable - Please report item affixes")!=std::string::npos);
    check(result.text.find("+6% Faster Run/Walk \xee\x81\xbe" "5(?)")!=std::string::npos);
    check(RangeText::Merge("+6% Faster Run/Walk","+6% Faster Run/Walk",20,missing).text=="+6% Faster Run/Walk");
    result=RangeText::Merge("+6% Faster Run/Walk","+"+range("(5-10)")+"% Faster Run/Walk",1024,missing);
    check(result.text.find("Unavailable")==std::string::npos);
    // Defense header is distinct from the flat modifier, even with the same
    // stat ID. A fixed base has no range, but its actual line must survive.
    result=RangeText::Merge("Defense: 2\n+30 Defense\n",
        "Defense: 2\n+"+range("(20-40)")+" Defense\n",1024);
    check(result.annotated==1 && result.text.starts_with("Defense: 2\n"));
    check(result.text.find("[+20 - +40]")!=std::string::npos && result.text.find("+30 Defense")!=std::string::npos);
    check(RangeText::Merge("Defense: 2","Defense: 2",1024).text=="Defense: 2");
    result=RangeText::Merge("Defense: 34\n+30 Defense\n+50% Enhanced Defense\n",
        "Defense: "+range("(29-34)")+"\n+"+range("(20-40)")+" Defense\n+"+range("(40-60)")+"% Enhanced Defense\n",2048);
    check(result.annotated==3 && result.text.find("Defense: 34")!=std::string::npos);
    check(result.text.find("+30 Defense")!=std::string::npos && result.text.find("+50% Enhanced Defense")!=std::string::npos);
    // Even a missing ranged header cannot erase an available actual header.
    result=RangeText::Merge("Defense: 2\n+30 Defense\n","+30 Defense\n",1024);
    check(result.text=="Defense: 2\n+30 Defense\n");
    std::puts("Passed Defense header/modifier separation and fixed-value retention; native header production is outside this fixture.");
}
