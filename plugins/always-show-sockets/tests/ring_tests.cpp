#include "../src/ring_resources.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

void Check(bool value,const char* why) {if(!value){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
unsigned registered{},removed{},failAt{};
D2RL::ResourceService service{};
D2RL::ServiceQueryResult __cdecl Query(const D2RL::PluginContext*,D2RL::ServiceId id,std::uint32_t version,const void** result) noexcept {
    if(id!=D2RL::ServiceId::Resource || version!=1) return D2RL::ServiceQueryResult::Unavailable;
    *result=&service;return D2RL::ServiceQueryResult::Success;
}
D2RL::Resources::Result __cdecl Register(const D2RL::PluginContext*,const D2RL::Resources::ResourceRegistration* r,D2RL::Resources::RegistrationHandle* handle) noexcept {
    ++registered;
    Check(r->structSize==D2RL::Resources::ResourceRegistrationSize && r->flags==0 &&
          std::strstr(r->path,"/d2rloader/always-show-sockets/") && r->byteCount>=40,"only owned namespace registered with complete bytes");
    if(registered==failAt) return D2RL::Resources::Result::Conflict;
    *handle=registered;return D2RL::Resources::Result::Success;
}
D2RL::Resources::Result __cdecl Remove(const D2RL::PluginContext*,D2RL::Resources::RegistrationHandle handle) noexcept {
    Check(handle>0 && handle<=registered,"remove only acquired handles");++removed;return D2RL::Resources::Result::Success;
}
int main(int argc,char** argv) {
    if(argc==3) {
        for(unsigned i=0;i<2;++i) {
            FILE* file{};const auto sprite=Sockets::MakeRingSprite(128,i?Sockets::RingLayer::Center:Sockets::RingLayer::Rim);
            Check(fopen_s(&file,argv[i+1],"wb")==0 && file,"open generated layer preview");
            Check(std::fwrite(sprite.data(),1,sprite.size(),file)==sprite.size(),"write actual generated layer");
            std::fclose(file);
        }
    }
    for(const unsigned side:{64u,128u}) {
        const auto sprite=Sockets::MakeRingSprite(side);
        const auto disk=Sockets::MakeRingSprite(side,Sockets::RingLayer::Center);
        auto word=[&](unsigned offset) {std::uint32_t result{};std::memcpy(&result,sprite.data()+offset,4);return result;};
        auto alpha=[&](unsigned x,unsigned y) {return sprite[40+(y*side+x)*4+3];};
        Check(sprite.size()==40+side*side*4 && std::memcmp(sprite.data(),"SpA1",4)==0,"complete RGBA sprite/header");
        Check(word(4)==(side<<16|31) && word(8)==side && word(12)==side && word(20)==1,"one-frame native dimensions/format");
        unsigned opaque{},soft{};
        for(unsigned y=0;y<side;++y) for(unsigned x=0;x<side;++x) {
            const auto a=alpha(x,y);opaque+=a==255;soft+=a>0 && a<255;
            if(std::hypot(x+0.5-side/2.0,y+0.5-side/2.0)<side*0.40) {
                const auto offset=40+(y*side+x)*4;
                Check(a==0,"rim texture has a clear center independent of its draw alpha");
                Check(disk[offset+3]==255 && !disk[offset] && !disk[offset+1] && !disk[offset+2],"center texture is a solid black disk whose draw alpha is independent");
            }
            Check(a==alpha(side-1-x,y) && a==alpha(x,side-1-y),"ring centered and symmetric");
        }
        Check(opaque>0 && soft>0 && alpha(0,0)==0 && disk[43]==0,"opaque rim, antialiased edges, clear exterior for both layers");
        // Interior/rim core pixels demonstrate independent native alpha values.
        Check(std::abs((disk[40+(side/2*side+side/2)*4+3]/255.0)*0.40-0.40)<0.00001,"default center draw gives 40 percent black");
    }
    Check(Sockets::MakeRingSprite(0).empty(),"unsupported raster size refused");
    const Sockets::SpriteBounds original{64,64,0,64},replacement{128,128,0,128};
    for(const unsigned percent:{10u,65u,75u,100u,105u,110u,115u}) for(const float uiScale:{0.5f,1.0f,2.0f,4.0f}) {
        Sockets::Point point{100,200};float scale=uiScale;
        Check(Sockets::FitRing(point,scale,original,replacement,percent),"fit admitted bounds");
        Sockets::SpriteCenter center{};Check(Sockets::CenterOf(point,scale,replacement,center),"read fitted center");
        Check(std::abs(center.x-(100+32*uiScale))<=0.5 && std::abs(center.y-(200-32*uiScale))<=0.5,"ring preserves native center across sizes/scales");
        Check(std::abs(scale-uiScale*percent/200)<0.000001f,"ring follows size percentage at native relative dimensions");
        Sockets::Point rune{103,225};float runeScale=uiScale;
        const Sockets::SpriteBounds runeBounds{64,96,-4,88};
        Check(Sockets::ScaleAtCenter(rune,runeScale,runeBounds,65,center),"non-square rune pairs to circle center");
        Sockets::SpriteCenter runeCenter{};Check(Sockets::CenterOf(rune,runeScale,runeBounds,runeCenter),"read paired rune center");
        Check(std::abs(center.x-runeCenter.x)<=0.5 && std::abs(center.y-runeCenter.y)<=0.5,"rune and ring centers match within pixel rounding at every UI scale");
        for(unsigned lift:{0u,1u,2u,8u}) {
            auto artworkCenter=center;artworkCenter.y-=lift;
            rune={103,225};runeScale=uiScale;
            Check(Sockets::ScaleAtCenter(rune,runeScale,runeBounds,65,artworkCenter) &&
                  Sockets::CenterOf(rune,runeScale,runeBounds,runeCenter),"lifted artwork center remains valid across UI scales");
            Check(std::abs(center.x-runeCenter.x)<=0.5 && std::abs(center.y-lift-runeCenter.y)<=0.5,"rendered-pixel lift preserves ring center and horizontal alignment");
        }
    }
    Sockets::Point point{100,200};float scale=1;
    Check(!Sockets::FitRing(point,scale,{64,32,0,32},replacement,65) && scale==1 && point.x==100,"non-square circle falls back without mutation");
    Check(!Sockets::FitRing(point,scale,original,replacement,116) && scale==1,"oversized ring percent refused atomically");
    Check(!Sockets::ScaleAtCenter(point,scale,original,115,{132,168}) && scale==1 && point.x==100,"ordinary inserted-art helper still refuses above 100 percent");
    scale=16;
    Check(!Sockets::FitRing(point,scale,original,original,115) && scale==16 && point.x==100,"enlarged rings retain native scale bounds and atomic refusal");
    scale=16;
    Check(!Sockets::FitRing(point,scale,{8192,8192,0,0},{1,1,0,0},100) && scale==16 && point.x==100,"invalid fitted scale never partially mutates native inputs");
    service={.serviceSize=D2RL::ResourceServiceSize,.serviceVersion=D2RL::ResourceService::AbiVersion,.registerResource=Register,.unregisterResource=Remove};
    const D2RL::PluginApi api{.apiSize=D2RL::PluginApiSize,.queryService=Query};
    const D2RL::PluginContext context{.contextSize=D2RL::PluginContextSize,.abiVersion=D2RL_PLUGIN_ABI_VERSION,.api=&api};
    Sockets::RingResources resources;
    Check(resources.Start(&context) && registered==4,"both layers and both HD variants registered");resources.Stop(&context);
    Check(removed==4,"all four owned registrations removed");
    registered=0;removed=0;failAt=2;
    Check(!resources.Start(&context) && registered==2 && removed==1 && !resources.service,"partial registration rolls back acquired handle");
    registered=0;removed=0;failAt=4;
    Check(!resources.Start(&context) && registered==4 && removed==3 && !resources.service,"last layer failure rolls back all earlier registrations");
    service.registerResource=nullptr;
    Check(!resources.Start(&context),"missing service entry refuses optional artwork");
    std::puts("Independent rim/disk pixels, 40 percent center alpha, sprite format, paired centers and resource rollback passed.");
}
