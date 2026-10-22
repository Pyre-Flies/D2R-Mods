#include "geometry.h"
#include "geometry_profile.h"
#include "text_profile.h"
#include "gold_anchor_profile.h"
#include "native_identify_profile.h"
#include "aim/tree_layout_profile.h"
#include <array>
#include <cstdio>
#include <cstring>

namespace GemBag::Geometry {
namespace {
using NativeRect=D2RL::Widgets::Rect;
using Widget=D2RL::Widgets::WidgetHandle;
using Result=D2RL::Widgets::Result;
namespace Profile=InventoryDepositGoldProfile;
const D2RL::PluginContext* ctx{};
const D2RL::WidgetService* widgets{};
bool admitted{};
const char* reason="not initialized";
const char* Name="gem-bag-ui/Selector";
template<class T>T Read(const void* p,size_t offset=0) {T value{};std::memcpy(&value,static_cast<const unsigned char*>(p)+offset,sizeof(value));return value;}
template<class T>T At(uintptr_t rva) {return reinterpret_cast<T>(ctx->exeBase+rva);}
bool Guard() {
    if(!ctx||!ctx->exeBase||!ctx->CheckExpectedBytes(0x846170,QolNativeIdentifyProfile::FindPanel,sizeof(QolNativeIdentifyProfile::FindPanel)))return false;
    for(const auto& site:Profile::Sites)if(!ctx->CheckExpectedBytes(site.rva,site.bytes,static_cast<uint32_t>(site.size)))return false;
    const unsigned char one[]={0,0,0x80,0x3f};
    return ctx->CheckExpectedBytes(0x8562a0,TreeLayoutNative::Rect,sizeof(TreeLayoutNative::Rect))&&
        ctx->CheckExpectedBytes(GeometryProfile::OriginRva,GeometryProfile::Origin,sizeof(GeometryProfile::Origin))&&
        ctx->CheckExpectedBytes(GeometryProfile::ScaleRva,GeometryProfile::Scale,sizeof(GeometryProfile::Scale))&&ctx->CheckExpectedBytes(0x1cba904,one,sizeof(one));
}
bool Equal(const NativeRect& a,const NativeRect& b) {return a.x==b.x&&a.y==b.y&&a.width==b.width&&a.height==b.height;}
bool Named(const void* p,const char* expected) {
    const auto name=Read<const char*>(p,8);if(!name)return false;
    const auto length=std::strlen(expected);if(length>=64)return false;
    for(size_t i=0;i<=length;++i)if(Read<char>(name,i)!=expected[i])return false;
    return true;
}
void* Child(void* parent,const char* name) {
    const auto count=Read<uint64_t>(parent,0x60);const auto vector=Read<void**>(parent,0x58);
    if(!count||count>128||!vector)return nullptr;
    void* found{};
    for(uint64_t i=0;i<count;++i) {
        auto p=Read<void*>(vector,static_cast<size_t>(i)*sizeof(void*));if(!p||Read<void*>(p,0x30)!=parent)return nullptr;
        if(Named(p,name)){if(found)return nullptr;found=p;}
    }
    return found;
}
bool Ancestors(void* p) {
    std::array<void*,32> seen{};unsigned depth{};
    while(p) {
        if(depth==seen.size())return false;
        for(unsigned i=0;i<depth;++i)if(seen[i]==p)return false;
        seen[depth++]=p;
        const auto scale=Read<float>(p,0x80),x=Read<float>(p,0x48),y=Read<float>(p,0x4c);
        auto parent=Read<void*>(p,0x30);const auto fill=Read<unsigned char>(p,0x52);
        if(!std::isfinite(scale)||scale<0.02f||scale>8||!std::isfinite(x)||!std::isfinite(y)||
            x<0||x>1||y<0||y>1||fill>1||(fill&&!parent))return false;
        p=parent;
    }
    return true;
}
bool Rect(void* native,Widget handle,NativeRect& rect) {
    NativeRect sdk{};
    if(widgets->getWidgetRect(ctx,handle,&sdk)!=Result::Success)return false;
    // The SDK copies stored +0x70 even for a fill-parent panel. Native GetRect
    // instead synthesizes that panel's effective dimensions from its parent.
    // Bind the SDK path to the stored rectangle, then validate the effective
    // getter separately; a zero stored size is expected for our root Panel.
    const auto stored=Read<NativeRect>(native,0x70);
    if(!Equal(stored,sdk)||stored.width<0||stored.height<0||stored.width>65536||stored.height>65536||
        std::abs(static_cast<int64_t>(stored.x))>65536||std::abs(static_cast<int64_t>(stored.y))>65536)return false;
    At<NativeRect*(__fastcall*)(void*,NativeRect*)>(0x8562a0)(native,&rect);
    const auto fill=Read<unsigned char>(native,0x52);
    if(!fill&&!Equal(rect,stored))return false;
    if(fill) {
        auto parent=Read<void*>(native,0x30);if(fill!=1||!parent)return false;
        NativeRect inherited{};At<NativeRect*(__fastcall*)(void*,NativeRect*)>(0x8562a0)(parent,&inherited);
        if(rect.x||rect.y||rect.width!=inherited.width||rect.height!=inherited.height)return false;
    }
    return rect.width>0&&rect.height>0&&rect.width<=65536&&rect.height<=65536&&
        std::abs(static_cast<int64_t>(rect.x))<=65536&&std::abs(static_cast<int64_t>(rect.y))<=65536;
}
bool Plain(void* p,void* parent) {
    const auto table=Read<void*>(p);
    return Read<void*>(p,0x30)==parent&&!Read<unsigned char>(p,0x52)&&Read<float>(p,0x48)==0&&Read<float>(p,0x4c)==0&&Read<float>(p,0x80)==1&&
        !Read<int>(p,0x38)&&!Read<int>(p,0x3c)&&!Read<int>(p,0x40)&&!Read<int>(p,0x44)&&table&&
        Read<uintptr_t>(table,0x40)==ctx->exeBase+Profile::ContainsRva;
}
struct Binding {void* root{};void* card{};Widget rootHandle{},cardHandle{};NativeRect cardRect{};};
bool Bind(Binding& b) {
    reason="native code ownership";
    if(!admitted||!Guard()){admitted=false;return false;}
    reason="SDK selector/Card lookup";
    if(widgets->findPanel(ctx,Name,&b.rootHandle)!=Result::Success||widgets->findWidget(ctx,b.rootHandle,"Card",&b.cardHandle)!=Result::Success)return false;
    b.root=At<void*(__fastcall*)(const char*)>(0x846170)(Name);
    reason="native selector lookup/name";
    if(!b.root||!Named(b.root,Name))return false;
    reason="bounded ancestor chain";if(!Ancestors(b.root))return false;
    b.card=Child(b.root,"Card");NativeRect parent{};
    reason="Card parent/anchor/scale/hit-test";
    if(!b.card||!Plain(b.card,b.root))return false;
    reason="selector stored/effective rectangle binding";if(!Rect(b.root,b.rootHandle,parent))return false;
    reason="Card stored/effective rectangle binding";if(!Rect(b.card,b.cardHandle,b.cardRect))return false;
    reason="Card dimensions";return b.cardRect.width==Ui::Card.width&&b.cardRect.height==Ui::Card.height;
}
bool PlaceUnsafe(POINT anchor,int width,int height,bool controller,Bounds& bounds) {
    Binding b{};if(!Bind(b))return false;
    POINT origin{};At<POINT*(__fastcall*)(void*,POINT*)>(GeometryProfile::OriginRva)(b.card,&origin);
    const auto scale=At<float(__fastcall*)(void*)>(GeometryProfile::ScaleRva)(b.card);
    reason="native scale or viewport bounds";
    auto target=Ui::Placement({b.cardRect.x,b.cardRect.y,b.cardRect.width,b.cardRect.height},origin.x,origin.y,scale,width,height,anchor.x,anchor.y,controller);
    if(!target)return false;
    if(target->x!=b.cardRect.x)At<void(__fastcall*)(void*,int32_t)>(Profile::SetXRva)(b.card,target->x);
    if(target->y!=b.cardRect.y)At<void(__fastcall*)(void*,int32_t)>(Profile::SetYRva)(b.card,target->y);
    reason="setter read-back";
    NativeRect applied{};if(!Rect(b.card,b.cardHandle,applied)||applied.x!=target->x||applied.y!=target->y)return false;
    At<POINT*(__fastcall*)(void*,POINT*)>(GeometryProfile::OriginRva)(b.card,&origin);
    reason="final screen containment";
    if(origin.x<0||origin.y<0||origin.x+applied.width*scale>width||origin.y+applied.height*scale>height)return false;
    bounds={origin,scale,width,height};return true;
}
bool HitUnsafe(POINT point,int& hit) {
    Binding b{};if(!Bind(b))return false;
    const uint64_t packed=static_cast<uint32_t>(point.x)|(uint64_t{static_cast<uint32_t>(point.y)}<<32);
    auto contains=At<bool(__fastcall*)(void*,uint64_t)>(Profile::ContainsRva);
    if(!contains(b.card,packed)){hit=Ui::Outside;return true;}
    hit=Ui::None;
    for(unsigned i=0;i<=Gems.size();++i) {
        char name[32];if(i==Gems.size())strcpy_s(name,"CloseHit");else std::snprintf(name,sizeof(name),"Row%u",i);
        reason="row parent/anchor/scale/hit-test or SDK/native rectangle agreement";
        auto native=Child(b.card,name);Widget handle{};NativeRect rect{};
        if(!native||!Plain(native,b.card)||widgets->findWidget(ctx,b.cardHandle,name,&handle)!=Result::Success||!Rect(native,handle,rect))return false;
        const auto expected=i==Gems.size()?Ui::Close:Ui::GemRect(i);
        if(rect.x!=expected.x||rect.y!=expected.y||rect.width!=expected.width||rect.height!=expected.height)return false;
        if(contains(native,packed))hit=static_cast<int>(i);
    }
    return true;
}
bool Writable(void* pointer,size_t bytes) {
    MEMORY_BASIC_INFORMATION memory{};
    if(!pointer||!VirtualQuery(pointer,&memory,sizeof(memory))||memory.State!=MEM_COMMIT||(memory.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    const auto protection=memory.Protect&0xff;
    if(protection!=PAGE_READWRITE&&protection!=PAGE_WRITECOPY)return false;
    const auto start=reinterpret_cast<uintptr_t>(pointer),base=reinterpret_cast<uintptr_t>(memory.BaseAddress);
    return start>=base&&start-base<=memory.RegionSize&&bytes<=memory.RegionSize-(start-base);
}
bool SetLabelsUnsafe(const GemLabels& current,const GemLabels& previous) {
    Binding b{};if(!Bind(b))return false;
    struct Label {void* widget{};char* buffer{};uint64_t size{},capacity{};LabelBuffer after{};};
    std::array<Label,Gems.size()> labels{};
    // Native TextBox type observed in the admitted game. Pointer/length/capacity
    // at +88/+90/+98 match the existing Buff HUD backing-string contract.
    reason="owned TextBox type, geometry, capacity or reserved text signature";
    for(unsigned i=0;i<labels.size();++i) {
        auto& label=labels[i];char name[32];std::snprintf(name,sizeof(name),"Name%u",i);
        label.widget=Child(b.card,name);Widget handle{};NativeRect rect{};
        auto replacement=LabelContents(i,current[i]);
        if(!replacement||!label.widget||!Plain(label.widget,b.card)||Read<uintptr_t>(label.widget)!=ctx->exeBase+TextProfile::VtableRva||
            Read<uintptr_t>(reinterpret_cast<const void*>(ctx->exeBase+TextProfile::VtableRva),0x10)!=ctx->exeBase+TextProfile::UpdateRva||
            widgets->findWidget(ctx,b.cardHandle,name,&handle)!=Result::Success||!Rect(label.widget,handle,rect)||
            rect.x!=8||rect.y!=218||rect.width!=380||rect.height!=26)return false;
        label.buffer=Read<char*>(label.widget,TextProfile::Data);label.size=Read<uint64_t>(label.widget,TextProfile::Size);label.capacity=Read<uint64_t>(label.widget,TextProfile::Capacity);
        if(label.capacity<LabelReserveBytes-1||label.capacity>16384||label.size>LabelReserveBytes-1||
            !Writable(label.buffer,LabelReserveBytes)||!Writable(static_cast<unsigned char*>(label.widget)+TextProfile::Size,sizeof(uint64_t)))return false;
        const auto reserve=LabelReserve(i);const auto old=LabelContents(i,previous[i]);
        const bool initial=label.size==LabelReserveBytes-1&&!std::memcmp(label.buffer,reserve.data(),reserve.size());
        const bool prior=old&&label.size==previous[i].size()&&!std::memcmp(label.buffer,old->data(),old->size());
        if(!initial&&!prior)return false;
        const auto begin=reinterpret_cast<uintptr_t>(label.buffer),object=reinterpret_cast<uintptr_t>(label.widget);
        if(begin<object+TextProfile::ObjectBytes&&begin+LabelReserveBytes>object)return false;
        for(unsigned j=0;j<i;++j) {
            const auto other=reinterpret_cast<uintptr_t>(labels[j].buffer);
            if(begin<other+LabelReserveBytes&&other<begin+LabelReserveBytes)return false;
        }
        label.after=*replacement;
    }
    // All eight are qualified before the first write. Preserve each allocation,
    // capacity flags, style and native layout; no allocator or text hook is used.
    for(unsigned i=0;i<labels.size();++i) {
        auto& label=labels[i];
        if(label.size==current[i].size()&&!std::memcmp(label.buffer,label.after.data(),label.after.size()))continue;
        std::memcpy(label.buffer,label.after.data(),label.after.size());
        const uint64_t size=current[i].size();std::memcpy(static_cast<unsigned char*>(label.widget)+TextProfile::Size,&size,sizeof(size));
        if(Read<char*>(label.widget,TextProfile::Data)!=label.buffer||Read<uint64_t>(label.widget,TextProfile::Size)!=size||
            Read<uint64_t>(label.widget,TextProfile::Capacity)!=label.capacity||std::memcmp(label.buffer,label.after.data(),label.after.size()))return false;
    }
    return true;
}
}
bool Initialize(const D2RL::PluginContext* context,const D2RL::WidgetService* service) noexcept {
    ctx=context;widgets=service;admitted=false;
    __try {admitted=widgets&&Guard();}__except(EXCEPTION_EXECUTE_HANDLER){admitted=false;}return admitted;
}
void UsePortable(bool portable) noexcept {Name=portable?"gem-bag-ui/SelectorPortable":"gem-bag-ui/Selector";}
bool Place(POINT anchor,int width,int height,bool controller,Bounds& bounds) noexcept {
    __try {return PlaceUnsafe(anchor,width,height,controller,bounds);}__except(EXCEPTION_EXECUTE_HANDLER){admitted=false;return false;}
}
bool Hit(POINT point,int& hit) noexcept {
    __try {return HitUnsafe(point,hit);}__except(EXCEPTION_EXECUTE_HANDLER){admitted=false;return false;}
}
bool SetLabels(const GemLabels& current,const GemLabels& previous) noexcept {
    __try {return SetLabelsUnsafe(current,previous);}__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
const char* LastReason() noexcept {return reason;}
}
