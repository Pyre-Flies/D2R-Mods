#pragma once
#include "range_text.h"
namespace RollRanges {
inline std::string RemoveRangeHint(std::string_view footer,std::string_view hint) {
    std::string key;unsigned hintLines=0;
    for(const auto line:RangeText::Lines(hint)){
        const auto shape=RangeText::Analyze(line).key;
        if(!shape.empty()){key=shape;++hintLines;}
    }
    if(hintLines!=1)return std::string(footer);
    unsigned count=0;for(const auto line:RangeText::Lines(footer))count+=RangeText::Analyze(line).key==key;
    if(count!=1)return std::string(footer);
    std::string result;
    for(const auto line:RangeText::Lines(footer)){
        if(RangeText::Analyze(line).key==key)continue;
        result+=line;result+='\n';
    }
    if(!footer.ends_with('\n') && !result.empty())result.pop_back();
    return result;
}
}
