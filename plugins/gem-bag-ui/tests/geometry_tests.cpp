// Execute the adopted geometry bodies against owned synthetic widget trees.
// Only manager lookup and the CRT-like rounding call are test substitutes.
// This validates ABI, packing, read-back and refusal; not real UI lifetime/DPI.
#include "geometry.h"
#include "geometry_profile.h"
#include "text_profile.h"
#include "gold_anchor_profile.h"
#include "native_identify_profile.h"
#include "aim/tree_layout_profile.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {
using namespace D2RL;
using namespace GemBag;
using Result=Widgets::Result;
namespace Profile=InventoryDepositGoldProfile;
void Check(bool value,const char* text){if(!value){std::fprintf(stderr,"FAIL %s\n",text);std::exit(1);}}
struct Object {std::array<unsigned char,0x240> bytes{};};
Object manager,root,card;std::array<Object,9> rows;
std::array<Object,8> textWidgets;std::array<LabelBuffer,8> textBuffers;
std::array<std::string,8> textNames;
std::array<void*,2> rootChildren{};std::array<void*,17> rowChildren{};
std::array<uintptr_t,16> vtable{};
std::array<std::string,9> names;
unsigned char* image{};
bool wrongSdkRoot{};
template<class T>void Put(Object& o,size_t offset,const T& value){std::memcpy(o.bytes.data()+offset,&value,sizeof(value));}
template<class T>T Get(const Object& o,size_t offset){T value{};std::memcpy(&value,o.bytes.data()+offset,sizeof(value));return value;}
Object* Resolve(Widgets::WidgetHandle handle){if(handle==1)return &root;if(handle==2)return &card;if(handle>=3&&handle<=11)return &rows[static_cast<size_t>(handle-3)];if(handle>=12&&handle<=19)return &textWidgets[static_cast<size_t>(handle-12)];return nullptr;}
Result __cdecl FindPanel(const PluginContext*,const char* name,Widgets::WidgetHandle* handle) noexcept {
    if(std::strcmp(name,"gem-bag-ui/Selector"))return Result::NotFound;*handle=1;return Result::Success;
}
Result __cdecl FindWidget(const PluginContext*,Widgets::WidgetHandle parent,const char* name,Widgets::WidgetHandle* handle) noexcept {
    if(parent==1&&!std::strcmp(name,"Card")){*handle=2;return Result::Success;}
    if(parent==2)for(unsigned i=0;i<names.size();++i)if(names[i]==name){*handle=i+3;return Result::Success;}
    if(parent==2)for(unsigned i=0;i<textNames.size();++i)if(textNames[i]==name){*handle=i+12;return Result::Success;}
    return Result::NotFound;
}
Result __cdecl GetRect(const PluginContext*,Widgets::WidgetHandle handle,Widgets::Rect* rect) noexcept {
    auto p=Resolve(handle);if(!p)return Result::NotFound;*rect=Get<Widgets::Rect>(*p,0x70);
    if(wrongSdkRoot&&handle==1){rect->width=3840;rect->height=2160;}
    return Result::Success;
}
bool __cdecl Expected(const PluginContext* context,uint64_t rva,const void* bytes,uint32_t size) noexcept {
    return rva+size<0x3500000&&!std::memcmp(reinterpret_cast<void*>(context->exeBase+rva),bytes,size);
}
void Copy(uintptr_t rva,const void* bytes,size_t length){std::memcpy(image+rva,bytes,length);}
void Setup() {
    image=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x3500000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));Check(image!=nullptr,"owned image allocation");
    for(const auto& site:Profile::Sites)Copy(site.rva,site.bytes,site.size);
    Copy(0x846170,QolNativeIdentifyProfile::FindPanel,sizeof(QolNativeIdentifyProfile::FindPanel));
    Copy(0x8562a0,TreeLayoutNative::Rect,sizeof(TreeLayoutNative::Rect));
    Copy(GeometryProfile::OriginRva,GeometryProfile::Origin,sizeof(GeometryProfile::Origin));
    Copy(GeometryProfile::ScaleRva,GeometryProfile::Scale,sizeof(GeometryProfile::Scale));
    const float one=1;Copy(0x1cba904,&one,sizeof(one));
    const auto textUpdate=reinterpret_cast<uintptr_t>(image)+TextProfile::UpdateRva,textContains=reinterpret_cast<uintptr_t>(image)+Profile::ContainsRva;
    Copy(TextProfile::VtableRva+0x10,&textUpdate,sizeof(textUpdate));Copy(TextProfile::VtableRva+0x40,&textContains,sizeof(textContains));
    // FindPanel retains its complete guarded entry and manager-null check.
    // Its manager search returns this test's synthetic selector object.
    const auto pointer=reinterpret_cast<uintptr_t>(&root);Copy(0x3440170,&pointer,sizeof(pointer));
    unsigned char lookup[]={0x48,0xb8,0,0,0,0,0,0,0,0,0xc3};std::memcpy(lookup+2,&pointer,sizeof(pointer));Copy(0x89f760,lookup,sizeof(lookup));
    // Keep floats unchanged in this substitute. Tests use interior hit points
    // and a pixel of edge tolerance; CRT rounding behavior is not qualified.
    const unsigned char ret=0xc3;Copy(0x12dea28,&ret,1);
    DWORD old{};Check(VirtualProtect(image,0x3500000,PAGE_EXECUTE_READ,&old)!=0,"test image executable/read-only");FlushInstructionCache(GetCurrentProcess(),image,0x3500000);
    vtable[8]=reinterpret_cast<uintptr_t>(image)+Profile::ContainsRva;
    auto init=[](Object& o,const char* name,Object* parent,Ui::Rect rect,float scale) {
        Put(o,0,vtable.data());Put(o,8,name);Put(o,0x30,parent);Put(o,0x70,rect);Put(o,0x80,scale);
    };
    // Match the actual captured panel: the SDK returns root's stored zero
    // rectangle, while native GetRect inherits the manager's dimensions.
    init(manager,"PanelManager",nullptr,{0,0,3840,2160},1.f/3);
    init(root,"gem-bag-ui/Selector",&manager,{0,0,0,0},1);
    Put<unsigned char>(root,0x52,1);
    init(card,"Card",&root,Ui::Card,1);
    rootChildren[0]=&card;Put(root,0x58,rootChildren.data());Put<uint64_t>(root,0x60,1);
    for(unsigned i=0;i<rows.size();++i) {
        names[i]=i==8?"CloseHit":"Row"+std::to_string(i);
        init(rows[i],names[i].c_str(),&card,i==8?Ui::Close:Ui::GemRect(i),1);rowChildren[i]=&rows[i];
    }
    for(unsigned i=0;i<textWidgets.size();++i) {
        textNames[i]="Name"+std::to_string(i);textBuffers[i]=LabelReserve(i);
        init(textWidgets[i],textNames[i].c_str(),&card,{8,218,380,26},1);
        Put(textWidgets[i],0,reinterpret_cast<uintptr_t>(image)+TextProfile::VtableRva);
        Put(textWidgets[i],TextProfile::Data,textBuffers[i].data());Put<uint64_t>(textWidgets[i],TextProfile::Size,LabelReserveBytes-1);Put<uint64_t>(textWidgets[i],TextProfile::Capacity,LabelReserveBytes-1);
        rowChildren[9+i]=&textWidgets[i];
    }
    Put(card,0x58,rowChildren.data());Put<uint64_t>(card,0x60,rowChildren.size());
}
}
int main() {
    Setup();
    const PluginApi api{.apiSize=PluginApiSize,.checkExpectedBytes=Expected};
    const PluginContext ctx{.contextSize=sizeof(PluginContext),.abiVersion=D2RL_PLUGIN_ABI_VERSION,.api=&api,.exeBase=reinterpret_cast<uintptr_t>(image)};
    const WidgetService widgets{.serviceSize=WidgetServiceSize,.serviceVersion=WidgetService::AbiVersion,.findPanel=FindPanel,.findWidget=FindWidget,.getWidgetRect=GetRect};
    Check(Geometry::Initialize(&ctx,&widgets),"exact native geometry witnesses admitted");
    const auto managerBefore=manager,rootBefore=root,cardBefore=card;const auto rowsBefore=rows;
    Geometry::Bounds bounds{};
    Check(Geometry::Place({200,200},1280,720,false,bounds),"zero stored root binds SDK raw rectangle and native inherited size");
    Check(bounds.origin.x==208&&bounds.origin.y==208&&bounds.scale==1.f/3,"native origin and recursive one-third manager scale ABI");
    auto permitted=cardBefore;auto position=Get<Ui::Rect>(cardBefore,0x70);const auto applied=Get<Ui::Rect>(card,0x70);
    position.x=applied.x;position.y=applied.y;Put(permitted,0x70,position);
    Check(manager.bytes==managerBefore.bytes&&root.bytes==rootBefore.bytes&&card.bytes==permitted.bytes&&std::memcmp(rows.data(),rowsBefore.data(),sizeof(rows))==0,"only Card position fields changed");
    for(unsigned i=0;i<8;++i) {
        const auto r=Ui::GemRect(i);int hit{};
        const POINT point{bounds.origin.x+static_cast<LONG>((r.x+r.width/2)*bounds.scale),bounds.origin.y+static_cast<LONG>((r.y+r.height/2)*bounds.scale)};
        Check(Geometry::Hit(point,hit)&&hit==static_cast<int>(i),"real native containment selects every moved icon");
    }
    int hit{};Check(Geometry::Hit({0,0},hit)&&hit==Ui::Outside,"native containment rejects outside clicks");
    const auto close=Ui::Close;
    Check(Geometry::Hit({bounds.origin.x+static_cast<LONG>((close.x+10)*bounds.scale),bounds.origin.y+static_cast<LONG>((close.y+10)*bounds.scale)},hit)&&hit==Ui::CloseHit,"native Close hit");
    Check(Geometry::Place({1279,719},1280,720,false,bounds)&&bounds.origin.x+Ui::Card.width*bounds.scale<=1273&&bounds.origin.y+Ui::Card.height*bounds.scale<=713,"native setters keep edge popup in viewport");
    GemLabels plain{};for(unsigned i=0;i<Gems.size();++i)plain[i]=Gems[i].name;
    const auto textBefore=textWidgets;const auto bufferBefore=textBuffers;
    Check(Geometry::SetLabels(plain,{}),"all eight owned reserved TextBox buffers accept plain names");
    for(unsigned i=0;i<Gems.size();++i) {
        auto expected=textBefore[i];Put<uint64_t>(expected,TextProfile::Size,plain[i].size());
        Check(textWidgets[i].bytes==expected.bytes&&textBuffers[i]==*LabelContents(i,plain[i]),"label write changes only reserved bytes and string length; pointer/capacity/style preserved");
    }
    const auto unchangedText=textWidgets;const auto unchangedBuffers=textBuffers;
    Check(Geometry::SetLabels(plain,plain)&&textBuffers==unchangedBuffers,"repeated validation uses fresh fields without changing text");
    auto translated=plain;translated[0]="\xe7\xb4\xab\xe5\xaf\xb6\xe7\x9f\xb3";translated[6]="Sch\xc3\xa4" "del";
    Put<uint64_t>(textWidgets[7],TextProfile::Capacity,16);const auto badCapacity=textWidgets;
    Check(!Geometry::SetLabels(translated,plain)&&textBuffers==unchangedBuffers&&std::memcmp(textWidgets.data(),badCapacity.data(),sizeof(textWidgets))==0,"all labels preflight before any write; insufficient capacity refused");
    textWidgets=unchangedText;textBuffers[7][0]='!';const auto foreignBuffers=textBuffers;
    Check(!Geometry::SetLabels(translated,plain)&&textBuffers==foreignBuffers,"foreign text is never overwritten");textBuffers=unchangedBuffers;
    Put(textWidgets[7],0,vtable.data());Check(!Geometry::SetLabels(translated,plain)&&textBuffers==unchangedBuffers,"alternate native TextBox type refused");textWidgets=unchangedText;
    auto tooLong=translated;tooLong[0]=std::string(LabelMaxBytes+1,'x');Check(!Geometry::SetLabels(tooLong,plain)&&textBuffers==unchangedBuffers,"overlong localized label refused without resizing");
    Check(Geometry::SetLabels(translated,plain),"UTF-8 labels replace qualified prior labels");
    for(unsigned i=0;i<Gems.size();++i) {
        textBuffers[i]=bufferBefore[i];Put<uint64_t>(textWidgets[i],TextProfile::Size,LabelReserveBytes-1);
    }
    Check(Geometry::SetLabels(translated,plain),"reconstructed native reserve content requalifies without cached pointers");
    Put<uint64_t>(card,0x60,129);Check(!Geometry::Hit({bounds.origin.x+10,bounds.origin.y+10},hit),"unbounded native child vector refused");Put<uint64_t>(card,0x60,17);
    Put(rows[0],0x30,&root);Check(!Geometry::Hit({bounds.origin.x+10,bounds.origin.y+10},hit),"wrong child parent refused");Put(rows[0],0x30,&card);
    Put<unsigned char>(card,0x52,1);const auto before=card;Check(!Geometry::Place({400,400},1280,720,false,bounds)&&card.bytes==before.bytes,"fill-parent setter diagnostic excluded without writes");Put<unsigned char>(card,0x52,0);
    Put(root,0x30,&card);Check(!Geometry::Place({400,400},1280,720,false,bounds),"cyclic ancestor chain refused");Put(root,0x30,&manager);
    wrongSdkRoot=true;Check(!Geometry::Place({400,400},1280,720,false,bounds),"SDK effective rectangle masquerading as stored rectangle refused");wrongSdkRoot=false;
    rootChildren[1]=&card;Put<uint64_t>(root,0x60,2);Check(!Geometry::Place({400,400},1280,720,false,bounds),"duplicate Card name refused");Put<uint64_t>(root,0x60,1);
    vtable[8]++;Check(!Geometry::Place({400,400},1280,720,false,bounds),"alternate containment virtual method refused");vtable[8]--;
    DWORD old{};VirtualProtect(image+Profile::ContainsRva,1,PAGE_EXECUTE_READWRITE,&old);image[Profile::ContainsRva]^=1;DWORD unused{};VirtualProtect(image+Profile::ContainsRva,1,old,&unused);
    Check(!Geometry::Hit({100,100},hit),"changed native code ownership refused before dispatch");
    VirtualFree(image,0,MEM_RELEASE);
    std::puts("Native geometry bodies: ABI, all eight hits, position footprint, edge placement and ownership/layout refusals passed; synthetic UI only.");
}
