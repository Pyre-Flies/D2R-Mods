#pragma once
#include <string_view>
namespace Probe {
inline bool GroundShortcutsAllowed(unsigned dedicated,unsigned submenus) noexcept {
    return dedicated==0 && submenus==0;
}
inline unsigned SubPanelBit(std::string_view name) noexcept {
    if(name=="QuestLogPanelExpansion" || name=="QuestLogPanelOriginal") return 1;
    if(name=="SkillsTreePanel") return 2;
    if(name=="SettingsPanel") return 4;
    if(name=="ChroniclePanel") return 8;
    return 0;
}
inline bool OptionsTab(std::string_view widget,std::string_view parent,bool visible) noexcept {
    return visible && widget=="OptionsTabs" && parent=="SettingsPanel";
}
inline bool ChronicleTab(std::string_view widget,std::string_view parent,bool visible) noexcept {
    return visible && widget=="ChronicleTabs" && parent=="ChroniclePanel";
}
inline int SubMenuAction(bool enabled,unsigned action) noexcept {
    if(!enabled) return static_cast<int>(action);
    if(action==7 || action==8) return -1;
    if(action==19) return 7;
    if(action==20) return 8;
    return static_cast<int>(action);
}
inline unsigned DedicatedPanelBit(std::string_view name) noexcept {
    if(name.starts_with("Bank")) return 1;
    if(name.starts_with("Vendor")) return 2;
    if(name.starts_with("HoradricCube")) return 4;
    if(name.starts_with("Waypoint")) return 8;
    if(name.starts_with("Trade")) return 16;
    if(name.starts_with("Imbue")) return 32;
    if(name.starts_with("ScrollOfInifuss")) return 64;
    return 0;
}
// Called only for UISwitcher's ControllerActionBegin, never global dispatch.
inline int MenuAction(bool enabled, unsigned action) noexcept {
    if(!enabled) return static_cast<int>(action);
    if(action==19 || action==20) return -1;
    if(action==7) return 19;
    if(action==8) return 20;
    return static_cast<int>(action);
}
inline bool ModifierHeld(bool useL1, bool l1, bool l2) noexcept {
    return useL1 ? l1 : l2;
}
inline bool RecoverControllerLabels(bool active,bool controller,bool eligible,int filtered,int displayed) noexcept {
    return active && controller && eligible && filtered==1 && displayed==0;
}
inline bool LockFilteredLabels(bool enabled, bool l1Mode, bool hooksReady,
                               bool inGame, bool controllerMode, unsigned channel) noexcept {
    return enabled && l1Mode && hooksReady && inGame && controllerMode && channel == 0;
}
inline bool ConsumeTabLeft(bool enabled, bool l2, bool controllerBegin,
                           bool tabSwitchEnabled, unsigned action,
                           unsigned left, unsigned right) noexcept {
    // Action 0 is the primary activation branch in the inspected handler.
    return enabled && l2 && controllerBegin && tabSwitchEnabled &&
           action != 0 && left != right && action == left;
}
inline bool IsControllerAction(std::string_view target, std::string_view command) noexcept {
    return target == "InputMessage" &&
           (command == "ControllerActionBegin" || command == "ControllerActionEnd");
}
// Routes observed in the user's v0.1 log. This is deliberately a screen-lock
// experiment while L2 is held; it does not swallow generic controller actions.
inline bool IsScreenTransition(std::string_view target, std::string_view command) noexcept {
    return (target == "PanelManager" &&
           (command == "OpenPanel" || command == "ClosePanel" || command == "OpenExclusivePanel")) ||
           (target == "BankPanelMessage" && command == "SelectTab");
}
inline bool ShouldTrace(std::string_view target, std::string_view command) noexcept {
    return IsControllerAction(target, command) || target == "PanelManager" ||
           target == "BankPanelMessage" || target == "UISwitcher";
}
inline bool ShouldConsume(bool enabled, bool l2, std::string_view target,
                          std::string_view command, std::string_view selectedTarget,
                          std::string_view selectedCommand) noexcept {
    return enabled && l2 && !selectedTarget.empty() && !target.empty() &&
           target == selectedTarget &&
           (selectedCommand == "*" || command == selectedCommand);
}
// The native lookup examines previous (+0x50) and current (+0x51) flags.
// Include the release edge so a release-triggered screen change is blocked.
inline bool OwnGesture(unsigned previous, unsigned current) noexcept {
    return previous <= 1 && current <= 1 && (previous != 0 || current != 0);
}
}
