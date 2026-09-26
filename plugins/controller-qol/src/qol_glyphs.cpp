#include "qol_glyphs.h"
#include "qol_navigation.h"
#include "glyph_policy.h"
#include "glyph_signatures.h"
#include "glyph_calls.h"
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
void DrawScoped(void* widget,Hint hint) {
    auto previous=currentHint;
    currentHint=hint; // A nested unrelated draw never inherits a hint.
    __try {originalDrawWidget(widget);}
    __finally {currentHint=previous;}
}
void __fastcall HookWidget(void* widget) {
    auto hint=Hint::None;
    if(active.load(std::memory_order_relaxed)) {
        char names[12][64]{};
        size_t count=ReadNames(widget,names);
        std::string_view views[12];
        for(size_t i=0;i<count;++i) views[i]=names[i];
        bool primary=false,secondary=false;
        QolNavigation::GetGlyphModes(primary,secondary);
        hint=QolGlyphPolicy::Classify(views,count,primary,secondary,QolNavigation::SharedPageRemapEnabled());
        if(hint!=Hint::None) ++hintDraws;
    }
    // No widget, text buffer, asset, or binding is modified. The two guarded
    // detours borrow static replacement strings only for synchronous rendering.
    DrawScoped(widget,hint);
}
bool CopySharedRect(void* rect,Hint hint,QolGlyphPolicy::DrawRect& output) noexcept {
    __try {
        return rect && QolGlyphPolicy::SharedChordRect(hint,*static_cast<const QolGlyphPolicy::DrawRect*>(rect),output);
    } __except(EXCEPTION_EXECUTE_HANDLER) { ++faults;return false; }
}
void __fastcall HookText(const char* text,void* rect,void* style,float scale) {
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
    // Install the consumer first; it is inert until both hooks are present.
    bool text=InstallTextCalls(ctx);
    bool widget=text && ctx->InstallInlineHook<DrawWidget>(0x86d410,GlyphBytes0,sizeof(GlyphBytes0),&HookWidget,&originalDrawWidget);
    active.store(text && widget);
    ctx->LogInfo(active?"[QOL/Glyphs] Scoped widget text calls installed; global renderer untouched: main=triggers, Quest/Skills=bumpers.":
        "[QOL/Glyphs] Hook installation incomplete; native prompts retained.");
}
void QolGlyphs::Shutdown() noexcept {active.store(false);}
void QolGlyphs::Status() noexcept {
    if(!context) return;
    char line[256];
    std::snprintf(line,sizeof(line),"[QOL/Glyphs] hooks=%d hintDraws=%llu mainDraws=%llu subDraws=%llu faults=%llu",
        active.load()?1:0,hintDraws.load(),mainDraws.load(),subDraws.load(),faults.load());
    context->WriteConsoleMessage(line);context->LogInfo(line);
}
