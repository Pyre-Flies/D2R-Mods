#include "qol_glyphs.h"
#include "qol_navigation.h"
#include "glyph_policy.h"
#include "glyph_signatures.h"
#include "glyph_calls.h"
#include "header_policy.h"
#include "controller_input.h"
#include <mutex>
#include <windows.h>
#include <atomic>
#include <cstdio>

namespace {
using Hint=QolGlyphPolicy::Hint;
using DrawWidget=void(__fastcall*)(void*);
using NativeTextDrawFn=void(__fastcall*)(const char*,void*,void*,float);
DrawWidget originalDrawWidget=nullptr;
NativeTextDrawFn originalDrawText=nullptr;
const D2RL::PluginContext* context=nullptr;
std::atomic<bool> active{false};
std::atomic<unsigned long long> mainDraws{0},subDraws{0},faults{0},hintDraws{0};
thread_local Hint currentHint=Hint::None;
struct HeaderSnapshot {char lines[7][128]{};unsigned long long tick=0;};
HeaderSnapshot headerSnapshot{};
std::mutex headerMutex;
std::atomic<QolGlyphs::BulkHeaderContext> bulkHeaderContext{nullptr};
thread_local bool currentHeader=false;
thread_local char currentHeaderLabel[512]{};
using MeasureText=uint64_t(__fastcall*)(const char*,void*,float,const int*);
MeasureText measureText=nullptr;
constexpr unsigned char measureBytes[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x50,0x0f,0x29,0x74,0x24,0x40,0x49,0x8b,0xf9,0x0f,0x28,0xf2,0x48,0x8b,0xf2,0x48,0x8b,0xd9};

// Widget ctor stores its UTF-8 name string at +8 and parent at +0x30.
// Copy bounded names while guarded; never retain native widget pointers.
size_t ReadNames(void* widget,char (&names)[12][64]) noexcept {
    __try {
        size_t count=0;
        for(auto node=static_cast<const unsigned char*>(widget);node && count<12;++count) {
            auto name=*reinterpret_cast<const char* const*>(node+8);
            if(!name) return 0;
            size_t i=0;for(;i<63 && name[i];++i) names[count][i]=name[i];
            names[count][i]=0;
            if(i==63 && name[i]) return 0;
            auto parent=*reinterpret_cast<const unsigned char* const*>(node+0x30);
            if(parent==node) return 0;
            node=parent;
        }
        return count;
    } __except(EXCEPTION_EXECUTE_HANDLER) {++faults;return 0;}
}
bool ReadGlyphText(const char* text) noexcept {
    __try {
        if(!text) return false;
        size_t size=0;while(size<64 && text[size]) ++size;
        return size<64 && QolGlyphPolicy::ContainsGlyph({text,size});
    } __except(EXCEPTION_EXECUTE_HANDLER) {++faults;return false;}
}
// Recover the unscrolled label only from the exact known Legend layout.
bool CopyHeaderLabel(void* widget,char* output) noexcept {
    __try {
        if(!context || !widget)return false;
        const auto node=reinterpret_cast<uintptr_t>(widget);
        const auto legend=*reinterpret_cast<const uintptr_t*>(node+0x30);
        if(!legend || *reinterpret_cast<const uintptr_t*>(legend)!=context->exeBase+0x1d76550 || node<legend+0x680)return false;
        const auto offset=node-(legend+0x680);
        const auto count=*reinterpret_cast<const uint64_t*>(legend+0x90);
        if(offset%0x230 || count>8 || offset/0x230>=count)return false;
        const auto entries=*reinterpret_cast<const uintptr_t*>(legend+0x88);
        if(!entries)return false;
        const auto entry=entries+(offset/0x230)*0x40;
        const auto glyph=*reinterpret_cast<const uint32_t*>(entry);
        const auto length=*reinterpret_cast<const uint64_t*>(entry+0x18);
        const auto label=*reinterpret_cast<const char* const*>(entry+0x10);
        if(!label || !length || length>480 || glyph<0xe000 || glyph>0xe03f)return false;
        output[0]=static_cast<char>(0xee);output[1]=static_cast<char>(0x80);output[2]=static_cast<char>(0x80+(glyph-0xe000));
        std::memcpy(output+3,label,static_cast<size_t>(length));output[length+3]=0;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
void DrawScoped(void* widget,Hint hint,bool header) {
    char previousLabel[512];std::memcpy(previousLabel,currentHeaderLabel,sizeof(previousLabel));
    currentHeaderLabel[0]=0;
    if(header && !CopyHeaderLabel(widget,currentHeaderLabel))currentHeaderLabel[0]=0;
    auto previous=currentHint;
    const bool previousHeader=currentHeader;
    currentHeader=header;
    currentHint=hint; // A nested unrelated draw never inherits a hint.
    __try {originalDrawWidget(widget);}
    __finally {currentHint=previous;currentHeader=previousHeader;std::memcpy(currentHeaderLabel,previousLabel,sizeof(previousLabel));}
}
void __fastcall HookWidget(void* widget) {
    auto hint=Hint::None;
    bool header=false;
    if(active.load(std::memory_order_relaxed)) {
        char names[12][64]{};
        size_t count=ReadNames(widget,names);
        std::string_view views[12];
        for(size_t i=0;i<count;++i) views[i]=names[i];
        header=measureText && QolHeaderPolicy::Scope(views,count) && ControllerQoL::IsControllerUiActive();
        bool primary=false,secondary=false;
        QolNavigation::GetGlyphModes(primary,secondary);
        hint=QolGlyphPolicy::Classify(views,count,primary,secondary,QolNavigation::SharedPageRemapEnabled(),QolNavigation::OptionsRemapEnabled(),QolNavigation::ChronicleRemapEnabled(),QolNavigation::LootFilterRemapEnabled());
        if(hint!=Hint::None) ++hintDraws;
    }
    // No widget, text buffer, asset, or binding is modified. The two guarded
    // detours borrow static replacement strings only for synchronous rendering.
    DrawScoped(widget,hint,header);
}
bool CopySharedRect(void* rect,Hint hint,QolGlyphPolicy::DrawRect& output) noexcept {
    __try {
        return rect && QolGlyphPolicy::SharedChordRect(hint,*static_cast<const QolGlyphPolicy::DrawRect*>(rect),output);
    } __except(EXCEPTION_EXECUTE_HANDLER) { ++faults;return false; }
}
bool DrawHeader(const char* text,void* rect,void* style,float scale,const HeaderSnapshot& snapshot) noexcept {
    __try {
        if(!text || !rect || !style || !measureText || scale<=0 || scale>10) return false;
        size_t size=0;while(size<512 && text[size])++size;
        if(size==512)return false;
        const int slot=QolHeaderPolicy::Slot({text,size});
        if(slot<0)return false;
        auto top=*static_cast<const QolGlyphPolicy::DrawRect*>(rect);
        if(top.width<=8 || top.width>16384 || top.height<20 || top.height>1024)return false;
        // Leave a small gutter between adjacent native columns.
        const int gutter=top.width/40;
        top.x+=gutter;top.width-=2*gutter;
        auto bottom=top;
        top.height/=2;bottom.y+=top.height;bottom.height-=top.height;
        const int limits[2]={32767,32767};
        float scales[2]={scale*0.75f,scale*0.75f};
        const char* compact=QolHeaderPolicy::Compact({text,size});
        const char* primary=compact?compact:text;
        if(slot==5 && QolNavigation::RangesRemapEnabled())primary="\xEE\x80\xA8" "Show Ranges";
        const char* rows[2]={primary,snapshot.lines[slot]};
        // Native font metrics include the active controller artwork. Fit within
        // the existing slot; never write native rectangles or text buffers.
        for(int i=0;i<2;++i) {
            if(!rows[i][0])continue;
            const auto sizePx=measureText(rows[i],style,scales[i],limits);
            const auto width=static_cast<uint32_t>(sizePx);
            const auto height=static_cast<uint32_t>(sizePx>>32);
            if(!width || !height || width>32767 || height>32767)return false;
            float fit=1.0f;
            if(width>static_cast<unsigned>(top.width))fit=static_cast<float>(top.width)/width;
            if(height>static_cast<unsigned>(top.height) && static_cast<float>(top.height)/height<fit)
                fit=static_cast<float>(top.height)/height;
            scales[i]*=fit;
            if(scales[i]<scale*0.30f)return false;
        }
        originalDrawText(rows[0],&top,style,scales[0]);
        if(rows[1][0])originalDrawText(rows[1],&bottom,style,scales[1]);
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {++faults;return false;}
}
bool IsRangeLabel(const char* text) noexcept {
    __try {
        if(!text)return false;
        size_t size=0;while(size<64 && text[size])++size;
        return size<64 && QolHeaderPolicy::RangeLabel({text,size});
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
void __fastcall HookText(const char* text,void* rect,void* style,float scale) {

    if(currentHeader && active.load(std::memory_order_relaxed)) {
        HeaderSnapshot snapshot;
        {std::lock_guard lock(headerMutex);snapshot=headerSnapshot;}
        const auto bulkContext=bulkHeaderContext.load();
        const char* bulkModifier=bulkContext?bulkContext():nullptr;
        if(QolHeaderPolicy::Compose(snapshot.lines,QolHeaderPolicy::Fresh(GetTickCount64(),snapshot.tick),bulkModifier) && DrawHeader(currentHeaderLabel[0]?currentHeaderLabel:text,rect,style,scale,snapshot))return;
    }

    if(currentHeader && active.load(std::memory_order_relaxed) &&
        QolNavigation::RangesRemapEnabled() && IsRangeLabel(text)) {
        originalDrawText("\xEE\x80\xA8" "Show Ranges",rect,style,scale);
        return;
    }

    auto replacement=active.load(std::memory_order_relaxed)?QolGlyphPolicy::Text(currentHint):nullptr;
    QolGlyphPolicy::DrawRect chordRect{};
    if(replacement && ReadGlyphText(text)) {
        const bool shared=currentHint==Hint::SharedLeft || currentHint==Hint::SharedRight;
        if(shared) {
            // The old 64px box wrapped/clipped the compound label. Changing
            // draw scale alone does not enlarge the text layout's width limit.
            if(!CopySharedRect(rect,currentHint,chordRect)) { originalDrawText(text,rect,style,scale);return; }
            rect=&chordRect;
            scale*=0.70f;
        }
        text=replacement;
        if(currentHint==Hint::MainLeft || currentHint==Hint::MainRight) ++mainDraws;
        else ++subDraws;
    }
    originalDrawText(text,rect,style,scale);
}
// Redirect only the two text calls inside the already-scoped widget draw.
// The loader tracks both byte patches; the global renderer entry stays original.
bool InstallTextCalls(const D2RL::PluginContext* ctx) noexcept {
    for(unsigned i=0;i<2;++i)
        if(!ctx->CheckExpectedBytes(QolGlyphCalls::Sites[i],QolGlyphCalls::Expected[i],5)) return false;
    HMODULE pinned{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&HookText),&pinned)) return false;
    const auto site=ctx->exeBase+QolGlyphCalls::Sites[0];
    SYSTEM_INFO info{};GetSystemInfo(&info);
    const uintptr_t step=info.dwAllocationGranularity;
    const uintptr_t aligned=site & ~(step-1);
    unsigned char* relay=nullptr;
    for(uintptr_t offset=step;offset<0x40000000 && !relay;offset+=step)
        relay=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(aligned+offset),64,
            MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!relay) return false;
    relay[0]=0xff;relay[1]=0x25;std::memset(relay+2,0,4);
    const auto destination=&HookText;std::memcpy(relay+6,&destination,8);
    unsigned char replacements[2][5]{};
    for(unsigned i=0;i<2;++i) {
        if(!QolGlyphCalls::Encode(ctx->exeBase+QolGlyphCalls::Sites[i],reinterpret_cast<uintptr_t>(relay),replacements[i])) {
            VirtualFree(relay,0,MEM_RELEASE);return false;
        }
    }
    DWORD old{};
    if(!VirtualProtect(relay,64,PAGE_EXECUTE_READ,&old)) {VirtualFree(relay,0,MEM_RELEASE);return false;}
    FlushInstructionCache(GetCurrentProcess(),relay,64);
    originalDrawText=reinterpret_cast<NativeTextDrawFn>(ctx->exeBase+0x902e20);
    // Retain after publication (including partial failure) for delayed callers.
    // Shutdown deactivates rendering substitutions; loader owns patch cleanup.
    if(!ctx->PatchBytes(QolGlyphCalls::Sites[0],QolGlyphCalls::Expected[0],5,replacements[0],5))
        return false; // Retain even on an ambiguous publication failure.
    return ctx->PatchBytes(QolGlyphCalls::Sites[1],QolGlyphCalls::Expected[1],5,replacements[1],5);
}

}
void QolGlyphs::Initialize(const D2RL::PluginContext* ctx,bool compatible) noexcept {
    context=ctx;
    if(!context) return;
    for(const auto& signature:GlyphSignatures)
        compatible=compatible && ctx->CheckExpectedBytes(signature.rva,signature.bytes,signature.size);
    if(!compatible) {ctx->LogWarn("[QOL/Glyphs] Binary guard mismatch; native prompts retained.");return;}
    if(ctx->CheckExpectedBytes(0x903cd0,measureBytes,sizeof(measureBytes)))
        measureText=reinterpret_cast<MeasureText>(ctx->exeBase+0x903cd0);
    // Install the consumer first; it is inert until both hooks are present.
    bool text=InstallTextCalls(ctx);
    bool widget=text && ctx->InstallInlineHook<DrawWidget>(0x86d410,GlyphBytes0,sizeof(GlyphBytes0),&HookWidget,&originalDrawWidget);
    active.store(text && widget);
    ctx->LogInfo(active?"[QOL/Glyphs] Scoped widget text calls installed; global renderer untouched: main=triggers, Quest/Skills=bumpers.":
        "[QOL/Glyphs] Hook installation incomplete; native prompts retained.");
}
void QolGlyphs::SetBulkHeaderContext(BulkHeaderContext callback) noexcept {bulkHeaderContext.store(callback);}
void QolGlyphs::Shutdown() noexcept {active.store(false);bulkHeaderContext.store(nullptr);}
void QolGlyphs::Status() noexcept {
    if(!context) return;
    char line[256];
    std::snprintf(line,sizeof(line),"[QOL/Glyphs] hooks=%d hintDraws=%llu mainDraws=%llu subDraws=%llu faults=%llu",
        active.load()?1:0,hintDraws.load(),mainDraws.load(),subDraws.load(),faults.load());
    context->WriteConsoleMessage(line);context->LogInfo(line);
}

void QolGlyphs::PublishHeader(const char* modifier,const char* a,const char* x,const char* y,const char* stick,const char* leftStick) noexcept {
    HeaderSnapshot snapshot{};
    if(modifier) {
        const char* actions[]={a,x,y,stick,leftStick};
        const char* glyph=QolHeaderPolicy::Modifier(modifier);
        for(int i=0;i<5;++i) if(actions[i])
            std::snprintf(snapshot.lines[i],sizeof(snapshot.lines[i]),"%s%s%s %s",glyph,*glyph?"+":"",QolHeaderPolicy::Buttons[i],actions[i]);
        snapshot.tick=GetTickCount64();
    }
    std::lock_guard lock(headerMutex);headerSnapshot=snapshot;
}
