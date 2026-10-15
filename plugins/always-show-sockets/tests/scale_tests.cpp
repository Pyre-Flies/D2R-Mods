#include "../src/config.h"
#include <cstdio>
#include <cstdlib>
#include <string>
void Check(bool value,const char* message) {if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
int main() {
    unsigned bad{},percent{};
    Check(Sockets::ParseScaleConfig("",bad)==65 && bad==0,"missing setting defaults to 65");
    Check(Sockets::ParseScaleConfig("\xef\xbb\xbf[always-show-sockets]\ninserted_item_scale_percent=75\n",bad)==75 && bad==0,"UTF-8 BOM configuration");
    Check(Sockets::ParseScaleConfig("[other]\ninserted_item_scale_percent=25\n[always-show-sockets]\ninserted_item_scale_percent = 80 # comment\n",bad)==80 && !bad,"section scoped config");
    for(auto text:{"0","9","101","-1","65.5","nan","inf","65oops",""}) Check(!Sockets::ParsePercent(text,percent),"invalid scale refused");
    Check(Sockets::ParseScaleConfig("[always-show-sockets]\ninserted_item_scale_percent=nan\n",bad)==65 && bad==1,"invalid config falls back");
    auto settings=Sockets::ParseConfig("[always-show-sockets]\ninserted_item_scale_percent=75\n",bad);
    Check(settings.scalePercent==75 && settings.opacityPercent==65 && !bad,"existing size-only config preserves size and gets opacity default");
    Check(settings.rimOpacityPercent==80 && settings.centerOpacityPercent==40,"missing keys get independent ring defaults");
    Check(settings.insertedLiftPixels==1,"missing lift key gets one-pixel default");
    for(unsigned lift:{0u,1u,2u,8u}) {
        Check(Sockets::ParseLift(std::to_string(lift),percent) && percent==lift,"lift accepts bounded whole pixels");
    }
    for(auto text:{"-1","9","1.5","nan","2px",""}) Check(!Sockets::ParseLift(text,percent),"invalid lift refused");
    settings=Sockets::ParseConfig("[always-show-sockets]\ninserted_item_lift_pixels=2\n",bad);
    Check(settings.insertedLiftPixels==2 && settings.scalePercent==65 && !bad,"saved lift independent of size");
    settings=Sockets::ParseConfig("[always-show-sockets]\ninserted_item_lift_pixels=9\n",bad);
    Check(settings.insertedLiftPixels==1 && bad==1,"invalid saved lift falls back to one pixel");
    settings=Sockets::ParseConfig("[always-show-sockets]\ninserted_item_scale_percent=80\ninserted_item_opacity_percent=40\n",bad);
    Check(settings.scalePercent==80 && settings.opacityPercent==40 && !bad,"independent saved settings");
    settings=Sockets::ParseConfig("[always-show-sockets]\ninserted_item_opacity_percent=nan\n",bad);
    Check(settings.opacityPercent==65 && bad==1,"invalid saved opacity falls back");
    settings=Sockets::ParseConfig("[always-show-sockets]\nring_opacity_percent=40\nring_center_opacity_percent=0\n",bad);
    Check(settings.rimOpacityPercent==40 && settings.centerOpacityPercent==0 && !bad,"independent saved ring values allow fully transparent center");
    settings=Sockets::ParseConfig("[always-show-sockets]\nring_opacity_percent=-1\nring_center_opacity_percent=101\n",bad);
    Check(settings.rimOpacityPercent==80 && settings.centerOpacityPercent==40 && bad==2,"invalid saved ring values keep per-setting defaults");
    const std::string original="\xef\xbb\xbf[always-show-sockets] # appearance\r\n  ring_opacity_percent = 80  # outline\r\ninserted_item_scale_percent = 65\r\n[other]\r\nring_opacity_percent = 12\r\n";
    const auto updatedConfig=Sockets::UpdateConfig(original,"ring_opacity_percent",72);
    Check(updatedConfig=="\xef\xbb\xbf[always-show-sockets] # appearance\r\n  ring_opacity_percent = 72  # outline\r\ninserted_item_scale_percent = 65\r\n[other]\r\nring_opacity_percent = 12\r\n","save preserves BOM, CRLF, comments, other keys and other sections");
    const auto inserted=Sockets::UpdateConfig(updatedConfig,"ring_center_opacity_percent",45);
    Check(inserted.find("ring_center_opacity_percent = 45\r\n[other]")!=std::string::npos,"missing key is inserted before next section");
    settings=Sockets::ParseConfig(inserted,bad);
    Check(settings.rimOpacityPercent==72 && settings.centerOpacityPercent==45 && settings.scalePercent==65 && !bad,"updated text round-trips through startup parser");
    Check(Sockets::UpdateConfig("[always-show-sockets]","inserted_item_lift_pixels",2)=="[always-show-sockets]\ninserted_item_lift_pixels = 2\n","missing final newline is handled");
    Check(Sockets::UpdateConfig("[other]\nkeep=1\n","ring_opacity_percent",80)=="[other]\nkeep=1\n[always-show-sockets]\nring_opacity_percent = 80\n","missing section is appended without changing another section");
    Check(Sockets::UpdateConfig("","ring_opacity_percent",80)=="[always-show-sockets]\nring_opacity_percent = 80\n","empty config gets the plugin section");
    Check(Sockets::UpdateConfig("\xef\xbb\xbf  [always-show-sockets]\nring_opacity_percent=80\n","ring_opacity_percent",60)=="\xef\xbb\xbf  [always-show-sockets]\nring_opacity_percent=60\n","BOM with leading header whitespace retains a single section");
    Check(Sockets::ParseRingOpacity("0",percent) && percent==0 && Sockets::ParseRingOpacity("100",percent) && percent==100,"ring opacity accepts zero and 100");
    for(auto text:{"-1","101","35.5","nan","35oops",""}) Check(!Sockets::ParseRingOpacity(text,percent),"invalid ring opacity refused");
    for(float base:{0.0f,0.5f,1.0f}) {
        float value=base;
        Check(Sockets::FadeOpacity(value,65) && std::abs(value-base*0.65f)<0.00001f,"relative native opacity");
        value=base;Check(!Sockets::FadeOpacity(value,100) && value==base,"100 preserves native opacity exactly");
    }
    for(float invalid:{-0.1f,1.1f,std::numeric_limits<float>::infinity()}) {
        float value=invalid;Check(!Sockets::FadeOpacity(value,65) && value==invalid,"invalid native alpha refused without mutation");
    }
    float nan=std::numeric_limits<float>::quiet_NaN();
    Check(!Sockets::FadeOpacity(nan,65) && std::isnan(nan),"nonfinite alpha refused");
    Check(Sockets::CircleScalePercent(10)==25 && Sockets::CircleScalePercent(65)==80 &&
          Sockets::CircleScalePercent(75)==90 && Sockets::CircleScalePercent(84)==99 &&
          Sockets::CircleScalePercent(85)==100 && Sockets::CircleScalePercent(90)==105 &&
          Sockets::CircleScalePercent(100)==115,"circle keeps fifteen-point margin through maximum artwork size");
    Check(Sockets::CircleScalePercent(0)==100 && Sockets::CircleScalePercent(std::numeric_limits<unsigned>::max())==100,"invalid size fails open without unsigned overflow");
    for(unsigned p:{10U,20U,65U,75U,85U,90U,99U,100U}) {
        for(float ui:{0.5f,1.0f,2.0f,4.0f}) {
            for(auto b:{Sockets::SpriteBounds{64,64,0,64},Sockets::SpriteBounds{80,48,12,36},Sockets::SpriteBounds{42,66,-4,70}}) {
                Sockets::Point point{100,200};float scale=ui;
                const double cx=point.x+ui*(b.width*0.5-b.pivotX),cy=point.y+ui*(b.height*0.5-b.pivotY);
                const auto changed=Sockets::CenterScale(point,scale,b,p);
                Check(changed==(p!=100),"100 percent preserves exact native input");
                Check(std::abs(scale-ui*p/100.0f)<0.00001f,"relative scale at all UI sizes");
                Check(std::abs(point.x+scale*(b.width*0.5-b.pivotX)-cx)<=0.501 &&
                      std::abs(point.y+scale*(b.height*0.5-b.pivotY)-cy)<=0.501,"visual center preserved within pixel rounding");
            }
        }
    }
    Sockets::Point point{100,200};float scale=1;
    Check(!Sockets::CenterScale(point,scale,{0,64,0,64},65) && point.x==100 && scale==1,"invalid metadata fail open");
    point.x=std::numeric_limits<std::int32_t>::max();
    Check(!Sockets::CenterScale(point,scale,{64,64,0,64},65) && scale==1,"overflow refuses atomically");
    std::puts("Config bounds, relative scale, native pivots, center preservation and safe fallback passed.");
}
