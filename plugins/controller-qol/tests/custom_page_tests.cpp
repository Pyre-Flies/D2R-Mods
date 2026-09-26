#include "custom_page_actions.h"
#include <cstdio>
#include <windows.h>
int main() {
    // Run the real bridge with no game/Core loaded. It must reject before
    // touching game-relative addresses, preserve ordinary routing, and prevent
    // a CustomPage source from falling through to a vanilla container lookup.
    D2RL::PluginContext ctx{};
    ctx.exeBase=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    D2RL::Items::ItemInfo info{.structSize=D2RL::Items::ItemInfoSize};
    if(QolCustomPage::Initialize(&ctx)) return 1;
    if(QolCustomPage::Visible(&ctx)) return 2;
    info.container=D2RL::Items::ItemContainer::Inventory;
    if(QolCustomPage::TryTransfer(&ctx,info)) return 3;
    info.container=D2RL::Items::ItemContainer::CustomPage;
    if(!QolCustomPage::TryTransfer(&ctx,info)) return 4;
    QolCustomPage::Shutdown();
    if(QolCustomPage::Visible(&ctx)) return 5;
    std::puts("Absent-provider admission, vanilla fallback and custom-source refusal passed.");
}
