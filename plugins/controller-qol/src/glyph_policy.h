#pragma once
#include <cstddef>
#include <string_view>
namespace QolGlyphPolicy {
// Replace only the native reorder modifier token; retain direction and text.
inline bool ReorderText(std::string_view text,char* output,size_t capacity) noexcept {
    if(text.size()+1>capacity || text.size()<3 || text.substr(0,3)!="\xEE\x80\x8D")return false;
    for(size_t i=0;i<text.size();++i)output[i]=text[i];
    output[2]=static_cast<char>(0xa7);output[text.size()]=0;return true;
}
enum class Hint { None, MainLeft, MainRight, SubLeft, SubRight, SharedLeft, SharedRight };
inline Hint Classify(const std::string_view* names, size_t count, bool primary, bool secondary, bool shared=false, bool options=false, bool chronicle=false, bool lootFilter=false, bool modInformation=false) noexcept {
    if(secondary && modInformation) {
        for(size_t i=0;i+2<count;++i) {
            if(names[i+1]!="OptionsTabs" || names[i+2]!="modinfo_aa") continue;
            if(names[i]=="TabLeftIndicator") return Hint::SubLeft;
            if(names[i]=="TabRightIndicator") return Hint::SubRight;
        }
    }
    // A generated child indicator must not shadow the owning Shared legend.
    if(shared) {
        Hint owner=Hint::None;
        for(size_t i=0;i<count;++i) {
            if(names[i]=="StashLeftArrow") owner=Hint::SharedLeft;
            if(names[i]=="StashRightArrow") owner=Hint::SharedRight;
            if(names[i]=="BankExpansionLayout" && owner!=Hint::None) return owner;
        }
    }
    std::string_view indicator;
    for(size_t i=0;i<count;++i) {
        auto name=names[i];
        if(indicator.empty() && (name=="CycleLeftIndicator" || name=="CycleRightIndicator" ||
            name=="TabLeftIndicator" || name=="TabRightIndicator" || name=="StashLeftArrow" || name=="StashRightArrow")) indicator=name;
        if(shared && name=="BankExpansionLayout") {
            if(indicator=="StashLeftArrow") return Hint::SharedLeft;
            if(indicator=="StashRightArrow") return Hint::SharedRight;
        }
        if(primary && name=="UISwitcher") {
            if(indicator=="CycleLeftIndicator") return Hint::MainLeft;
            if(indicator=="CycleRightIndicator") return Hint::MainRight;
        }
        if(secondary && (name=="QuestLogPanelOriginal" || name=="QuestLogPanelExpansion" || name=="SkillsTreePanel" || (options && name=="SettingsPanel") || (chronicle && name=="ChroniclePanel") || (lootFilter && name=="LootFilterRuleDetailsPanel"))) {
            if(indicator=="TabLeftIndicator") return Hint::SubLeft;
            if(indicator=="TabRightIndicator") return Hint::SubRight;
        }
    }
    return Hint::None;
}
// Native action-glyph table at RVA 0x1d84940: E01B/E01C -> actions 7/8;
// E027/E028 -> actions 19/20. Native font rendering selects controller artwork.
inline const char* Text(Hint hint) noexcept {
    switch(hint) {
    case Hint::MainLeft: return "\xEE\x80\x9B";
    case Hint::MainRight:return "\xEE\x80\x9C";
    case Hint::SubLeft: return "\xEE\x80\xA7";
    case Hint::SubRight:return "\xEE\x80\xA8";
    case Hint::SharedLeft:return "\xEE\x80\xA7+\xEE\x80\x9B";
    case Hint::SharedRight:return "\xEE\x80\xA7+\xEE\x80\x9C";
    default:return nullptr;
    }
}
struct DrawRect { int x,y,width,height; };
inline bool SharedChordRect(Hint hint,const DrawRect& input,DrawRect& output) noexcept {
    if((hint!=Hint::SharedLeft && hint!=Hint::SharedRight) || input.width<=0 || input.width>4096 || input.height<=0 || input.height>4096 || input.x<-100000 || input.x>100000) return false;
    output=input;
    output.width=input.width*3;
    // Center each wider chord on its original slot, using the spare page-label
    // padding instead of growing into main-tab indicators or outside the panel.
    output.x-=input.width;
    return true;
}
inline bool ContainsGlyph(std::string_view text) noexcept {
    for(size_t i=0;i+2<text.size();++i)
        if(static_cast<unsigned char>(text[i])==0xEE &&
           static_cast<unsigned char>(text[i+1])>=0x80 && static_cast<unsigned char>(text[i+1])<=0xBF &&
           (static_cast<unsigned char>(text[i+2])&0xC0)==0x80) return true;
    return false;
}
}
