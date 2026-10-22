#pragma once
#include "policy.h"
#include <D2RLPlugin/localization.h>
#include <array>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>

namespace GemBag {
using GemLabels=std::array<std::string,Gems.size()>;
inline constexpr size_t LabelReserveBytes=256,LabelMaxBytes=223;
using LabelBuffer=std::array<char,LabelReserveBytes>;
inline LabelBuffer LabelReserve(unsigned index) {
    LabelBuffer result{};result.fill('_');result.back()=0;
    const auto tag="__GEM_BAG_LABEL_"+std::to_string(index)+"__";
    std::memcpy(result.data(),tag.data(),tag.size());
    // Retain an identifying tail even after a shorter label replaces the head.
    std::memcpy(result.data()+LabelReserveBytes-1-tag.size(),tag.data(),tag.size());
    return result;
}
inline GemLabels ReservedGemLabels() {
    GemLabels result{};
    for(unsigned i=0;i<Gems.size();++i)result[i]=LabelReserve(i).data();
    return result;
}
inline std::optional<LabelBuffer> LabelContents(unsigned index,std::string_view text) {
    if(text.empty()||text.size()>LabelMaxBytes||text.find('\0')!=std::string_view::npos)return {};
    auto result=LabelReserve(index);std::memcpy(result.data(),text.data(),text.size());result[text.size()]=0;return result;
}
// Localization may copy the game's color introducer as U+E07E followed by
// one ASCII selector. Also accept the UTF-8 and legacy source forms of y-diaeresis+c.
// TextBoxWidget draws these item-formatting tokens; its neutral labels need text only.
inline std::string PlainGemLabel(std::string_view source) {
    std::string result;
    for(size_t i=0;i<source.size();) {
        const auto byte=[&](size_t offset){return static_cast<unsigned char>(source[i+offset]);};
        size_t token{};
        if(i+3<source.size()&&byte(0)==0xee&&byte(1)==0x81&&byte(2)==0xbe&&byte(3)<0x80)token=4;
        else if(i+3<source.size()&&byte(0)==0xc3&&byte(1)==0xbf&&source[i+2]=='c'&&byte(3)<0x80)token=4;
        else if(i+2<source.size()&&byte(0)==0xff&&source[i+1]=='c'&&byte(2)<0x80)token=3;
        if(token)i+=token;else result.push_back(source[i++]);
    }
    return result;
}
inline std::optional<GemLabels> ReadGemLabels(const D2RL::PluginContext* context,const D2RL::LocalizationService* service) {
    if(!D2RL::HasLocalizationServiceField(service,D2RL::LocalizationServiceRequiredSize)||!service->getStringByKey)return {};
    GemLabels labels{};
    for(size_t i=0;i<Gems.size();++i) {
        std::array<char,1024> text{};uint32_t required{};
        if(service->getStringByKey(context,Gems[i].nameKey+1,text.data(),static_cast<uint32_t>(text.size()),&required)!=D2RL::Localization::Result::Success)return {};
        const auto end=static_cast<const char*>(std::memchr(text.data(),0,text.size()));
        if(!end||end==text.data())return {};
        labels[i]=PlainGemLabel({text.data(),static_cast<size_t>(end-text.data())});
        if(labels[i].empty())return {};
    }
    return labels;
}
struct GemLabelIssue {
    unsigned gem{8};uint32_t id{},required{};
    D2RL::Localization::Result result{D2RL::Localization::Result::Unavailable};
    const char* reason{"localization service unavailable"};
};
inline std::optional<GemLabels> ReadGemLabels(const D2RL::PluginContext* context,const D2RL::LocalizationService* service,
    const std::array<uint32_t,8>& ids,unsigned mask,GemLabelIssue* issue=nullptr) {
    if(issue)*issue={};
    if(!mask||!D2RL::HasLocalizationServiceField(service,D2RL::LocalizationServiceRequiredSize)||!service->getStringById)return {};
    GemLabels labels{};labels.fill(" ");
    for(unsigned i=0;i<labels.size();++i)if(mask&(1u<<i)) {
        std::array<char,1024> text{};uint32_t required{};
        const auto result=service->getStringById(context,ids[i],text.data(),static_cast<uint32_t>(text.size()),&required);
        if(issue)*issue={i,ids[i],required,result,"localized name lookup failed"};
        if(result!=D2RL::Localization::Result::Success)return {};
        if(issue)issue->reason="empty or unterminated localized name";
        const auto end=static_cast<const char*>(std::memchr(text.data(),0,text.size()));if(!end||end==text.data())return {};
        labels[i]=PlainGemLabel({text.data(),static_cast<size_t>(end-text.data())});if(labels[i].empty())return {};
        if(labels[i].size()>LabelMaxBytes){if(issue)issue->reason="localized name exceeds widget limit";return {};}
    }
    return labels;
}
inline std::string JsonText(std::string_view value) {
    std::string result="\"";
    constexpr char hex[]="0123456789abcdef";
    for(const auto c:value) {
        const auto byte=static_cast<unsigned char>(c);
        if(c=='\"'||c=='\\'){result+='\\';result+=c;}
        else if(byte<0x20){result+="\\u00";result+=hex[byte>>4];result+=hex[byte&15];}
        else result+=c;
    }
    return result+'\"';
}
}
