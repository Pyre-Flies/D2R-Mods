#pragma once
#include "labels.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace GemBag {
namespace Ui {
struct Rect {int x,y,width,height;bool Contains(double px,double py) const {return px>=x&&py>=y&&px<x+width&&py<y+height;}};
inline constexpr Rect Card{0,0,426,250},Close{396,218,24,26};
inline constexpr int None=-1,CloseHit=8,Outside=9;
inline constexpr Rect GemRect(unsigned i) {return {8+static_cast<int>(i%4)*104,8+static_cast<int>(i/4)*104,98,98};}
inline constexpr const char* Art[]={"perfect_amethyst","perfect_topaz","perfect_saphire","perfect_emerald","perfect_diamond","perfect_ruby","perfect_skull","perfect_chaos_onyx"};
// Inputs are measured native geometry in client pixels. The result is local.
inline std::optional<Rect> Placement(Rect current,double originX,double originY,double scale,
    int width,int height,double cursorX,double cursorY,bool controller) {
    if(!std::isfinite(scale)||scale<0.02||scale>8||width<=0||height<=0||
       !std::isfinite(originX)||!std::isfinite(originY)||!std::isfinite(cursorX)||!std::isfinite(cursorY))return {};
    const double w=current.width*scale,h=current.height*scale;
    if(current.width!=Card.width||current.height!=Card.height||w+16>width||h+16>height)return {};
    const auto x=std::clamp(controller?(width-w)/2:cursorX+8,8.0,width-w-8);
    const auto y=std::clamp(controller?(height-h)/2:cursorY+8,8.0,height-h-8);
    const auto localX=current.x+(x-originX)/scale,localY=current.y+(y-originY)/scale;
    if(std::abs(localX)>65536||std::abs(localY)>65536)return {};
    current.x=static_cast<int>(std::lround(localX));current.y=static_cast<int>(std::lround(localY));return current;
}
inline std::string RectJson(Rect r) {
    return "{\"x\":"+std::to_string(r.x)+",\"y\":"+std::to_string(r.y)+",\"width\":"+std::to_string(r.width)+",\"height\":"+std::to_string(r.height)+"}";
}
}
inline std::string Layout(const GemLabels& labels,bool portable=false) {
    std::string text=R"({"type":"Panel","name":"gem-bag-ui/Selector","fields":{"priority":9010,"fitToParent":true},"children":[
      {"type":"Widget","name":"Card","fields":{"anchor":{"x":0,"y":0},"rect":{"x":0,"y":0,"width":426,"height":250}},"children":[
        {"type":"RectangleWidget","name":"Border","fields":{"rect":{"width":426,"height":250},"color":[0.30,0.27,0.21,1]}},
        {"type":"RectangleWidget","name":"Background","fields":{"rect":{"x":2,"y":2,"width":422,"height":246},"color":[0.065,0.06,0.05,0.96]}})";
    for(unsigned i=0;i<Gems.size();++i) {
        const auto n=std::to_string(i),rect=Ui::RectJson(Ui::GemRect(i));
        text+=",{\"type\":\"Widget\",\"name\":\"Row"+n+"\",\"fields\":{\"rect\":"+rect+"}}";
        text+=",{\"type\":\"RectangleWidget\",\"name\":\"Focus"+n+"\",\"fields\":{\"visible\":false,\"rect\":"+rect+",\"color\":[0.27,0.24,0.16,1]}}";
        const auto art=portable&&i==7?Ui::Art[6]:Ui::Art[i];
        text+=",{\"type\":\"ImageWidget\",\"name\":\"Gem"+n+"\",\"fields\":{\"rect\":"+rect+",\"filename\":\"items/misc/gem/"+art+"\"}}";
        text+=",{\"type\":\"RectangleWidget\",\"name\":\"Disabled"+n+"\",\"fields\":{\"visible\":false,\"rect\":"+rect+",\"color\":[0,0,0,0.55]}}";
    }
    auto label=[&](const std::string& name,std::string_view value,Ui::Rect rect,bool visible) {
        text+=",{\"type\":\"TextBoxWidget\",\"name\":\""+name+"\",\"fields\":{\"visible\":"+(visible?"true":"false")+",\"rect\":"+Ui::RectJson(rect)+",\"text\":"+JsonText(value)+",\"style\":{\"fontFace\":\"BlizzardGlobal\",\"pointSize\":24,\"fontColor\":{\"r\":200,\"g\":195,\"b\":176,\"a\":255},\"alignment\":{\"h\":\"center\",\"v\":\"center\"}}}}";
    };
    for(unsigned i=0;i<Gems.size();++i)label("Name"+std::to_string(i),labels[i],{8,218,380,26},false);
    const char* status[]={"Reading...","Bag empty","No free space","Unavailable","Working..."};
    for(unsigned i=0;i<std::size(status);++i)label("Status"+std::to_string(i),status[i],{8,218,380,26},false);
    text+=",{\"type\":\"Widget\",\"name\":\"CloseHit\",\"fields\":{\"rect\":"+Ui::RectJson(Ui::Close)+"}}";
    label("Close","X",Ui::Close,true);
    text+="]}]}";
    if(portable)text.replace(text.find("gem-bag-ui/Selector"),std::string("gem-bag-ui/Selector").size(),"gem-bag-ui/SelectorPortable");
    return text;
}
}
