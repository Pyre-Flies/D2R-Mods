#include "native.h"
#include "stash_search.h"
#include <D2RLPlugin/api.h>
#include <imm.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace StashSearch {
namespace {

// Global plugin state (mirrors .data 0x1b000..0x1b4f0)
std::mutex               g_StateMutex;
std::wstring             g_QueryText;                        // 0x1b050..0x1b068
std::wstring             g_EditSyncedQuery;                  // 0x1b070..0x1b088
std::wstring             g_SearchPlaceholder = L"Search";    // 0x1b090..0x1b0a8
std::wstring             g_TabLabelPersonal  = L"Personal";
std::wstring             g_TabLabelGems      = L"Gems";
std::wstring             g_TabLabelMaterials = L"Materials";
std::wstring             g_TabLabelRunes     = L"Runes";
std::wstring             g_ImeCompositionText;               // 0x1b0b0..0x1b0c8
std::atomic<bool>        g_MatchesDirty{ true };             // 0x1b0d0
std::atomic<uint64_t>    g_QueryGeneration{ 1 };             // 0x1b0d8
std::vector<MatchEntry>  g_Matches;                          // 0x1b230..0x1b240

bool                     g_PendingJumpIsShared  = false;     // 0x1b248
uint64_t                 g_PendingJumpPageIndex = 0;         // 0x1b250
bool                     g_PendingJumpClearFlag = false;     // 0x1b258
uint32_t                 g_PendingJumpRetryCount = 0;        // 0x1b25c
bool                     g_HasPendingJump       = false;     // 0x1b260

bool                     g_IsFocused            = false;     // 0x1b268
bool                     g_PlaceholderResolved  = false;     // 0x1b269
bool                     g_ImeComposing         = false;     // 0x1b26a
std::atomic<bool>        g_ShuttingDown{ false };            // 0x1b26b
std::atomic<bool>        g_WorkerReady{ false };             // 0x1b26c
std::atomic<bool>        g_WorkerFocusRequested{ false };    // 0x1b26d
bool                     g_SuppressEditSync     = false;     // 0x1b26e

uint32_t                 g_ImeCursorStart       = 0;         // 0x1b270
uint32_t                 g_ImeCursorEnd         = 0;         // 0x1b274
uint32_t                 g_ImeCursorPos         = 0;         // 0x1b278
uint32_t                 g_SelStart             = 0;
uint32_t                 g_SelEnd               = 0;
size_t                   g_SelectedIndex        = 0;         // 0x1b280
size_t                   g_ScrollOffset         = 0;         // 0x1b288
bool                     g_WasPanelActive          = false;
bool                     g_PlaceholderSelected     = false;
bool                     g_PlaceholderCleared      = false;
bool                     g_SelectAllOnFocus        = false;
bool                     g_PagePickerOpen          = false;
bool                     g_PersistedPagePickerOpen = false;
size_t                   g_PagePickerScrollRow     = 0;
bool                     g_PendingCenterPagePicker = false;
size_t                   g_PendingCenterPageIndex  = 0;
int32_t                  g_PendingWheelSteps    = 0;
int32_t                  g_PendingParentTabWheelSteps = 0;

std::wstring                                   g_CustomPersonalTabName;
std::vector<std::pair<uint64_t, std::wstring>> g_CustomSharedTabNames;
bool                                           g_IsRenamingTab        = false;
bool                                           g_RenamingTabIsShared  = true;
uint64_t                                       g_RenamingTabPageIndex = 0;
std::wstring                                   g_TabRenameText;

enum class ScrollbarDragMode : uint8_t {
	None           = 0,
	PagePicker     = 1,
	SearchDropdown = 2,
};
std::atomic<uint8_t>     g_ScrollbarDragMode{ 0 };
int                      g_ScrollbarDragGrabOffsetY = 0;

struct GridItemTooltipCacheEntry {
	uintptr_t    itemUnit = 0;
	std::wstring fullTooltip;
};
std::vector<GridItemTooltipCacheEntry> g_GridTooltipCache;
uint64_t                               g_GridTooltipCacheTickMs = 0;

SearchBarLayout          g_CurrentLayout{};                  // 0x1b290..0x1b3a0
uint64_t                 g_LastRenderTickMs     = 0;         // 0x1b3a8
uint64_t                 g_BuiltGeneration      = 0;         // 0x1b3b0
uint64_t                 g_LastBuildTickMs      = 0;         // 0x1b3b8
uint64_t                 g_LastTabSwitchTickMs  = 0;

std::atomic<DWORD>       g_WorkerThreadId{ 0 };              // 0x1b3c0
HANDLE                   g_WorkerThreadHandle   = nullptr;   // 0x1b3c8
HANDLE                   g_WorkerReadyEvent     = nullptr;   // 0x1b3d0
HHOOK                    g_MouseHook            = nullptr;   // 0x1b3d8
HHOOK                    g_KeyboardHook         = nullptr;   // 0x1b3e0
HWND                     g_EditHwnd             = nullptr;   // 0x1b3e8
uint8_t                  g_ForwardedKeyDown[256]{};          // 0x1b3f0..0x1b4ef
WNDPROC                  g_OriginalEditWndProc  = nullptr;   // 0x1b4f0
const D2RL::PluginContext* g_PluginCtx          = nullptr;
std::atomic<bool>        g_LoggedFirstStashFrame{ false };

constexpr uint64_t kLayoutActiveTimeoutMs  = 301;  // 0x12d ms in 0xC2B0 / 0xE620
constexpr uint64_t kPeriodicRefreshMs      = 3000; // 3000 ms in 0xE460
constexpr uint64_t kMinTabSwitchIntervalMs = 120;  // Coalesce rapid tab/page switches to prevent D2Render texture-cache & widget lifecycle thrashing

// 0xF5E0: Clears match list and bumps query generation (caller holds g_StateMutex)
void MarkQueryChangedLocked() noexcept {
	g_Matches.clear();
	g_HasPendingJump = false;
	g_EditSyncedQuery.clear();
	g_SelectedIndex = 0;
	g_ScrollOffset  = 0;
	g_QueryGeneration.fetch_add(1, std::memory_order_seq_cst);
	g_MatchesDirty.store(true, std::memory_order_release);
}

// 0xC2B0: Checks whether BankPanel was rendered within the last 301ms (caller holds g_StateMutex)
auto IsSearchBarActiveLocked(uint64_t nowTickMs) noexcept -> bool {
	if (!g_ShuttingDown.load(std::memory_order_acquire) &&
		g_CurrentLayout.valid &&
		g_LastRenderTickMs != 0 &&
		nowTickMs >= g_LastRenderTickMs &&
		(nowTickMs - g_LastRenderTickMs) < kLayoutActiveTimeoutMs) {
		return true;
	}
	g_IsFocused     = false;
	g_IsRenamingTab = false;
	g_TabRenameText.clear();
	g_EditSyncedQuery.clear();
	g_ScrollbarDragMode.store(static_cast<uint8_t>(ScrollbarDragMode::None), std::memory_order_release);
	g_ScrollbarDragGrabOffsetY = 0;
	return false;
}

// 0xC220: Thread-safe check whether search bar is currently active and focused
auto IsSearchBarFocusedAndActive() -> bool {
	const uint64_t nowTickMs = GetTickCount64();
	std::lock_guard<std::mutex> lock(g_StateMutex);
	if (g_ShuttingDown.load(std::memory_order_acquire)) {
		return false;
	}
	return IsSearchBarActiveLocked(nowTickMs) && g_IsFocused;
}

// 0xDAB0 / 0xDC50: Updates cached layout and render timestamp
void ClearCurrentLayout(uint64_t nowTickMs) {
	std::lock_guard<std::mutex> lock(g_StateMutex);
	g_CurrentLayout    = SearchBarLayout{};
	g_LastRenderTickMs = nowTickMs;
}

void UpdateCurrentLayout(const SearchBarLayout& layout, bool valid, uint64_t nowTickMs) {
	std::lock_guard<std::mutex> lock(g_StateMutex);
	g_CurrentLayout       = layout;
	g_CurrentLayout.valid = valid;
	g_LastRenderTickMs    = nowTickMs;
}

// 0x4F70: Resets all search state on plugin load/unload
void ResetSearchState() {
	g_ShuttingDown.store(true, std::memory_order_release);
	g_WorkerFocusRequested.store(false, std::memory_order_release);
	g_MatchesDirty.store(true, std::memory_order_release);
	g_QueryGeneration.store(1, std::memory_order_release);
	g_BuiltGeneration = 0;
	g_LastBuildTickMs = 0;

	std::lock_guard<std::mutex> lock(g_StateMutex);
	g_QueryText.clear();
	g_Matches.clear();
	g_HasPendingJump    = false;
	g_IsFocused         = false;
	g_EditSyncedQuery.clear();
	g_SearchPlaceholder   = L"Search";
	g_TabLabelPersonal    = L"Personal";
	g_TabLabelGems        = L"Gems";
	g_TabLabelMaterials   = L"Materials";
	g_TabLabelRunes       = L"Runes";
	g_PlaceholderResolved = false;
	g_ImeCompositionText.clear();
	g_ImeCursorStart    = 0;
	g_ImeCursorEnd      = 0;
	g_ImeCursorPos      = 0;
	g_SelStart          = 0;
	g_SelEnd            = 0;
	g_ImeComposing      = false;
	g_SelectedIndex     = 0;
	g_ScrollOffset      = 0;
	g_WasPanelActive          = false;
	g_PlaceholderSelected     = false;
	g_PlaceholderCleared      = false;
	g_SelectAllOnFocus        = false;
	g_PagePickerOpen          = false;
	g_PersistedPagePickerOpen = false;
	g_PagePickerScrollRow     = 0;
	g_PendingCenterPagePicker = false;
	g_PendingCenterPageIndex  = 0;
	g_PendingWheelSteps = 0;
	g_PendingParentTabWheelSteps = 0;
	g_CustomPersonalTabName.clear();
	g_CustomSharedTabNames.clear();
	g_IsRenamingTab        = false;
	g_RenamingTabIsShared  = true;
	g_RenamingTabPageIndex = 0;
	g_TabRenameText.clear();
	g_ScrollbarDragMode.store(static_cast<uint8_t>(ScrollbarDragMode::None), std::memory_order_release);
	g_ScrollbarDragGrabOffsetY = 0;
	g_GridTooltipCache.clear();
	g_GridTooltipCacheTickMs = 0;
	g_CurrentLayout     = SearchBarLayout{};
	g_LastRenderTickMs  = 0;
	g_LastTabSwitchTickMs = 0;
}

// Helpers for looking up and updating client-side custom tab names (caller holds g_StateMutex)
auto TrimTabNameWhitespace(std::wstring_view input) -> std::wstring {
	size_t first = 0;
	while (first < input.size() && (input[first] == L' ' || input[first] == L'\t' || input[first] == L'\r' || input[first] == L'\n')) {
		++first;
	}
	size_t last = input.size();
	while (last > first && (input[last - 1] == L' ' || input[last - 1] == L'\t' || input[last - 1] == L'\r' || input[last - 1] == L'\n')) {
		--last;
	}
	std::wstring out(input.substr(first, last - first));
	if (out.size() > kMaxTabNameChars) {
		out.resize(kMaxTabNameChars);
		const auto lastUnit = static_cast<uint16_t>(out[kMaxTabNameChars - 1]);
		if (static_cast<uint16_t>(lastUnit - 0xD800u) < 0x0400u) {
			out.resize(kMaxTabNameChars - 1);
		}
	}
	return out;
}

auto GetCustomSharedTabNameLocked(uint64_t pageIndex) -> std::wstring {
	for (const auto& [idx, name] : g_CustomSharedTabNames) {
		if (idx == pageIndex) {
			return name;
		}
	}
	return {};
}

auto SetCustomTabNameLocked(bool isShared, uint64_t pageIndex, std::wstring_view rawName) -> bool {
	std::wstring cleaned = TrimTabNameWhitespace(rawName);
	bool         changed = false;
	if (!isShared) {
		if (pageIndex == 0 && g_CustomPersonalTabName != cleaned) {
			g_CustomPersonalTabName = std::move(cleaned);
			changed                 = true;
		}
	} else {
		bool found = false;
		for (auto it = g_CustomSharedTabNames.begin(); it != g_CustomSharedTabNames.end(); ++it) {
			if (it->first == pageIndex) {
				found = true;
				if (cleaned.empty()) {
					g_CustomSharedTabNames.erase(it);
					changed = true;
				} else if (it->second != cleaned) {
					it->second = std::move(cleaned);
					changed    = true;
				}
				break;
			}
		}
		if (!found && !cleaned.empty()) {
			g_CustomSharedTabNames.emplace_back(pageIndex, std::move(cleaned));
			std::sort(
				g_CustomSharedTabNames.begin(),
				g_CustomSharedTabNames.end(),
				[](const auto& a, const auto& b) noexcept { return a.first < b.first; });
			changed = true;
		}
	}
	if (changed) {
		g_MatchesDirty.store(true, std::memory_order_release);
	}
	return changed;
}

auto CommitActiveTabRenameLocked() -> bool {
	if (!g_IsRenamingTab) {
		return false;
	}
	const bool     isShared  = g_RenamingTabIsShared;
	const uint64_t pageIndex = g_RenamingTabPageIndex;
	const auto     textCopy  = std::move(g_TabRenameText);
	g_IsRenamingTab          = false;
	g_TabRenameText.clear();
	return SetCustomTabNameLocked(isShared, pageIndex, textCopy);
}

auto SnapshotPersistedConfigLocked() -> StashSearchConfig {
	StashSearchConfig cfg{};
	cfg.tabFlyoutExpanded = g_PersistedPagePickerOpen;
	cfg.personalTabName   = g_CustomPersonalTabName;
	cfg.sharedTabNames    = g_CustomSharedTabNames;
	return cfg;
}

// Loads and saves `tab_flyout_expanded` and custom tab names in the user's D2RLoader plugin configuration (`stash-search.toml`)
constexpr size_t kMaxConfigBytes = 16384;

void LoadPersistedConfig(const D2RL::PluginContext* ctx) noexcept {
	if (ctx == nullptr) {
		return;
	}
	const std::string defaultToml = UpdateTabFlyoutExpandedConfig({}, false);
	(void)ctx->EnsureConfig(defaultToml.c_str());

	char              buf[kMaxConfigBytes]{};
	uint32_t          required = 0;
	StashSearchConfig cfg{};
	if (ctx->ReadConfig(buf, static_cast<uint32_t>(sizeof(buf)), &required) &&
		required <= sizeof(buf)) {
		const size_t len = strnlen_s(buf, sizeof(buf));
		if (len < sizeof(buf)) {
			cfg = ParseStashSearchConfig(std::string_view(buf, len), false);
		}
	}
	std::lock_guard<std::mutex> lock(g_StateMutex);
	g_PersistedPagePickerOpen = cfg.tabFlyoutExpanded;
	g_PagePickerOpen          = cfg.tabFlyoutExpanded;
	g_CustomPersonalTabName   = std::move(cfg.personalTabName);
	g_CustomSharedTabNames    = std::move(cfg.sharedTabNames);
}

void SavePersistedConfig(const D2RL::PluginContext* ctx, const StashSearchConfig& cfg) noexcept {
	if (ctx == nullptr) {
		return;
	}
	char        buf[kMaxConfigBytes]{};
	uint32_t    required = 0;
	std::string currentText;
	if (ctx->ReadConfig(buf, static_cast<uint32_t>(sizeof(buf)), &required) &&
		required <= sizeof(buf)) {
		const size_t len = strnlen_s(buf, sizeof(buf));
		if (len < sizeof(buf)) {
			currentText.assign(buf, len);
		}
	}
	const std::string updated = SerializeStashSearchConfig(currentText, cfg);
	if (updated.size() < kMaxConfigBytes) {
		(void)ctx->WriteConfig(updated.c_str());
	}
}

// 0x9CB0 & 0x9C30: Finds the main game window for the current process
struct EnumGameWindowContext {
	DWORD targetPid  = 0;
	HWND  bestHwnd   = nullptr;
	int64_t bestArea = 0;
};

BOOL CALLBACK EnumGameWindowsProc(HWND hwnd, LPARAM lParam) {
	auto* ctx = reinterpret_cast<EnumGameWindowContext*>(lParam);
	if (ctx == nullptr) {
		return FALSE;
	}
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	if (pid != ctx->targetPid || !IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER) != nullptr) {
		return TRUE;
	}
	RECT rc{};
	if (!GetClientRect(hwnd, &rc)) {
		return TRUE;
	}
	const int64_t w    = static_cast<int64_t>(rc.right) - static_cast<int64_t>(rc.left);
	const int64_t h    = static_cast<int64_t>(rc.bottom) - static_cast<int64_t>(rc.top);
	const int64_t area = (w > 0 && h > 0) ? (w * h) : 0;
	if (area >= ctx->bestArea) {
		ctx->bestHwnd = hwnd;
		ctx->bestArea = area;
	}
	return TRUE;
}

auto FindGameWindow() noexcept -> HWND {
	const DWORD currentPid = GetCurrentProcessId();
	const HWND  fgHwnd     = GetForegroundWindow();
	if (fgHwnd != nullptr) {
		DWORD fgPid = 0;
		if (GetWindowThreadProcessId(fgHwnd, &fgPid) != 0 && fgPid == currentPid) {
			return fgHwnd;
		}
	}
	EnumGameWindowContext ctx{};
	ctx.targetPid = currentPid;
	EnumWindows(&EnumGameWindowsProc, reinterpret_cast<LPARAM>(&ctx));
	return ctx.bestHwnd;
}

// Synchronizes the worker thread's keyboard layout and modifier/toggle key state with the game window thread
// before TranslateMessage / DispatchMessageW processes forwarded keystrokes on the message-only EDIT control.
void SyncWorkerThreadKeyboardState() noexcept {
	const HWND gameHwnd = FindGameWindow();
	if (gameHwnd != nullptr) {
		const DWORD gameTid = GetWindowThreadProcessId(gameHwnd, nullptr);
		if (gameTid != 0) {
			const HKL gameHkl = GetKeyboardLayout(gameTid);
			if (gameHkl != nullptr && GetKeyboardLayout(0) != gameHkl) {
				ActivateKeyboardLayout(gameHkl, 0);
			}
		}
	}
	BYTE ks[256]{};
	if (!GetKeyboardState(ks)) {
		std::memset(ks, 0, sizeof(ks));
	}
	const auto syncDown = [&ks](int vk) noexcept {
		if ((GetAsyncKeyState(vk) & 0x8000) != 0) {
			ks[vk] |= 0x80u;
		} else {
			ks[vk] &= static_cast<BYTE>(~0x80u);
		}
	};
	syncDown(VK_SHIFT);
	syncDown(VK_LSHIFT);
	syncDown(VK_RSHIFT);
	syncDown(VK_CONTROL);
	syncDown(VK_LCONTROL);
	syncDown(VK_RCONTROL);
	syncDown(VK_MENU);
	syncDown(VK_LMENU);
	syncDown(VK_RMENU);
	if ((GetKeyState(VK_CAPITAL) & 1) != 0) {
		ks[VK_CAPITAL] |= 1u;
	} else {
		ks[VK_CAPITAL] &= static_cast<BYTE>(~1u);
	}
	if ((GetKeyState(VK_NUMLOCK) & 1) != 0) {
		ks[VK_NUMLOCK] |= 1u;
	} else {
		ks[VK_NUMLOCK] &= static_cast<BYTE>(~1u);
	}
	SetKeyboardState(ks);
}

// 0xE300: Reads text of an HWND into std::wstring
auto ReadWindowTextWide(HWND hwnd, std::wstring& outText) -> bool {
	if (hwnd == nullptr) {
		return false;
	}
	const int len = GetWindowTextLengthW(hwnd);
	if (len < 0) {
		return false;
	}
	std::wstring buf(static_cast<size_t>(len) + 1, L'\0');
	const int copied = GetWindowTextW(hwnd, buf.data(), len + 1);
	if (copied < 0) {
		return false;
	}
	buf.resize(static_cast<size_t>(copied));
	outText = std::move(buf);
	return true;
}

// 0xFCC0: Syncs text from hidden EDIT control into g_QueryText or g_TabRenameText
void SyncEditControlText(HWND hwnd) {
	if (hwnd == nullptr || g_SuppressEditSync) {
		return;
	}
	std::wstring rawText;
	if (!ReadWindowTextWide(hwnd, rawText)) {
		return;
	}
	size_t maxChars = kMaxQueryChars;
	{
		std::lock_guard<std::mutex> lock(g_StateMutex);
		if (g_IsRenamingTab) {
			maxChars = kMaxTabNameChars;
		}
	}
	std::wstring clamped = rawText;
	if (clamped.size() > maxChars) {
		clamped.resize(maxChars);
		// Avoid splitting a UTF-16 high surrogate at the boundary (0xD800..0xDBFF)
		const auto lastUnit = static_cast<uint16_t>(clamped[maxChars - 1]);
		if (static_cast<uint16_t>(lastUnit - 0xD800u) < 0x0400u) {
			clamped.resize(maxChars - 1);
		}
	}
	DWORD selStart = 0;
	DWORD selEnd   = 0;
	SendMessageW(hwnd, EM_GETSEL, reinterpret_cast<WPARAM>(&selStart), reinterpret_cast<LPARAM>(&selEnd));

	if (clamped != rawText) {
		g_SuppressEditSync = true;
		const BOOL ok      = SetWindowTextW(hwnd, clamped.c_str());
		g_SuppressEditSync = false;
		const auto maxPos  = static_cast<DWORD>(std::min<size_t>(clamped.size(), 0xFFFFFFFFu));
		selStart           = std::min(selStart, maxPos);
		selEnd             = std::min(selEnd, maxPos);
		SendMessageW(hwnd, EM_SETSEL, selStart, selEnd);
		if (!ok) {
			return;
		}
	}

	const auto maxPos = static_cast<uint32_t>(std::min<size_t>(clamped.size(), 0xFFFFFFFFu));
	const uint32_t clampedSelStart = std::min<uint32_t>(selStart, maxPos);
	const uint32_t clampedSelEnd   = std::min<uint32_t>(selEnd, maxPos);

	const uint64_t nowTickMs   = GetTickCount64();
	const bool     focusActive = g_WorkerFocusRequested.load(std::memory_order_acquire);
	std::lock_guard<std::mutex> lock(g_StateMutex);
	if (focusActive && IsSearchBarActiveLocked(nowTickMs) && g_IsFocused) {
		g_SelStart        = std::min(clampedSelStart, clampedSelEnd);
		g_SelEnd          = std::max(clampedSelStart, clampedSelEnd);
		g_EditSyncedQuery = clamped;
		if (g_IsRenamingTab) {
			g_TabRenameText = clamped;
		} else {
			if (!clamped.empty()) {
				g_PlaceholderSelected = false;
				g_PlaceholderCleared  = true;
			}
			if (g_QueryText != clamped) {
				g_QueryText = clamped;
				if (clamped.empty()) {
					g_PlaceholderSelected = false;
					g_PlaceholderCleared  = true;
				}
				MarkQueryChangedLocked();
				g_EditSyncedQuery = clamped;
			}
		}
	}
}

// Centers the Side Tab Flyout scroll window on a 0-based shared stash page index (caller holds g_StateMutex)
void CenterPagePickerOnSharedPageLocked(size_t targetPageIndex, size_t totalPagesHint = 0) noexcept {
	g_PendingCenterPagePicker = true;
	g_PendingCenterPageIndex  = targetPageIndex;
	const size_t totalRows    = (g_CurrentLayout.pagePickerTotalRows != 0)
									? g_CurrentLayout.pagePickerTotalRows
									: totalPagesHint;
	const size_t visRows      = (g_CurrentLayout.pagePickerVisRows != 0)
									? g_CurrentLayout.pagePickerVisRows
									: kMaxPageGridVisibleRows;
	if (totalRows <= visRows || visRows == 0) {
		g_PagePickerScrollRow = 0;
		return;
	}
	const size_t maxScroll = totalRows - visRows;
	const size_t halfVis   = (visRows - 1) / 2;
	const size_t desired   = (targetPageIndex > halfVis) ? (targetPageIndex - halfVis) : 0;
	g_PagePickerScrollRow  = std::min(desired, maxScroll);
}

// 0xAF70: Handles VK_RETURN — commits inline tab rename, executes slash page jump, or jumps to selected item page
auto HandleEnterKey() -> bool {
	const uint64_t    nowTickMs  = GetTickCount64();
	bool              unfocus    = false;
	bool              saveConfig = false;
	StashSearchConfig savedCfg{};
	{
		std::lock_guard<std::mutex> lock(g_StateMutex);
		if (!IsSearchBarActiveLocked(nowTickMs) || !g_IsFocused) {
			return false;
		}
		if (g_IsRenamingTab) {
			if (CommitActiveTabRenameLocked()) {
				saveConfig = true;
				savedCfg   = SnapshotPersistedConfigLocked();
			}
			g_IsFocused           = false;
			g_PlaceholderSelected = false;
			g_PlaceholderCleared  = false;
			g_SelectAllOnFocus    = false;
			g_EditSyncedQuery.clear();
			g_ImeCompositionText.clear();
			g_ImeCursorStart = 0;
			g_ImeCursorEnd   = 0;
			g_ImeCursorPos   = 0;
			g_SelStart       = 0;
			g_SelEnd         = 0;
			g_ImeComposing   = false;
			unfocus          = true;
		} else {
			const SlashPageCommand slashCmd = ParseSlashPageCommand(g_QueryText);
			if (slashCmd.state == SlashCommandState::ValidPage && slashCmd.pageNumber != 0) {
				g_PendingJumpIsShared   = true;
				g_PendingJumpPageIndex  = slashCmd.pageNumber - 1;
				g_PendingJumpClearFlag  = false;
				g_PendingJumpRetryCount = 0;
				g_HasPendingJump        = true;
				CenterPagePickerOnSharedPageLocked(static_cast<size_t>(slashCmd.pageNumber - 1));
				g_QueryText.clear();
				MarkQueryChangedLocked();
				g_IsFocused = false;
				g_EditSyncedQuery.clear();
				g_ImeCompositionText.clear();
				g_ImeCursorStart = 0;
				g_ImeCursorEnd   = 0;
				g_ImeCursorPos   = 0;
				g_SelStart       = 0;
				g_SelEnd         = 0;
				g_ImeComposing   = false;
				unfocus          = true;
			} else if (slashCmd.state == SlashCommandState::None) {
				if (g_SelectedIndex < g_Matches.size() && g_Matches[g_SelectedIndex].hasTarget) {
					g_PendingJumpIsShared   = g_Matches[g_SelectedIndex].target.isShared;
					g_PendingJumpPageIndex  = g_Matches[g_SelectedIndex].target.pageIndex;
					g_PendingJumpClearFlag  = false;
					g_PendingJumpRetryCount = 0;
					g_HasPendingJump        = true;
					if (g_PendingJumpIsShared) {
						CenterPagePickerOnSharedPageLocked(static_cast<size_t>(g_PendingJumpPageIndex));
					}
					// Keep g_QueryText intact so the matching item stays highlighted on the target page!
					g_IsFocused = false;
					g_EditSyncedQuery.clear();
					g_ImeCompositionText.clear();
					g_ImeCursorStart = 0;
					g_ImeCursorEnd   = 0;
					g_ImeCursorPos   = 0;
					g_SelStart       = 0;
					g_SelEnd         = 0;
					g_ImeComposing   = false;
					unfocus          = true;
				}
			}
		}
	}
	if (saveConfig) {
		SavePersistedConfig(g_PluginCtx, savedCfg);
	}
	if (unfocus) {
		g_WorkerFocusRequested.store(false, std::memory_order_release);
		const DWORD tid = g_WorkerThreadId.load(std::memory_order_acquire);
		if (tid != 0 && g_WorkerReady.load(std::memory_order_acquire) && !g_ShuttingDown.load(std::memory_order_acquire)) {
			PostThreadMessageW(tid, kWorkerMsgUnfocusHideEdit, 0, 0);
		}
	}
	return true;
}

// 0xB280: Handles VK_UP, VK_DOWN, VK_PRIOR, VK_NEXT navigation in dropdown
auto HandleNavigationKey(uint32_t vk) -> bool {
	const uint64_t nowTickMs = GetTickCount64();
	std::lock_guard<std::mutex> lock(g_StateMutex);
	if (!IsSearchBarActiveLocked(nowTickMs) || !g_IsFocused) {
		return false;
	}
	if (g_IsRenamingTab) {
		return true;
	}
	const size_t matchCount = g_Matches.size();
	size_t       step       = 4;
	if (g_CurrentLayout.visibleRowCount != 0) {
		step = g_CurrentLayout.visibleRowCount;
	}
	size_t desired = g_SelectedIndex;
	if (vk == VK_PRIOR) { // 0x21 Page Up
		desired = (g_SelectedIndex > step) ? (g_SelectedIndex - step) : 0;
	} else if (vk == VK_NEXT) { // 0x22 Page Down
		if (g_SelectedIndex < matchCount) {
			desired = std::min(g_SelectedIndex + step, matchCount - 1);
		}
	} else if (vk == VK_UP) { // 0x26 Up
		if (g_SelectedIndex != 0) {
			desired = g_SelectedIndex - 1;
		}
	} else if (vk == VK_DOWN) { // 0x28 Down
		if (g_SelectedIndex + 1 < matchCount) {
			desired = g_SelectedIndex + 1;
		}
	} else {
		return false;
	}
	AdjustDropdownSelectionAndScroll(matchCount, step, desired, g_SelectedIndex, g_ScrollOffset);
	return true;
}

// 0x6730: Unfocuses the message-only EDIT control on the worker thread without touching the game window's Win32 focus
void UnfocusAndHideEditControl() noexcept {
	std::memset(g_ForwardedKeyDown, 0, sizeof(g_ForwardedKeyDown));
	g_WorkerFocusRequested.store(false, std::memory_order_release);
	if (g_EditHwnd == nullptr || !IsWindow(g_EditHwnd)) {
		return;
	}
	HIMC himc = ImmGetContext(g_EditHwnd);
	if (himc != nullptr) {
		ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
		ImmSetOpenStatus(himc, FALSE);
		ImmReleaseContext(g_EditHwnd, himc);
	}
}

// 0x6850: Restores WndProc and destroys the hidden EDIT control
void DestroyEditControl() noexcept {
	const HWND hwnd = g_EditHwnd;
	g_EditHwnd      = nullptr;
	if (hwnd != nullptr && IsWindow(hwnd)) {
		SendMessageW(hwnd, kWorkerMsgDestroyEdit, 0, 0);
	} else {
		g_OriginalEditWndProc = nullptr;
	}
}

// 0x9530: Subclassed WndProc for the hidden EDIT control
LRESULT CALLBACK SubclassedEditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	const WNDPROC origProc = g_OriginalEditWndProc;
	if (msg == kWorkerMsgDestroyEdit) {
		if (origProc != nullptr) {
			SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(origProc));
		}
		g_OriginalEditWndProc = nullptr;
		DestroyWindow(hwnd);
		return 0;
	}
	if (msg == WM_PAINT) {
		PAINTSTRUCT ps{};
		BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
		return 0;
	}
	if (msg == WM_ERASEBKGND) {
		return 1;
	}
	if (msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP || msg == WM_SYSCHAR) {
		// Suppress Alt combinations inside the hidden EDIT control so DefWindowProcW never calls MessageBeep(0)
		return 0;
	}
	if (msg == WM_KEYDOWN) {
		if (wParam == VK_ESCAPE) {
			const uint64_t nowTickMs = GetTickCount64();
			bool           hadQueryOrPicker = false;
			{
				std::lock_guard<std::mutex> lock(g_StateMutex);
				if (IsSearchBarActiveLocked(nowTickMs)) {
					if (g_IsRenamingTab) {
						g_IsRenamingTab = false;
						g_TabRenameText.clear();
						hadQueryOrPicker = true;
					} else if (!g_QueryText.empty()) {
						g_QueryText.clear();
						MarkQueryChangedLocked();
						hadQueryOrPicker = true;
					} else if (g_PlaceholderSelected || g_PlaceholderCleared) {
						hadQueryOrPicker = true;
					}
					g_IsFocused           = false;
					g_PlaceholderSelected = false;
					g_PlaceholderCleared  = false;
					g_SelectAllOnFocus    = false;
					g_EditSyncedQuery.clear();
					g_ImeCompositionText.clear();
					g_ImeCursorStart = 0;
					g_ImeCursorEnd   = 0;
					g_ImeCursorPos   = 0;
					g_SelStart       = 0;
					g_SelEnd         = 0;
					g_ImeComposing   = false;
				}
			}
			g_WorkerFocusRequested.store(false, std::memory_order_release);
			UnfocusAndHideEditControl();
			// If the search box was already empty when Escape was pressed, forward Escape to the game window so Stash closes
			if (!hadQueryOrPicker) {
				const HWND gameHwnd = FindGameWindow();
				if (gameHwnd != nullptr && IsWindow(gameHwnd)) {
					PostMessageW(gameHwnd, WM_KEYDOWN, VK_ESCAPE, lParam);
					PostMessageW(gameHwnd, WM_KEYUP, VK_ESCAPE, lParam | static_cast<LPARAM>(0xC0000000ULL));
				}
			}
			return 0;
		}
		if (wParam == VK_RETURN) {
			HandleEnterKey();
			return 0;
		}
		if (wParam == VK_TAB) {
			return 0;
		}
		if (wParam == 'A' && GetKeyState(VK_CONTROL) < 0 && GetKeyState(VK_MENU) >= 0) {
			if (GetWindowTextLengthW(hwnd) <= 0) {
				std::lock_guard<std::mutex> lock(g_StateMutex);
				if (!g_IsRenamingTab) {
					g_PlaceholderSelected = true;
					g_PlaceholderCleared  = false;
				}
			}
			SendMessageW(hwnd, EM_SETSEL, 0, static_cast<LPARAM>(-1));
			SyncEditControlText(hwnd);
			return 0;
		}
		if (wParam == VK_BACK) {
			DWORD s0 = 0;
			DWORD s1 = 0;
			SendMessageW(hwnd, EM_GETSEL, reinterpret_cast<WPARAM>(&s0), reinterpret_cast<LPARAM>(&s1));
			if (s0 == 0 && s1 == 0) {
				{
					std::lock_guard<std::mutex> lock(g_StateMutex);
					if (!g_IsRenamingTab) {
						g_PlaceholderSelected = false;
						g_PlaceholderCleared  = true;
					}
				}
				SyncEditControlText(hwnd);
				return 0;
			}
		}
		if (wParam == VK_DELETE) {
			DWORD s0 = 0;
			DWORD s1 = 0;
			SendMessageW(hwnd, EM_GETSEL, reinterpret_cast<WPARAM>(&s0), reinterpret_cast<LPARAM>(&s1));
			const int curLen = GetWindowTextLengthW(hwnd);
			if (s0 == s1 && curLen >= 0 && static_cast<int>(s0) >= curLen) {
				{
					std::lock_guard<std::mutex> lock(g_StateMutex);
					if (!g_IsRenamingTab) {
						g_PlaceholderSelected = false;
						g_PlaceholderCleared  = true;
					}
				}
				SyncEditControlText(hwnd);
				return 0;
			}
		}
		if (wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_HOME || wParam == VK_END) {
			if (GetWindowTextLengthW(hwnd) <= 0) {
				std::lock_guard<std::mutex> lock(g_StateMutex);
				if (!g_IsRenamingTab) {
					g_PlaceholderSelected = false;
					g_PlaceholderCleared  = true;
				}
			}
		}
		if (wParam == VK_PRIOR || wParam == VK_NEXT || wParam == VK_UP || wParam == VK_DOWN) {
			HandleNavigationKey(static_cast<uint32_t>(wParam));
			return 0;
		}
	} else if (msg == WM_CHAR) {
		if (wParam == 1) { // Ctrl+A
			if (GetWindowTextLengthW(hwnd) <= 0) {
				std::lock_guard<std::mutex> lock(g_StateMutex);
				if (!g_IsRenamingTab) {
					g_PlaceholderSelected = true;
					g_PlaceholderCleared  = false;
				}
			}
			SendMessageW(hwnd, EM_SETSEL, 0, static_cast<LPARAM>(-1));
			SyncEditControlText(hwnd);
			return 0;
		}
		if (wParam == VK_BACK) {
			DWORD s0 = 0;
			DWORD s1 = 0;
			SendMessageW(hwnd, EM_GETSEL, reinterpret_cast<WPARAM>(&s0), reinterpret_cast<LPARAM>(&s1));
			if (s0 == 0 && s1 == 0) {
				return 0;
			}
		} else if (wParam < 0x20 || wParam == 0x7F) {
			// Allow only Ctrl+C (0x03), Ctrl+V (0x16), Ctrl+X (0x18), Ctrl+Z (0x1A); suppress all other control chars so EditWndProc never beeps
			if (wParam != 0x03 && wParam != 0x16 && wParam != 0x18 && wParam != 0x1A) {
				return 0;
			}
		} else {
			// Prevent EM_LIMITTEXT from calling MessageBeep(0) when already at maxChars with no selection
			size_t maxChars = kMaxQueryChars;
			{
				std::lock_guard<std::mutex> lock(g_StateMutex);
				if (g_IsRenamingTab) {
					maxChars = kMaxTabNameChars;
				}
			}
			DWORD s0 = 0;
			DWORD s1 = 0;
			SendMessageW(hwnd, EM_GETSEL, reinterpret_cast<WPARAM>(&s0), reinterpret_cast<LPARAM>(&s1));
			const int curLen = GetWindowTextLengthW(hwnd);
			if (s0 == s1 && curLen >= static_cast<int>(maxChars)) {
				return 0;
			}
		}
	} else if (msg == WM_IME_STARTCOMPOSITION) {
		{
			std::lock_guard<std::mutex> lock(g_StateMutex);
			g_ImeComposing = true;
		}
		return origProc != nullptr ? CallWindowProcW(origProc, hwnd, msg, wParam, lParam)
								   : DefWindowProcW(hwnd, msg, wParam, lParam);
	} else if (msg == WM_IME_COMPOSITION) {
		const LRESULT res = origProc != nullptr ? CallWindowProcW(origProc, hwnd, msg, wParam, lParam)
												: DefWindowProcW(hwnd, msg, wParam, lParam);
		std::wstring compStr;
		uint32_t     cursorPos = 0;
		HIMC         himc      = ImmGetContext(hwnd);
		if (himc != nullptr) {
			const LONG byteLen = ImmGetCompositionStringW(himc, GCS_COMPSTR, nullptr, 0);
			if (byteLen > 0) {
				compStr.resize(static_cast<size_t>(byteLen) / sizeof(wchar_t));
				ImmGetCompositionStringW(himc, GCS_COMPSTR, compStr.data(), static_cast<DWORD>(byteLen));
			}
			const LONG cursor = ImmGetCompositionStringW(himc, GCS_CURSORPOS, nullptr, 0);
			if (cursor > 0) {
				cursorPos = static_cast<uint32_t>(cursor);
			}
			ImmReleaseContext(hwnd, himc);
		}
		DWORD selStart = 0;
		DWORD selEnd   = 0;
		SendMessageW(hwnd, EM_GETSEL, reinterpret_cast<WPARAM>(&selStart), reinterpret_cast<LPARAM>(&selEnd));
		{
			std::lock_guard<std::mutex> lock(g_StateMutex);
			g_ImeComposing       = !compStr.empty();
			g_ImeCompositionText = std::move(compStr);
			g_ImeCursorStart     = selStart;
			g_ImeCursorEnd       = selEnd;
			g_ImeCursorPos       = cursorPos;
		}
		if ((lParam & GCS_RESULTSTR) != 0) {
			SyncEditControlText(hwnd);
		}
		return res;
	} else if (msg == WM_IME_ENDCOMPOSITION || msg == WM_KILLFOCUS) {
		{
			std::lock_guard<std::mutex> lock(g_StateMutex);
			if (msg == WM_KILLFOCUS) {
				g_IsFocused = false;
			}
			g_ImeCompositionText.clear();
			g_ImeCursorStart = 0;
			g_ImeCursorEnd   = 0;
			g_ImeCursorPos   = 0;
			g_ImeComposing   = false;
		}
		if (msg == WM_KILLFOCUS) {
			g_WorkerFocusRequested.store(false, std::memory_order_release);
		}
		const LRESULT res = origProc != nullptr ? CallWindowProcW(origProc, hwnd, msg, wParam, lParam)
												: DefWindowProcW(hwnd, msg, wParam, lParam);
		if (msg == WM_IME_ENDCOMPOSITION) {
			SyncEditControlText(hwnd);
		}
		return res;
	}

	const LRESULT res = origProc != nullptr ? CallWindowProcW(origProc, hwnd, msg, wParam, lParam)
											: DefWindowProcW(hwnd, msg, wParam, lParam);
	if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR || msg == WM_PASTE || msg == WM_CUT || msg == WM_CLEAR || msg == WM_SETTEXT || msg == WM_UNICHAR) {
		SyncEditControlText(hwnd);
	}
	return res;
}

// 0x3080: Creates or updates the message-only Win32 EDIT control without parenting to or stealing focus from gameHwnd
void SyncHiddenEditWindow() {
	if (g_EditHwnd != nullptr && !IsWindow(g_EditHwnd)) {
		g_EditHwnd            = nullptr;
		g_OriginalEditWndProc = nullptr;
	}
	if (g_EditHwnd == nullptr) {
		const auto hInst = GetModuleHandleW(nullptr);
		const HWND edit  = CreateWindowExW(
			0,
			L"EDIT",
			L"",
			WS_CHILD | ES_AUTOHSCROLL,
			0,
			0,
			1,
			1,
			HWND_MESSAGE,
			nullptr,
			hInst,
			nullptr);
		g_EditHwnd = edit;
		if (edit == nullptr) {
			return;
		}
		SendMessageW(edit, EM_LIMITTEXT, kMaxQueryChars, 0);
		SetLastError(0);
		const LONG_PTR prevProc = SetWindowLongPtrW(edit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&SubclassedEditWndProc));
		if (prevProc == 0 && GetLastError() != 0) {
			DestroyWindow(edit);
			g_EditHwnd            = nullptr;
			g_OriginalEditWndProc = nullptr;
			return;
		}
		g_OriginalEditWndProc = reinterpret_cast<WNDPROC>(prevProc);
	}

	const uint64_t nowTickMs     = GetTickCount64();
	bool           active        = false;
	bool           alreadySynced = false;
	bool           selectAll     = false;
	size_t         maxChars      = kMaxQueryChars;
	std::wstring   queryCopy;
	{
		std::lock_guard<std::mutex> lock(g_StateMutex);
		if (IsSearchBarActiveLocked(nowTickMs) && g_IsFocused) {
			active             = true;
			maxChars           = g_IsRenamingTab ? kMaxTabNameChars : kMaxQueryChars;
			queryCopy          = g_IsRenamingTab ? g_TabRenameText : g_QueryText;
			selectAll          = g_SelectAllOnFocus;
			g_SelectAllOnFocus = false;
			alreadySynced      = (!selectAll && !g_EditSyncedQuery.empty() && g_EditSyncedQuery == queryCopy &&
								  g_WorkerFocusRequested.load(std::memory_order_acquire));
		}
	}
	if (!active) {
		UnfocusAndHideEditControl();
		return;
	}

	SendMessageW(g_EditHwnd, EM_LIMITTEXT, maxChars, 0);
	if (queryCopy.size() > maxChars) {
		queryCopy.resize(maxChars);
		const auto lastUnit = static_cast<uint16_t>(queryCopy[maxChars - 1]);
		if (static_cast<uint16_t>(lastUnit - 0xD800u) < 0x0400u) {
			queryCopy.resize(maxChars - 1);
		}
	}

	if (!alreadySynced) {
		g_SuppressEditSync = true;
		SetWindowTextW(g_EditHwnd, queryCopy.c_str());
		const auto endPos   = static_cast<WPARAM>(std::min<size_t>(queryCopy.size(), 0xFFFFFFFFu));
		const auto startPos = (selectAll && endPos > 0) ? static_cast<WPARAM>(0) : endPos;
		SendMessageW(g_EditHwnd, EM_SETSEL, startPos, endPos);
		g_SuppressEditSync = false;
		{
			std::lock_guard<std::mutex> lock(g_StateMutex);
			g_EditSyncedQuery = queryCopy;
			g_SelStart        = static_cast<uint32_t>(startPos);
			g_SelEnd          = static_cast<uint32_t>(endPos);
		}
	}

	g_WorkerFocusRequested.store(true, std::memory_order_release);
}

// 0xBDD0: Hit-tests dropdown rows (caller holds g_StateMutex)
struct DropdownHitResult {
	uint64_t matchIndex = 0;
	bool     hit        = false;
};

auto HitTestDropdownRowLocked(int clientX, int clientY) noexcept -> DropdownHitResult {
	for (size_t i = 0; i < g_CurrentLayout.visibleRowCount && i < kMaxDropdownVisibleRows; ++i) {
		if (g_CurrentLayout.dropdownRows[i].Contains(clientX, clientY)) {
			const uint64_t idx = g_CurrentLayout.dropdownMatchIndices[i];
			if (idx < g_Matches.size()) {
				return { idx, true };
			}
		}
	}
	return { 0, false };
}

auto HitTestScrollbarRect(const IntRect& rect, float scale, int x, int y) noexcept -> bool {
	if (!rect.IsValid()) {
		return false;
	}
	const int padX = ScaleBorderThickness(6, scale);
	return x >= (rect.x - padX) && x < (rect.x + rect.width + padX) &&
		   y >= rect.y && y < (rect.y + rect.height);
}

// Updates page-picker or search-dropdown scroll offset from a client Y coordinate during scrollbar drag (caller holds g_StateMutex)
void ApplyScrollbarDragAtYLocked(ScrollbarDragMode mode, int clientY) noexcept {
	if (mode == ScrollbarDragMode::PagePicker) {
		if (!g_PagePickerOpen || !g_CurrentLayout.pagePickerHasScrollbar ||
			g_CurrentLayout.pagePickerTotalRows <= g_CurrentLayout.pagePickerVisRows) {
			return;
		}
		g_PendingCenterPagePicker = false;
		const IntRect& track      = g_CurrentLayout.pagePickerScrollbarTrack;
		const IntRect& thumb      = g_CurrentLayout.pagePickerScrollbarThumb;
		const size_t   maxScroll  = g_CurrentLayout.pagePickerTotalRows - g_CurrentLayout.pagePickerVisRows;
		const int      availH     = track.height - thumb.height;
		if (availH <= 0 || maxScroll == 0) {
			g_PagePickerScrollRow = 0;
			return;
		}
		const int    relY  = std::clamp(clientY - track.y - g_ScrollbarDragGrabOffsetY, 0, availH);
		const double ratio = static_cast<double>(relY) / static_cast<double>(availH);
		g_PagePickerScrollRow = std::clamp<size_t>(
			static_cast<size_t>(std::llround(ratio * static_cast<double>(maxScroll))),
			0,
			maxScroll);
	} else if (mode == ScrollbarDragMode::SearchDropdown) {
		const size_t matchCount = g_Matches.size();
		const size_t visRows    = std::min(g_CurrentLayout.visibleRowCount, matchCount);
		if (!g_CurrentLayout.hasScrollbar || visRows == 0 || matchCount <= visRows) {
			return;
		}
		const IntRect& track     = g_CurrentLayout.scrollbarTrack;
		const IntRect& thumb     = g_CurrentLayout.scrollbarThumb;
		const size_t   maxScroll = matchCount - visRows;
		const int      availH    = track.height - thumb.height;
		if (availH <= 0 || maxScroll == 0) {
			g_ScrollOffset = 0;
			return;
		}
		const int    relY      = std::clamp(clientY - track.y - g_ScrollbarDragGrabOffsetY, 0, availH);
		const double ratio     = static_cast<double>(relY) / static_cast<double>(availH);
		const size_t newScroll = std::clamp<size_t>(
			static_cast<size_t>(std::llround(ratio * static_cast<double>(maxScroll))),
			0,
			maxScroll);
		g_ScrollOffset = newScroll;
		if (g_SelectedIndex < g_ScrollOffset) {
			g_SelectedIndex = g_ScrollOffset;
		} else if (g_SelectedIndex >= g_ScrollOffset + visRows) {
			g_SelectedIndex = g_ScrollOffset + visRows - 1;
		}
	}
}

// 0xB670: Handles WM_LBUTTONDOWN in client coordinates
auto HandleMouseClick(int clientX, int clientY) -> bool {
	const uint64_t    nowTickMs      = GetTickCount64();
	bool              requestFocus   = false;
	bool              requestUnfocus = false;
	bool              consumed       = false;
	bool              saveConfig     = false;
	StashSearchConfig savedCfg{};
	{
		std::lock_guard<std::mutex> lock(g_StateMutex);
		if (!IsSearchBarActiveLocked(nowTickMs)) {
			return false;
		}
		g_ScrollbarDragMode.store(static_cast<uint8_t>(ScrollbarDragMode::None), std::memory_order_release);
		g_ScrollbarDragGrabOffsetY = 0;

		if (g_IsRenamingTab) {
			if (CommitActiveTabRenameLocked()) {
				saveConfig = true;
			}
			if (!g_CurrentLayout.searchBox.Contains(clientX, clientY) ||
				g_CurrentLayout.pageButtonRect.Contains(clientX, clientY)) {
				g_IsFocused           = false;
				g_PlaceholderSelected = false;
				g_PlaceholderCleared  = false;
				g_SelectAllOnFocus    = false;
				g_EditSyncedQuery.clear();
				g_ImeCompositionText.clear();
				g_ImeCursorStart = 0;
				g_ImeCursorEnd   = 0;
				g_ImeCursorPos   = 0;
				g_SelStart       = 0;
				g_SelEnd         = 0;
				g_ImeComposing   = false;
				requestUnfocus   = true;
			}
		}

		if (g_CurrentLayout.pageButtonRect.Contains(clientX, clientY)) {
			g_PagePickerOpen          = !g_PagePickerOpen;
			g_PersistedPagePickerOpen = g_PagePickerOpen;
			g_PagePickerScrollRow     = 0;
			g_PendingCenterPagePicker = false;
			saveConfig                = true;
			consumed                  = true;
		} else if (g_CurrentLayout.searchBox.Contains(clientX, clientY)) {
			g_IsFocused = true;
			g_EditSyncedQuery.clear();
			if (g_QueryText.empty()) {
				g_PlaceholderSelected = true;
				g_PlaceholderCleared  = false;
				g_SelectAllOnFocus    = false;
			} else {
				g_PlaceholderSelected = false;
				g_SelectAllOnFocus    = true;
				g_SelStart            = 0;
				g_SelEnd              = static_cast<uint32_t>(g_QueryText.size());
			}
			g_ImeCompositionText = g_QueryText;
			g_ImeCursorStart     = static_cast<uint32_t>(g_CurrentLayout.searchBox.x);
			g_ImeCursorEnd       = static_cast<uint32_t>(g_CurrentLayout.searchBox.y);
			g_ImeCursorPos       = static_cast<uint32_t>(g_CurrentLayout.searchBox.width);
			g_ImeComposing       = false;
			requestFocus         = true;
			requestUnfocus       = false;
			consumed             = true;
		} else if (g_PagePickerOpen &&
				   (g_CurrentLayout.pagePickerBox.Contains(clientX, clientY) ||
					(g_CurrentLayout.pagePickerHasScrollbar &&
					 HitTestScrollbarRect(g_CurrentLayout.pagePickerScrollbarTrack, g_CurrentLayout.scale, clientX, clientY)))) {
			if (g_CurrentLayout.pagePickerHasScrollbar &&
				HitTestScrollbarRect(g_CurrentLayout.pagePickerScrollbarTrack, g_CurrentLayout.scale, clientX, clientY)) {
				g_PendingCenterPagePicker = false;
				const IntRect& thumb      = g_CurrentLayout.pagePickerScrollbarThumb;
				if (clientY >= thumb.y && clientY < (thumb.y + thumb.height)) {
					g_ScrollbarDragGrabOffsetY = clientY - thumb.y;
				} else {
					g_ScrollbarDragGrabOffsetY = thumb.height / 2;
					ApplyScrollbarDragAtYLocked(ScrollbarDragMode::PagePicker, clientY);
				}
				g_ScrollbarDragMode.store(static_cast<uint8_t>(ScrollbarDragMode::PagePicker), std::memory_order_release);
			} else {
				const size_t cellCount = std::min(g_CurrentLayout.pageCellCount, kMaxPageGridVisibleCells);
				for (size_t i = 0; i < cellCount; ++i) {
					if (g_CurrentLayout.pageCellRects[i].Contains(clientX, clientY)) {
						g_PendingJumpIsShared   = g_CurrentLayout.pageCellIsShared[i];
						g_PendingJumpPageIndex  = g_CurrentLayout.pageCellIndices[i];
						g_PendingJumpClearFlag  = false;
						g_PendingJumpRetryCount = 0;
						g_HasPendingJump        = true;
						break;
					}
				}
			}
			consumed = true;
		} else if (g_CurrentLayout.hasScrollbar &&
				   HitTestScrollbarRect(g_CurrentLayout.scrollbarTrack, g_CurrentLayout.scale, clientX, clientY)) {
			const IntRect& thumb = g_CurrentLayout.scrollbarThumb;
			if (clientY >= thumb.y && clientY < (thumb.y + thumb.height)) {
				g_ScrollbarDragGrabOffsetY = clientY - thumb.y;
			} else {
				g_ScrollbarDragGrabOffsetY = thumb.height / 2;
				ApplyScrollbarDragAtYLocked(ScrollbarDragMode::SearchDropdown, clientY);
			}
			g_ScrollbarDragMode.store(static_cast<uint8_t>(ScrollbarDragMode::SearchDropdown), std::memory_order_release);
			consumed = true;
		} else {
			const DropdownHitResult hit = HitTestDropdownRowLocked(clientX, clientY);
			if (hit.hit) {
				g_IsFocused           = false;
				g_PlaceholderSelected = false;
				g_PlaceholderCleared  = false;
				g_SelectAllOnFocus    = false;
				g_EditSyncedQuery.clear();
				g_ImeCompositionText.clear();
				g_ImeCursorStart = 0;
				g_ImeCursorEnd   = 0;
				g_ImeCursorPos   = 0;
				g_SelStart       = 0;
				g_SelEnd         = 0;
				g_ImeComposing   = false;
				g_SelectedIndex  = static_cast<size_t>(hit.matchIndex);
				if (g_SelectedIndex < g_Matches.size() && g_Matches[g_SelectedIndex].hasTarget) {
					const auto& entry       = g_Matches[g_SelectedIndex];
					g_PendingJumpIsShared   = entry.target.isShared;
					g_PendingJumpPageIndex  = entry.target.pageIndex;
					g_PendingJumpClearFlag  = false;
					g_PendingJumpRetryCount = 0;
					g_HasPendingJump        = true;
					if (entry.target.isShared) {
						CenterPagePickerOnSharedPageLocked(static_cast<size_t>(entry.target.pageIndex));
					}
				}
				requestUnfocus = true;
				consumed       = true;
			} else if (g_IsFocused) {
				g_IsFocused           = false;
				g_PlaceholderSelected = false;
				g_PlaceholderCleared  = false;
				g_SelectAllOnFocus    = false;
				g_EditSyncedQuery.clear();
				g_ImeCompositionText.clear();
				g_ImeCursorStart = 0;
				g_ImeCursorEnd   = 0;
				g_ImeCursorPos   = 0;
				g_SelStart       = 0;
				g_SelEnd         = 0;
				g_ImeComposing   = false;
				requestUnfocus   = true;
			}
		}

		if (saveConfig) {
			savedCfg = SnapshotPersistedConfigLocked();
		}
	}

	if (saveConfig) {
		SavePersistedConfig(g_PluginCtx, savedCfg);
	}
	if (requestFocus) {
		g_WorkerFocusRequested.store(true, std::memory_order_release);
		const DWORD tid = g_WorkerThreadId.load(std::memory_order_acquire);
		if (tid != 0 && g_WorkerReady.load(std::memory_order_acquire) && !g_ShuttingDown.load(std::memory_order_acquire)) {
			PostThreadMessageW(tid, kWorkerMsgSyncEditFocus, 0, 0);
		}
	} else if (requestUnfocus) {
		g_WorkerFocusRequested.store(false, std::memory_order_release);
		const DWORD tid = g_WorkerThreadId.load(std::memory_order_acquire);
		if (tid != 0 && g_WorkerReady.load(std::memory_order_acquire) && !g_ShuttingDown.load(std::memory_order_acquire)) {
			PostThreadMessageW(tid, kWorkerMsgUnfocusHideEdit, 0, 0);
		}
	}
	return consumed;
}

// 0xBAB0: Handles WM_MOUSEWHEEL in client coordinates (dropdown scroll, page-picker scroll, and Ctrl+Wheel tab/page rotation)
auto HandleMouseWheel(int clientX, int clientY, int wheelDelta) -> bool {
	const uint64_t nowTickMs = GetTickCount64();
	std::lock_guard<std::mutex> lock(g_StateMutex);
	if (!IsSearchBarActiveLocked(nowTickMs) || !g_CurrentLayout.valid || wheelDelta == 0) {
		return false;
	}

	const uint64_t absDelta = wheelDelta < 0 ? static_cast<uint64_t>(-static_cast<int64_t>(wheelDelta))
											 : static_cast<uint64_t>(wheelDelta);
	const size_t   steps    = std::max<size_t>(1, static_cast<size_t>(absDelta / 120));

	// 1. Page-picker grid scrolling when open and hovered
	if (g_PagePickerOpen && g_CurrentLayout.pagePickerBox.Contains(clientX, clientY)) {
		g_PendingCenterPagePicker = false;
		if (g_CurrentLayout.pagePickerTotalRows > g_CurrentLayout.pagePickerVisRows) {
			const size_t maxScrollRow = g_CurrentLayout.pagePickerTotalRows - g_CurrentLayout.pagePickerVisRows;
			if (wheelDelta < 0) {
				g_PagePickerScrollRow = std::min(g_PagePickerScrollRow + steps, maxScrollRow);
			} else {
				g_PagePickerScrollRow = (g_PagePickerScrollRow > steps) ? (g_PagePickerScrollRow - steps) : 0;
			}
		}
		return true;
	}

	// 2. Search results dropdown scrolling when hovered
	if (g_CurrentLayout.hasScrollbar) {
		bool insideDropdown = g_CurrentLayout.dropdownBox.Contains(clientX, clientY) ||
							  g_CurrentLayout.scrollbarTrack.Contains(clientX, clientY);
		if (!insideDropdown) {
			const size_t rows = std::min(g_CurrentLayout.visibleRowCount, kMaxDropdownVisibleRows);
			for (size_t i = 0; i < rows; ++i) {
				if (g_CurrentLayout.dropdownRows[i].Contains(clientX, clientY)) {
					insideDropdown = true;
					break;
				}
			}
		}
		if (insideDropdown) {
			const size_t count   = g_Matches.size();
			size_t       desired = g_SelectedIndex;
			if (wheelDelta < 0) {
				if (g_SelectedIndex < count) {
					desired = std::min(g_SelectedIndex + steps, count - 1);
				}
			} else {
				desired = (g_SelectedIndex > steps) ? (g_SelectedIndex - steps) : 0;
			}
			const size_t cap = g_CurrentLayout.visibleRowCount != 0 ? g_CurrentLayout.visibleRowCount : kMaxDropdownVisibleRows;
			AdjustDropdownSelectionAndScroll(count, cap, desired, g_SelectedIndex, g_ScrollOffset);
			return true;
		}
	}

	// 3. Ctrl + Shift + Mouse Wheel rotates parent tabs; Ctrl + Mouse Wheel rotates shared stash pages
	if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0 && (GetAsyncKeyState(VK_MENU) & 0x8000) == 0) {
		const int32_t dir = (wheelDelta < 0) ? 1 : -1;
		if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) {
			g_PendingParentTabWheelSteps = std::clamp<int32_t>(
				g_PendingParentTabWheelSteps + dir * static_cast<int32_t>(steps),
				-16,
				16);
		} else {
			g_PendingWheelSteps = std::clamp<int32_t>(
				g_PendingWheelSteps + dir * static_cast<int32_t>(steps),
				-16,
				16);
		}
		return true;
	}

	return false;
}

// 0xD0A0: WH_MOUSE_LL callback
LRESULT CALLBACK LowLevelMouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
	const auto* ms = reinterpret_cast<const MSLLHOOKSTRUCT*>(lParam);
	if (nCode < 0 || g_ShuttingDown.load(std::memory_order_acquire) || ms == nullptr) {
		return CallNextHookEx(g_MouseHook, nCode, wParam, lParam);
	}
	if (wParam == WM_MOUSEMOVE) {
		const auto dragMode = static_cast<ScrollbarDragMode>(g_ScrollbarDragMode.load(std::memory_order_acquire));
		if (dragMode == ScrollbarDragMode::None) {
			return CallNextHookEx(g_MouseHook, nCode, wParam, lParam);
		}
		// Note: Do NOT check GetAsyncKeyState(VK_LBUTTON) here because returning 1 from WH_MOUSE_LL on WM_LBUTTONDOWN
		// prevents Windows from setting VK_LBUTTON in the async key state table. Dragging ends on WM_LBUTTONUP.
		const HWND fgHwnd = GetForegroundWindow();
		if (fgHwnd != nullptr) {
			DWORD fgPid = 0;
			if (GetWindowThreadProcessId(fgHwnd, &fgPid) != 0 && fgPid == GetCurrentProcessId()) {
				POINT clientPt = ms->pt;
				if (ScreenToClient(fgHwnd, &clientPt)) {
					const uint64_t nowTickMs = GetTickCount64();
					std::lock_guard<std::mutex> lock(g_StateMutex);
					if (IsSearchBarActiveLocked(nowTickMs)) {
						ApplyScrollbarDragAtYLocked(dragMode, clientPt.y);
					} else {
						g_ScrollbarDragMode.store(static_cast<uint8_t>(ScrollbarDragMode::None), std::memory_order_release);
					}
				}
			} else {
				g_ScrollbarDragMode.store(static_cast<uint8_t>(ScrollbarDragMode::None), std::memory_order_release);
			}
		} else {
			g_ScrollbarDragMode.store(static_cast<uint8_t>(ScrollbarDragMode::None), std::memory_order_release);
		}
		return CallNextHookEx(g_MouseHook, nCode, wParam, lParam);
	}
	if (wParam != WM_LBUTTONDOWN &&
		wParam != WM_LBUTTONUP &&
		wParam != WM_RBUTTONDOWN &&
		wParam != WM_RBUTTONUP &&
		wParam != WM_MOUSEWHEEL) {
		return CallNextHookEx(g_MouseHook, nCode, wParam, lParam);
	}
	const bool wasDraggingScrollbar =
		(wParam == WM_LBUTTONUP) &&
		(g_ScrollbarDragMode.exchange(static_cast<uint8_t>(ScrollbarDragMode::None), std::memory_order_acq_rel) !=
		 static_cast<uint8_t>(ScrollbarDragMode::None));

	const HWND fgHwnd = GetForegroundWindow();
	if (fgHwnd == nullptr) {
		return CallNextHookEx(g_MouseHook, nCode, wParam, lParam);
	}
	DWORD fgPid = 0;
	if (GetWindowThreadProcessId(fgHwnd, &fgPid) == 0 || fgPid != GetCurrentProcessId()) {
		return CallNextHookEx(g_MouseHook, nCode, wParam, lParam);
	}
	POINT clientPt = ms->pt;
	RECT  clientRc{};
	if (!ScreenToClient(fgHwnd, &clientPt) || !GetClientRect(fgHwnd, &clientRc)) {
		return CallNextHookEx(g_MouseHook, nCode, wParam, lParam);
	}
	if (clientPt.x < 0 || clientPt.y < 0 || clientPt.x >= clientRc.right || clientPt.y >= clientRc.bottom) {
		if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN) {
			bool              hadFocus   = false;
			bool              saveConfig = false;
			StashSearchConfig savedCfg{};
			{
				std::lock_guard<std::mutex> lock(g_StateMutex);
				if (g_IsRenamingTab && CommitActiveTabRenameLocked()) {
					saveConfig = true;
					savedCfg   = SnapshotPersistedConfigLocked();
				}
				if (g_IsFocused) {
					g_IsFocused           = false;
					g_PlaceholderSelected = false;
					g_PlaceholderCleared  = false;
					g_SelectAllOnFocus    = false;
					g_EditSyncedQuery.clear();
					g_ImeCompositionText.clear();
					g_ImeCursorStart = 0;
					g_ImeCursorEnd   = 0;
					g_ImeCursorPos   = 0;
					g_SelStart       = 0;
					g_SelEnd         = 0;
					g_ImeComposing   = false;
					hadFocus         = true;
				}
			}
			if (saveConfig) {
				SavePersistedConfig(g_PluginCtx, savedCfg);
			}
			if (hadFocus) {
				g_WorkerFocusRequested.store(false, std::memory_order_release);
				const DWORD tid = g_WorkerThreadId.load(std::memory_order_acquire);
				if (tid != 0 && g_WorkerReady.load(std::memory_order_acquire) && !g_ShuttingDown.load(std::memory_order_acquire)) {
					PostThreadMessageW(tid, kWorkerMsgUnfocusHideEdit, 0, 0);
				}
			}
		}
		return CallNextHookEx(g_MouseHook, nCode, wParam, lParam);
	}

	bool consumed = false;
	if (wParam == WM_LBUTTONDOWN) {
		consumed = HandleMouseClick(clientPt.x, clientPt.y);
	} else if (wParam == WM_LBUTTONUP) {
		const uint64_t nowTickMs = GetTickCount64();
		std::lock_guard<std::mutex> lock(g_StateMutex);
		if (wasDraggingScrollbar) {
			consumed = true;
		} else if (IsSearchBarActiveLocked(nowTickMs)) {
			if (g_CurrentLayout.searchBox.Contains(clientPt.x, clientPt.y) ||
				g_CurrentLayout.pageButtonRect.Contains(clientPt.x, clientPt.y) ||
				(g_PagePickerOpen &&
				 (g_CurrentLayout.pagePickerBox.Contains(clientPt.x, clientPt.y) ||
				  (g_CurrentLayout.pagePickerHasScrollbar &&
				   HitTestScrollbarRect(g_CurrentLayout.pagePickerScrollbarTrack, g_CurrentLayout.scale, clientPt.x, clientPt.y)))) ||
				HitTestDropdownRowLocked(clientPt.x, clientPt.y).hit ||
				HitTestScrollbarRect(g_CurrentLayout.scrollbarTrack, g_CurrentLayout.scale, clientPt.x, clientPt.y) ||
				HitTestScrollbarRect(g_CurrentLayout.scrollbarThumb, g_CurrentLayout.scale, clientPt.x, clientPt.y)) {
				consumed = true;
			}
		}
	} else if (wParam == WM_RBUTTONDOWN) {
		const uint64_t    nowTickMs    = GetTickCount64();
		bool              requestFocus = false;
		bool              saveConfig   = false;
		StashSearchConfig savedCfg{};
		{
			std::lock_guard<std::mutex> lock(g_StateMutex);
			if (IsSearchBarActiveLocked(nowTickMs)) {
				if (g_CurrentLayout.searchBox.Contains(clientPt.x, clientPt.y) &&
					!g_CurrentLayout.pageButtonRect.Contains(clientPt.x, clientPt.y)) {
					if (g_IsRenamingTab && CommitActiveTabRenameLocked()) {
						saveConfig = true;
						savedCfg   = SnapshotPersistedConfigLocked();
					}
					g_IsFocused           = true;
					g_IsRenamingTab       = false;
					g_PlaceholderSelected = false;
					g_PlaceholderCleared  = true;
					g_SelectAllOnFocus    = false;
					g_QueryText.clear();
					g_ImeCompositionText.clear();
					g_ImeCursorStart = 0;
					g_ImeCursorEnd   = 0;
					g_ImeCursorPos   = 0;
					g_SelStart       = 0;
					g_SelEnd         = 0;
					g_ImeComposing   = false;
					MarkQueryChangedLocked();
					requestFocus = true;
					consumed     = true;
				} else if (g_PagePickerOpen &&
						   (g_CurrentLayout.pagePickerBox.Contains(clientPt.x, clientPt.y) ||
							(g_CurrentLayout.pagePickerHasScrollbar &&
							 HitTestScrollbarRect(g_CurrentLayout.pagePickerScrollbarTrack, g_CurrentLayout.scale, clientPt.x, clientPt.y)))) {
					const size_t cellCount = std::min(g_CurrentLayout.pageCellCount, kMaxPageGridVisibleCells);
					for (size_t i = 0; i < cellCount; ++i) {
						if (g_CurrentLayout.pageCellRects[i].Contains(clientPt.x, clientPt.y)) {
							const bool     isShared = g_CurrentLayout.pageCellIsShared[i];
							const uint64_t pageIdx  = g_CurrentLayout.pageCellIndices[i];
							if (isShared || pageIdx == 0) {
								if (g_IsRenamingTab &&
									(g_RenamingTabIsShared != isShared || g_RenamingTabPageIndex != pageIdx)) {
									if (CommitActiveTabRenameLocked()) {
										saveConfig = true;
										savedCfg   = SnapshotPersistedConfigLocked();
									}
								}
								g_IsFocused            = true;
								g_IsRenamingTab        = true;
								g_RenamingTabIsShared  = isShared;
								g_RenamingTabPageIndex = pageIdx;
								g_TabRenameText        = isShared
															 ? GetCustomSharedTabNameLocked(pageIdx)
															 : g_CustomPersonalTabName;
								g_EditSyncedQuery.clear();
								g_PlaceholderSelected = false;
								g_PlaceholderCleared  = false;
								g_SelectAllOnFocus    = !g_TabRenameText.empty();
								g_SelStart            = 0;
								g_SelEnd              = static_cast<uint32_t>(g_TabRenameText.size());
								g_ImeCompositionText.clear();
								g_ImeCursorStart = 0;
								g_ImeCursorEnd   = 0;
								g_ImeCursorPos   = 0;
								g_ImeComposing   = false;
								requestFocus     = true;
							}
							break;
						}
					}
					consumed = true;
				}
			}
		}
		if (saveConfig) {
			SavePersistedConfig(g_PluginCtx, savedCfg);
		}
		if (requestFocus) {
			g_WorkerFocusRequested.store(true, std::memory_order_release);
			const DWORD tid = g_WorkerThreadId.load(std::memory_order_acquire);
			if (tid != 0 && g_WorkerReady.load(std::memory_order_acquire) && !g_ShuttingDown.load(std::memory_order_acquire)) {
				PostThreadMessageW(tid, kWorkerMsgSyncEditFocus, 0, 0);
			}
		}
	} else if (wParam == WM_RBUTTONUP) {
		const uint64_t nowTickMs = GetTickCount64();
		std::lock_guard<std::mutex> lock(g_StateMutex);
		if (IsSearchBarActiveLocked(nowTickMs)) {
			if ((g_CurrentLayout.searchBox.Contains(clientPt.x, clientPt.y) &&
				 !g_CurrentLayout.pageButtonRect.Contains(clientPt.x, clientPt.y)) ||
				(g_PagePickerOpen &&
				 (g_CurrentLayout.pagePickerBox.Contains(clientPt.x, clientPt.y) ||
				  (g_CurrentLayout.pagePickerHasScrollbar &&
				   HitTestScrollbarRect(g_CurrentLayout.pagePickerScrollbarTrack, g_CurrentLayout.scale, clientPt.x, clientPt.y))))) {
				consumed = true;
			}
		}
	} else if (wParam == WM_MOUSEWHEEL) {
		const int wheelDelta = static_cast<short>(HIWORD(ms->mouseData));
		consumed             = HandleMouseWheel(clientPt.x, clientPt.y, wheelDelta);
	}

	if (consumed) {
		return 1;
	}
	return CallNextHookEx(g_MouseHook, nCode, wParam, lParam);
}

auto IsModifierOrSystemVk(DWORD vk) noexcept -> bool {
	switch (vk) {
	case VK_SHIFT:
	case VK_LSHIFT:
	case VK_RSHIFT:
	case VK_CONTROL:
	case VK_LCONTROL:
	case VK_RCONTROL:
	case VK_MENU:
	case VK_LMENU:
	case VK_RMENU:
	case VK_LWIN:
	case VK_RWIN:
	case VK_CAPITAL:
	case VK_NUMLOCK:
	case VK_SCROLL:
		return true;
	default:
		return false;
	}
}

// 0xC320: WH_KEYBOARD_LL callback — forwards keys to the message-only EDIT control while the search box is focused
LRESULT CALLBACK LowLevelKeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
	const auto* kb = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
	if (nCode >= 0 && !g_ShuttingDown.load(std::memory_order_acquire) && kb != nullptr &&
		(wParam == WM_KEYDOWN || wParam == WM_KEYUP)) {
		const HWND fgHwnd = GetForegroundWindow();
		if (fgHwnd == nullptr) {
			return CallNextHookEx(g_KeyboardHook, nCode, wParam, lParam);
		}
		DWORD fgPid = 0;
		if (GetWindowThreadProcessId(fgHwnd, &fgPid) == 0 || fgPid != GetCurrentProcessId()) {
			return CallNextHookEx(g_KeyboardHook, nCode, wParam, lParam);
		}
		if (kb->vkCode > 0xFF || IsModifierOrSystemVk(kb->vkCode)) {
			return CallNextHookEx(g_KeyboardHook, nCode, wParam, lParam);
		}
		// Never swallow Win+Key shortcuts or pure Alt+Key shortcuts (e.g. Alt+Tab, Alt+F4, Alt+Enter)
		const bool winHeld  = (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 || (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;
		const bool ctrlHeld = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
		const bool altHeld  = (kb->flags & LLKHF_ALTDOWN) != 0 || (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
		if (winHeld || (altHeld && !ctrlHeld)) {
			return CallNextHookEx(g_KeyboardHook, nCode, wParam, lParam);
		}

		if (wParam == WM_KEYUP) {
			const uint8_t wasForwarded   = g_ForwardedKeyDown[kb->vkCode];
			g_ForwardedKeyDown[kb->vkCode] = 0;
			if (wasForwarded == 0) {
				return CallNextHookEx(g_KeyboardHook, nCode, WM_KEYUP, lParam);
			}
		} else if (!g_WorkerFocusRequested.load(std::memory_order_acquire) || !IsSearchBarFocusedAndActive()) {
			return CallNextHookEx(g_KeyboardHook, nCode, wParam, lParam);
		}

		const HWND editHwnd = g_EditHwnd;
		if (editHwnd == nullptr || !IsWindow(editHwnd)) {
			g_ForwardedKeyDown[kb->vkCode] = 0;
			return CallNextHookEx(g_KeyboardHook, nCode, wParam, lParam);
		}

		LPARAM msgLParam = static_cast<LPARAM>(((kb->flags & LLKHF_EXTENDED) ? 0x01000000ULL : 0ULL) + 1ULL |
											   (static_cast<uint64_t>(static_cast<uint8_t>(kb->scanCode)) << 16));
		if ((kb->flags & LLKHF_ALTDOWN) != 0) {
			msgLParam |= 0x20000000LL;
		}
		if (wParam == WM_KEYUP) {
			msgLParam |= static_cast<LPARAM>(0xC0000000ULL);
		}

		WPARAM msgWParam = kb->vkCode;
		UINT   msgId     = static_cast<UINT>(wParam);
		if (wParam == WM_KEYDOWN && ctrlHeld && !altHeld) {
			if (kb->vkCode == 'A') {
				msgId     = WM_CHAR;
				msgWParam = 0x01; // Ctrl+A (Select All)
			} else if (kb->vkCode == 'C') {
				msgId     = WM_CHAR;
				msgWParam = 0x03; // Ctrl+C (Copy)
			} else if (kb->vkCode == 'V') {
				msgId     = WM_CHAR;
				msgWParam = 0x16; // Ctrl+V (Paste)
			} else if (kb->vkCode == 'X') {
				msgId     = WM_CHAR;
				msgWParam = 0x18; // Ctrl+X (Cut)
			} else if (kb->vkCode == 'Z') {
				msgId     = WM_CHAR;
				msgWParam = 0x1A; // Ctrl+Z (Undo)
			}
		}

		if (PostMessageW(editHwnd, msgId, msgWParam, msgLParam)) {
			if (wParam != WM_KEYUP) {
				g_ForwardedKeyDown[kb->vkCode] = 1;
			}
			return 1;
		}
		g_ForwardedKeyDown[kb->vkCode] = 0;
	}
	return CallNextHookEx(g_KeyboardHook, nCode, wParam, lParam);
}

// 0xE620: Expires search state when BankPanel stops rendering for >= 301ms
auto CheckAndExpireInactivePanel() -> bool {
	const uint64_t nowTickMs = GetTickCount64();
	{
		std::lock_guard<std::mutex> lock(g_StateMutex);
		if (g_LastRenderTickMs == 0 || (nowTickMs >= g_LastRenderTickMs && (nowTickMs - g_LastRenderTickMs) < kLayoutActiveTimeoutMs)) {
			return false;
		}
		g_QueryText.clear();
		MarkQueryChangedLocked();
		g_IsFocused = false;
		g_EditSyncedQuery.clear();
		g_ImeCompositionText.clear();
		g_ImeCursorStart      = 0;
		g_ImeCursorEnd        = 0;
		g_ImeCursorPos        = 0;
		g_SelStart            = 0;
		g_SelEnd              = 0;
		g_ImeComposing        = false;
		g_WasPanelActive      = false;
		g_PlaceholderSelected = false;
		g_PlaceholderCleared  = false;
		g_SelectAllOnFocus    = false;
		g_PagePickerOpen          = g_PersistedPagePickerOpen;
		g_PagePickerScrollRow     = 0;
		g_PendingCenterPagePicker = false;
		g_PendingCenterPageIndex  = 0;
		g_PendingWheelSteps       = 0;
		g_ScrollbarDragMode.store(static_cast<uint8_t>(ScrollbarDragMode::None), std::memory_order_release);
		g_ScrollbarDragGrabOffsetY = 0;
		g_GridTooltipCache.clear();
		g_GridTooltipCacheTickMs = 0;
		g_CurrentLayout       = SearchBarLayout{};
		g_LastRenderTickMs    = 0;
	}
	g_WorkerFocusRequested.store(false, std::memory_order_release);
	const DWORD tid = g_WorkerThreadId.load(std::memory_order_acquire);
	if (tid != 0 && g_WorkerReady.load(std::memory_order_acquire) && !g_ShuttingDown.load(std::memory_order_acquire)) {
		PostThreadMessageW(tid, kWorkerMsgUnfocusHideEdit, 0, 0);
	}
	return true;
}

// 0xBEC0: Dedicated Win32 message & hook worker thread
DWORD WINAPI InputWorkerThreadMain(LPVOID) {
	g_WorkerThreadId.store(GetCurrentThreadId(), std::memory_order_release);
	MSG initMsg{};
	PeekMessageW(&initMsg, nullptr, 0, 0, PM_NOREMOVE);

	const UINT_PTR timerId = SetTimer(nullptr, kWorkerTimerId, kWorkerTimerIntervalMs, nullptr);
	HMODULE        selfMod = nullptr;
	bool           ok      = false;
	if (timerId != 0) {
		if (GetModuleHandleExW(
				GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
				reinterpret_cast<LPCWSTR>(&LowLevelMouseHookProc),
				&selfMod)) {
			g_MouseHook    = SetWindowsHookExW(WH_MOUSE_LL, &LowLevelMouseHookProc, selfMod, 0);
			g_KeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, &LowLevelKeyboardHookProc, selfMod, 0);
		}
		if (g_MouseHook != nullptr && g_KeyboardHook != nullptr) {
			ok = true;
		}
	}

	g_WorkerReady.store(ok, std::memory_order_release);
	if (g_WorkerReadyEvent != nullptr) {
		SetEvent(g_WorkerReadyEvent);
	}

	if (!ok) {
		if (timerId != 0) {
			KillTimer(nullptr, kWorkerTimerId);
		}
		if (g_KeyboardHook != nullptr) {
			UnhookWindowsHookEx(g_KeyboardHook);
			g_KeyboardHook = nullptr;
		}
		std::memset(g_ForwardedKeyDown, 0, sizeof(g_ForwardedKeyDown));
		if (g_MouseHook != nullptr) {
			UnhookWindowsHookEx(g_MouseHook);
			g_MouseHook = nullptr;
		}
	} else {
		MSG  msg{};
		bool quitHandled = false;
		while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
			if (msg.message == kWorkerMsgSyncEditFocus) {
				SyncHiddenEditWindow();
			} else if (msg.message == kWorkerMsgUnfocusHideEdit) {
				UnfocusAndHideEditControl();
			} else if (msg.message == kWorkerMsgDestroyEdit) {
				DestroyEditControl();
			} else if (msg.message == kWorkerMsgShutdownThread) {
				UnfocusAndHideEditControl();
				DestroyEditControl();
				PostQuitMessage(0);
				quitHandled = true;
			} else if (msg.message == WM_TIMER && msg.wParam == kWorkerTimerId) {
				const HWND fgHwnd = GetForegroundWindow();
				DWORD      fgPid  = 0;
				const bool isGameForeground = (fgHwnd != nullptr &&
											   GetWindowThreadProcessId(fgHwnd, &fgPid) != 0 &&
											   fgPid == GetCurrentProcessId());
				if (!isGameForeground) {
					std::lock_guard<std::mutex> lock(g_StateMutex);
					if (g_IsFocused) {
						g_IsFocused           = false;
						g_PlaceholderSelected = false;
						g_PlaceholderCleared  = false;
						g_SelectAllOnFocus    = false;
						g_EditSyncedQuery.clear();
						g_ImeCompositionText.clear();
						g_ImeCursorStart = 0;
						g_ImeCursorEnd   = 0;
						g_ImeCursorPos   = 0;
						g_SelStart       = 0;
						g_SelEnd         = 0;
						g_ImeComposing   = false;
					}
				}
				if (!CheckAndExpireInactivePanel() &&
					g_WorkerFocusRequested.load(std::memory_order_acquire) &&
					!IsSearchBarFocusedAndActive()) {
					UnfocusAndHideEditControl();
				}
			} else {
				if (msg.message == WM_KEYDOWN || msg.message == WM_KEYUP) {
					SyncWorkerThreadKeyboardState();
				}
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
		}
		if (!quitHandled) {
			UnfocusAndHideEditControl();
			DestroyEditControl();
			PostQuitMessage(0);
		}
		if (timerId != 0) {
			KillTimer(nullptr, kWorkerTimerId);
		}
		if (g_KeyboardHook != nullptr) {
			UnhookWindowsHookEx(g_KeyboardHook);
			g_KeyboardHook = nullptr;
		}
		std::memset(g_ForwardedKeyDown, 0, sizeof(g_ForwardedKeyDown));
		if (g_MouseHook != nullptr) {
			UnhookWindowsHookEx(g_MouseHook);
			g_MouseHook = nullptr;
		}
		g_WorkerFocusRequested.store(false, std::memory_order_release);
	}

	g_WorkerThreadId.store(0, std::memory_order_release);
	g_WorkerReady.store(false, std::memory_order_release);
	return 0;
}

// 0xF680: Stops and joins the input worker thread
void StopInputWorkerThread() noexcept {
	const DWORD tid = g_WorkerThreadId.load(std::memory_order_acquire);
	if (tid != 0) {
		PostThreadMessageW(tid, kWorkerMsgShutdownThread, 0, 0);
	}
	if (g_WorkerThreadHandle != nullptr) {
		WaitForSingleObject(g_WorkerThreadHandle, 5000);
		CloseHandle(g_WorkerThreadHandle);
		g_WorkerThreadHandle = nullptr;
	}
	if (g_WorkerReadyEvent != nullptr) {
		CloseHandle(g_WorkerReadyEvent);
		g_WorkerReadyEvent = nullptr;
	}
	g_WorkerFocusRequested.store(false, std::memory_order_release);
	g_WorkerReady.store(false, std::memory_order_release);
	g_WorkerThreadId.store(0, std::memory_order_release);
}

// 0x37E0: Inspects one stash item, deduplicating identical labels on the same tab/page
void ProcessInventoryItem(
	uintptr_t                itemUnit,
	std::wstring_view        normalizedQuery,
	std::wstring_view        shortTabSuffix,
	std::wstring_view        fullTabSuffix,
	const StashPageTarget&   target,
	std::vector<MatchEntry>& outMatches) {
	if (itemUnit == 0) {
		return;
	}
	std::wstring fullTooltip;
	std::wstring shortName;
	std::wstring fullNameLine;
	BuildItemFullTooltipText(itemUnit, fullTooltip, shortName, &fullNameLine);
	if (fullTooltip.empty() || !EvaluateSearchQuery(normalizedQuery, fullTooltip)) {
		return;
	}

	const std::wstring_view hoverItemName = fullNameLine.empty() ? std::wstring_view(shortName) : std::wstring_view(fullNameLine);

	std::wstring baseLabel;
	baseLabel.reserve(shortName.size() + shortTabSuffix.size());
	baseLabel.append(shortName);
	baseLabel.append(shortTabSuffix);

	std::wstring fullHoverBase;
	fullHoverBase.reserve(hoverItemName.size() + fullTabSuffix.size());
	fullHoverBase.append(hoverItemName);
	fullHoverBase.append(fullTabSuffix);

	for (auto& existing : outMatches) {
		if (existing.hasTarget &&
			existing.target.isShared == target.isShared &&
			existing.target.pageIndex == target.pageIndex &&
			existing.baseLabel == baseLabel) {
			++existing.count;
			const std::wstring countStr = L" \x00D7 " + std::to_wstring(existing.count);
			existing.displayText        = shortName + countStr + std::wstring(shortTabSuffix);
			existing.hoverText          = std::wstring(hoverItemName) + countStr + std::wstring(fullTabSuffix);
			return;
		}
	}

	if (outMatches.size() < kMaxMatches) {
		MatchEntry entry{};
		entry.displayText    = baseLabel;
		entry.count          = 1;
		entry.target         = target;
		entry.hasTarget      = true;
		entry.baseLabel      = std::move(baseLabel);
		entry.hoverText      = std::move(fullHoverBase);
		entry.shortTabSuffix = std::wstring(shortTabSuffix);
		outMatches.push_back(std::move(entry));
	}
}

// 0x9A90: Walks an Inventory* item linked list (up to 2048 items), filtering for stash page (location == 4)
void ScanInventoryItems(
	uintptr_t                inventoryPtr,
	std::wstring_view        normalizedQuery,
	std::wstring_view        shortTabSuffix,
	std::wstring_view        fullTabSuffix,
	const StashPageTarget&   target,
	std::vector<MatchEntry>& outMatches) {
	if (inventoryPtr == 0) {
		return;
	}
	uintptr_t item = 0;
	if (!SafeCall_PtrFromPtr(g_Native.fnInventoryGetFirstItem, inventoryPtr, &item)) {
		return;
	}
	std::unordered_set<uintptr_t> visitedItems;
	size_t                        steps = 0;
	while (item != 0 && steps < 2048) {
		if (!visitedItems.insert(item).second) {
			break;
		}
		uint8_t locPage = 0;
		if (!SafeCall_U8FromPtr(g_Native.fnItemGetLocationPage, item, &locPage)) {
			break;
		}
		if (locPage == kItemLocationStashPage) {
			ProcessInventoryItem(item, normalizedQuery, shortTabSuffix, fullTabSuffix, target, outMatches);
			if (outMatches.size() >= kMaxMatches) {
				break;
			}
		}
		uintptr_t nextItem = 0;
		if (!SafeCall_PtrFromPtr(g_Native.fnItemGetNextInventoryItem, item, &nextItem)) {
			break;
		}
		item = nextItem;
		++steps;
	}
}

// 0x3E90: Builds the full vector of MatchEntry results for the current query
auto BuildSearchResults(std::wstring_view rawQuery) -> std::vector<MatchEntry> {
	std::vector<MatchEntry> results;
	const std::wstring      normalizedQuery = NormalizeWideTooltipLine(rawQuery);
	if (normalizedQuery.empty()) {
		return results;
	}

	const SlashPageCommand slashCmd = ParseSlashPageCommand(rawQuery);
	if (slashCmd.state != SlashCommandState::None) {
		MatchEntry entry{};
		entry.displayText = FormatSlashPagePrompt(slashCmd);
		entry.count       = 1;
		entry.baseLabel   = entry.displayText;
		entry.hoverText   = entry.displayText;
		if (slashCmd.state == SlashCommandState::ValidPage && slashCmd.pageNumber != 0) {
			entry.target.isShared  = true;
			entry.target.pageIndex = slashCmd.pageNumber - 1;
			entry.hasTarget        = true;
		}
		results.push_back(std::move(entry));
		return results;
	}

	std::wstring personalLabel = L"Personal";
	LookupLocalizedString("personal", 8, personalLabel);
	std::wstring sharedLabel = L"Shared";
	LookupLocalizedString("shared", 6, sharedLabel);

	std::wstring                                   customPersonalName;
	std::vector<std::pair<uint64_t, std::wstring>> customSharedNames;
	{
		std::lock_guard<std::mutex> lock(g_StateMutex);
		customPersonalName = g_CustomPersonalTabName;
		customSharedNames  = g_CustomSharedTabNames;
	}

	uint32_t  localSlot  = 0;
	uintptr_t playerUnit = 0;
	uintptr_t playerInv  = 0;
	if (!SafeCall_GetU32NoArg(g_Native.fnGetLocalPlayerSlot, &localSlot) ||
		!SafeCall_PtrFromU32(g_Native.fnGetLocalPlayerUnit, localSlot, &playerUnit) ||
		playerUnit == 0 ||
		!SafeCall_PtrFromPtr(g_Native.fnGetUnitInventory, playerUnit, &playerInv) ||
		playerInv == 0) {
		MatchEntry unavailable{};
		unavailable.displayText = L"Stash unavailable";
		unavailable.count       = 1;
		unavailable.baseLabel   = unavailable.displayText;
		results.push_back(std::move(unavailable));
		return results;
	}

	// Scan Personal Stash ("<ShortName> - P" in row, "<FullName> - Personal" in hover tooltip)
	const std::wstring personalShortSuffix =
		customPersonalName.empty() ? L" - P" : (L" - P (" + customPersonalName + L")");
	const std::wstring personalFullSuffix =
		customPersonalName.empty() ? (L" - " + personalLabel) : (L" - " + personalLabel + L" (" + customPersonalName + L")");
	ScanInventoryItems(playerInv, normalizedQuery, personalShortSuffix, personalFullSuffix, StashPageTarget{ false, 0 }, results);

	// Walk Shared Stash linked list at [playerInv + 0x68]
	uint64_t sharedNode = 0;
	if (SafeReadU64(reinterpret_cast<const uint64_t*>(playerInv + 0x68), &sharedNode) && sharedNode != 0) {
		std::unordered_set<uintptr_t> visitedNodes(512);
		uint64_t                      sharedIndex = 0;
		size_t                        nodeSteps   = 0;
		while (sharedNode != 0 && nodeSteps < 256 && results.size() < kMaxMatches) {
			if (!visitedNodes.insert(static_cast<uintptr_t>(sharedNode)).second) {
				break;
			}
			uint8_t flag8 = 0;
			uint8_t flagA = 0;
			if (!SafeReadU8(reinterpret_cast<const uint8_t*>(sharedNode + 0x08), &flag8) ||
				!SafeReadU8(reinterpret_cast<const uint8_t*>(sharedNode + 0x0A), &flagA)) {
				break;
			}
			if (flag8 == 0 && flagA != 1) {
				const uint64_t tabIdx0 = sharedIndex;
				++sharedIndex;
				uint32_t sharedUnitId = 0;
				if (!SafeReadU32(reinterpret_cast<const uint32_t*>(sharedNode), &sharedUnitId)) {
					break;
				}
				if (sharedUnitId != 0) {
					uintptr_t sharedUnit = 0;
					uintptr_t sharedInv  = 0;
					if (SafeCall_PtrFromU32U32(g_Native.fnFindClientUnitByIdAndType, sharedUnitId, 0, &sharedUnit) &&
						sharedUnit != 0 &&
						SafeCall_PtrFromPtr(g_Native.fnGetUnitInventory, sharedUnit, &sharedInv) &&
						sharedInv != 0) {
						std::wstring customTabName;
						for (const auto& [idx, name] : customSharedNames) {
							if (idx == tabIdx0) {
								customTabName = name;
								break;
							}
						}
						const std::wstring sharedShortSuffix =
							customTabName.empty()
								? (L" - Sh." + std::to_wstring(sharedIndex))
								: (L" - Sh." + std::to_wstring(sharedIndex) + L" (" + customTabName + L")");
						const std::wstring sharedFullSuffix =
							customTabName.empty()
								? (L" - " + sharedLabel + L" " + std::to_wstring(sharedIndex))
								: (L" - " + sharedLabel + L" " + std::to_wstring(sharedIndex) + L" (" + customTabName + L")");
						ScanInventoryItems(
							sharedInv,
							normalizedQuery,
							sharedShortSuffix,
							sharedFullSuffix,
							StashPageTarget{ true, tabIdx0 },
							results);
					}
				}
			}
			uint64_t nextNode = 0;
			if (sharedNode > UINT64_MAX - 0x10 ||
				!SafeReadU64(reinterpret_cast<const uint64_t*>(sharedNode + 0x10), &nextNode) ||
				nextNode == 0 ||
				nextNode == sharedNode) {
				break;
			}
			sharedNode = nextNode;
			++nodeSteps;
		}
	}

	if (results.empty()) {
		MatchEntry noMatch{};
		noMatch.displayText = L"No matches";
		noMatch.count       = 1;
		noMatch.baseLabel   = noMatch.displayText;
		results.push_back(std::move(noMatch));
	}
	return results;
}

// 0xDD50 & 0xE460: Refreshes search results when dirty, generation changed, or 3s timer elapsed
void CommitSearchResults(std::vector<MatchEntry>&& newMatches, uint64_t generation, uint64_t nowTickMs) {
	std::lock_guard<std::mutex> lock(g_StateMutex);
	if (g_QueryGeneration.load(std::memory_order_acquire) != generation) {
		return;
	}
	const uint64_t prevGen = g_BuiltGeneration;
	g_Matches              = std::move(newMatches);
	if (generation == prevGen) {
		const size_t cap = g_CurrentLayout.visibleRowCount != 0 ? g_CurrentLayout.visibleRowCount : kMaxDropdownVisibleRows;
		AdjustDropdownSelectionAndScroll(g_Matches.size(), cap, g_SelectedIndex, g_SelectedIndex, g_ScrollOffset);
	} else {
		g_SelectedIndex = 0;
		bool found      = false;
		for (size_t i = 0; i < g_Matches.size(); ++i) {
			if (g_Matches[i].hasTarget) {
				g_SelectedIndex = i;
				found           = true;
				break;
			}
		}
		if (!found) {
			g_SelectedIndex = 0;
		}
		g_ScrollOffset = 0;
	}
	g_MatchesDirty.store(false, std::memory_order_release);
	g_BuiltGeneration = generation;
	g_LastBuildTickMs = nowTickMs;
}

void RefreshMatchesIfNeeded(uint64_t nowTickMs) {
	const uint64_t gen = g_QueryGeneration.load(std::memory_order_acquire);
	std::wstring   querySnapshot;
	{
		std::lock_guard<std::mutex> lock(g_StateMutex);
		querySnapshot = g_QueryText;
	}
	const bool periodicDue = !querySnapshot.empty() &&
							 (g_LastBuildTickMs == 0 || (nowTickMs - g_LastBuildTickMs) >= kPeriodicRefreshMs);
	if (g_MatchesDirty.load(std::memory_order_acquire) || gen != g_BuiltGeneration || periodicDue) {
		std::vector<MatchEntry> built = BuildSearchResults(querySnapshot);
		CommitSearchResults(std::move(built), gen, nowTickMs);
	}
}

// 0x7980: Switches BankPanel to a specific 0-based shared stash subpage
auto SwitchSharedStashSubpage(uintptr_t bankPanel, uint64_t pageIndex, bool* outDidSwitch = nullptr) noexcept -> bool {
	if (outDidSwitch != nullptr) {
		*outDidSwitch = false;
	}
	if (bankPanel == 0 ||
		g_Native.fnBankPanelGetSharedCount == 0 ||
		g_Native.fnBankPanelIsPrevSeason == 0 ||
		g_Native.fnBankPanelGetSelectedTab == 0 ||
		g_Native.fnBankPanelGetSelectedRecord == 0 ||
		g_Native.fnBankPanelUpdateActiveView == 0 ||
		g_Native.ptrActiveSharedStashUnitId == nullptr) {
		return false;
	}

	uint64_t pageCount = 0;
	if (!SafeCall_U64FromPtr(g_Native.fnBankPanelGetSharedCount, bankPanel, &pageCount) || pageIndex >= pageCount) {
		return false;
	}
	uint8_t isPrevSeason = 0;
	if (!SafeCall_BoolFromPtr(g_Native.fnBankPanelIsPrevSeason, bankPanel, &isPrevSeason)) {
		return false;
	}

	auto* const subpageSlot = reinterpret_cast<uint64_t*>(
		bankPanel + 0x168 + (isPrevSeason != 0 ? 8ULL : 0ULL));
	uint64_t prevSubpage = 0;
	uint32_t prevUnitId  = 0;
	uint8_t  selTab      = 0;
	if (!SafeReadU64(subpageSlot, &prevSubpage) ||
		prevSubpage >= pageCount ||
		!SafeReadU32(g_Native.ptrActiveSharedStashUnitId, &prevUnitId) ||
		!SafeCall_BoolFromPtr(g_Native.fnBankPanelGetSelectedTab, bankPanel, &selTab)) {
		return false;
	}
	if (selTab == 1 && prevSubpage == pageIndex) {
		return true;
	}
	if (!SafeWriteU64(subpageSlot, pageIndex)) {
		return false;
	}

	uint32_t  localSlot  = 0;
	uintptr_t playerUnit = 0;
	if (SafeCall_GetU32NoArg(g_Native.fnGetLocalPlayerSlot, &localSlot) &&
		SafeCall_PtrFromU32(g_Native.fnGetLocalPlayerUnit, localSlot, &playerUnit) &&
		playerUnit != 0) {
		uint32_t newSharedUid = 0xFFFFFFFEu;
		bool     recordOk     = true;
		if (selTab == 1) {
			uint64_t recordPtr = 0;
			recordOk           = SafeCall_U64FromPtr(g_Native.fnBankPanelGetSelectedRecord, bankPanel, &recordPtr);
			if (recordOk && recordPtr != 0) {
				recordOk = SafeReadU32(reinterpret_cast<const uint32_t*>(recordPtr), &newSharedUid);
			}
		}
		if (recordOk &&
			SafeWriteU32(g_Native.ptrActiveSharedStashUnitId, newSharedUid) &&
			SafeCall_VoidFromPtrPtr(g_Native.fnBankPanelUpdateActiveView, bankPanel, playerUnit)) {
			if (outDidSwitch != nullptr) {
				*outDidSwitch = true;
			}
			return true;
		}
	}

	SafeWriteU64(subpageSlot, prevSubpage);
	SafeWriteU32(g_Native.ptrActiveSharedStashUnitId, prevUnitId);
	return false;
}

// Safely selects an enabled parent tab in "BankTabs", validating bounds (+0x558) and enabled state (+0x1788)
// so D2R.exe+0x878FB0 never hits its out-of-bounds int3 trap or re-triggers on an already-selected tab.
auto SafeSelectBankParentTab(uintptr_t bankPanel, uint32_t targetTab, bool* outDidSwitch = nullptr) noexcept -> bool {
	if (outDidSwitch != nullptr) {
		*outDidSwitch = false;
	}
	if (bankPanel == 0 || g_Native.fnWidgetFindChild == 0 || g_Native.fnTabBarSelectTab == 0) {
		return false;
	}
	uintptr_t tabBar = 0;
	if (!SafeCall_FindChild(g_Native.fnWidgetFindChild, bankPanel, "BankTabs", &tabBar) || tabBar == 0) {
		return false;
	}
	int32_t tabCount = 0;
	int32_t curTab   = -1;
	if (!SafeReadU32(reinterpret_cast<const uint32_t*>(tabBar + 0x558), reinterpret_cast<uint32_t*>(&tabCount)) ||
		!SafeReadU32(reinterpret_cast<const uint32_t*>(tabBar + 0x16C4), reinterpret_cast<uint32_t*>(&curTab)) ||
		tabCount <= 0 || tabCount > 16 ||
		targetTab >= static_cast<uint32_t>(tabCount)) {
		return false;
	}
	uint8_t isEnabled = 0;
	if (!SafeReadU8(reinterpret_cast<const uint8_t*>(tabBar + 0x1788 + static_cast<size_t>(targetTab)), &isEnabled) ||
		isEnabled == 0) {
		return false;
	}
	if (static_cast<uint32_t>(curTab) == targetTab) {
		return true;
	}
	if (!SafeCall_SelectTab(g_Native.fnTabBarSelectTab, tabBar, targetTab, 0)) {
		return false;
	}
	if (g_Native.ptrActiveSharedStashUnitId != nullptr) {
		uint32_t newSharedUid = 0xFFFFFFFEu;
		if (targetTab == 1 && g_Native.fnBankPanelGetSelectedRecord != 0) {
			uint64_t recordPtr = 0;
			if (SafeCall_U64FromPtr(g_Native.fnBankPanelGetSelectedRecord, bankPanel, &recordPtr) &&
				recordPtr != 0) {
				SafeReadU32(reinterpret_cast<const uint32_t*>(recordPtr), &newSharedUid);
			}
		}
		SafeWriteU32(g_Native.ptrActiveSharedStashUnitId, newSharedUid);
	}
	if (outDidSwitch != nullptr) {
		*outDidSwitch = true;
	}
	return true;
}

// Pre-writes the desired 0-based shared subpage into BankPanel (+0x168/+0x170) before switching to parent tab 1,
// so BankPanel_OnTabSelected transitions directly to the target shared page in a single frame instead of two.
void PrewriteSharedStashSubpageSlot(uintptr_t bankPanel, uint64_t pageIndex) noexcept {
	if (bankPanel == 0 || g_Native.fnBankPanelGetSharedCount == 0 || g_Native.fnBankPanelIsPrevSeason == 0) {
		return;
	}
	uint64_t pageCount    = 0;
	uint8_t  isPrevSeason = 0;
	if (SafeCall_U64FromPtr(g_Native.fnBankPanelGetSharedCount, bankPanel, &pageCount) &&
		pageIndex < pageCount &&
		SafeCall_BoolFromPtr(g_Native.fnBankPanelIsPrevSeason, bankPanel, &isPrevSeason)) {
		auto* const subpageSlot = reinterpret_cast<uint64_t*>(
			bankPanel + 0x168 + (isPrevSeason != 0 ? 8ULL : 0ULL));
		SafeWriteU64(subpageSlot, pageIndex);
	}
}

// 0xD7C0: Processes pending Personal/Shared tab & subpage jump inside Detour_BankPanelDraw (before originalBankPanelDraw)
void ProcessPendingTabAndPageSwitch(uintptr_t bankPanel, uint64_t nowTickMs) {
	bool     isShared  = false;
	uint64_t pageIndex = 0;
	{
		std::lock_guard<std::mutex> lock(g_StateMutex);
		if (!g_HasPendingJump) {
			return;
		}
		if (g_LastTabSwitchTickMs != 0 &&
			nowTickMs >= g_LastTabSwitchTickMs &&
			(nowTickMs - g_LastTabSwitchTickMs) < kMinTabSwitchIntervalMs) {
			return;
		}
		isShared                = g_PendingJumpIsShared;
		pageIndex               = g_PendingJumpPageIndex;
		g_HasPendingJump        = false;
		g_PendingJumpRetryCount = 0;
	}

	bool didSwitch = false;
	if (!isShared) {
		SafeSelectBankParentTab(bankPanel, static_cast<uint32_t>(pageIndex), &didSwitch);
	} else {
		uint8_t currentTab = 0;
		if (SafeCall_BoolFromPtr(g_Native.fnBankPanelGetSelectedTab, bankPanel, &currentTab)) {
			if (currentTab == 1) {
				SwitchSharedStashSubpage(bankPanel, pageIndex, &didSwitch);
			} else {
				PrewriteSharedStashSubpageSlot(bankPanel, pageIndex);
				SafeSelectBankParentTab(bankPanel, 1, &didSwitch);
			}
		}
	}

	if (didSwitch) {
		std::lock_guard<std::mutex> lock(g_StateMutex);
		g_LastTabSwitchTickMs = nowTickMs;
	}
}

// Queries shared stash page count and currently active 0-based shared subpage (-1 if Personal tab is active)
void GetSharedStashPageInfo(uintptr_t bankPanel, uint64_t* outPageCount, int64_t* outActiveSharedPage) noexcept {
	if (outPageCount != nullptr) {
		*outPageCount = 0;
	}
	if (outActiveSharedPage != nullptr) {
		*outActiveSharedPage = -1;
	}
	if (bankPanel == 0 || g_Native.fnBankPanelGetSharedCount == 0) {
		return;
	}
	uint64_t pageCount = 0;
	if (!SafeCall_U64FromPtr(g_Native.fnBankPanelGetSharedCount, bankPanel, &pageCount) || pageCount == 0) {
		return;
	}
	if (outPageCount != nullptr) {
		*outPageCount = pageCount;
	}
	uint8_t currentTab   = 0;
	uint8_t isPrevSeason = 0;
	if (outActiveSharedPage != nullptr &&
		SafeCall_BoolFromPtr(g_Native.fnBankPanelGetSelectedTab, bankPanel, &currentTab) &&
		currentTab == 1 &&
		SafeCall_BoolFromPtr(g_Native.fnBankPanelIsPrevSeason, bankPanel, &isPrevSeason)) {
		uint64_t curSubpage = 0;
		const auto* subpageSlot = reinterpret_cast<const uint64_t*>(
			bankPanel + 0x168 + (isPrevSeason != 0 ? 8ULL : 0ULL));
		if (SafeReadU64(subpageSlot, &curSubpage) && curSubpage < pageCount) {
			*outActiveSharedPage = static_cast<int64_t>(curSubpage);
		}
	}
}

// Processes queued Ctrl + Mouse Wheel (shared stash pages) and Ctrl + Shift + Mouse Wheel (parent tabs) steps on the UI thread
void ProcessPendingWheelPageStep(uintptr_t bankPanel, uint64_t nowTickMs) {
	if (bankPanel == 0) {
		return;
	}
	int32_t pageStep      = 0;
	int32_t parentTabStep = 0;
	{
		std::lock_guard<std::mutex> lock(g_StateMutex);
		if (g_PendingWheelSteps == 0 && g_PendingParentTabWheelSteps == 0) {
			return;
		}
		if (g_LastTabSwitchTickMs != 0 &&
			nowTickMs >= g_LastTabSwitchTickMs &&
			(nowTickMs - g_LastTabSwitchTickMs) < kMinTabSwitchIntervalMs) {
			return;
		}
		pageStep                     = g_PendingWheelSteps;
		parentTabStep                = g_PendingParentTabWheelSteps;
		g_PendingWheelSteps          = 0;
		g_PendingParentTabWheelSteps = 0;
	}

	bool didSwitch = false;

	// 1. Ctrl + Shift + Mouse Wheel: rotate through enabled parent tabs in "BankTabs"
	if (parentTabStep != 0) {
		uintptr_t tabBar = 0;
		if (SafeCall_FindChild(g_Native.fnWidgetFindChild, bankPanel, "BankTabs", &tabBar) && tabBar != 0) {
			int32_t tabCount = 0;
			int32_t curTab   = 0;
			if (SafeReadU32(reinterpret_cast<const uint32_t*>(tabBar + 0x558), reinterpret_cast<uint32_t*>(&tabCount)) &&
				SafeReadU32(reinterpret_cast<const uint32_t*>(tabBar + 0x16C4), reinterpret_cast<uint32_t*>(&curTab)) &&
				tabCount >= 2 && tabCount <= 16) {
				std::vector<uint32_t> enabledTabs;
				enabledTabs.reserve(static_cast<size_t>(tabCount));
				size_t curIdxInEnabled = 0;
				for (int32_t t = 0; t < tabCount; ++t) {
					uint8_t isEnabled = 0;
					if (SafeReadU8(reinterpret_cast<const uint8_t*>(tabBar + 0x1788 + static_cast<size_t>(t)), &isEnabled) &&
						isEnabled != 0) {
						if (t == curTab) {
							curIdxInEnabled = enabledTabs.size();
						}
						enabledTabs.push_back(static_cast<uint32_t>(t));
					}
				}
				if (enabledTabs.size() >= 2) {
					const int64_t count   = static_cast<int64_t>(enabledTabs.size());
					int64_t       nextIdx = (static_cast<int64_t>(curIdxInEnabled) + parentTabStep) % count;
					if (nextIdx < 0) {
						nextIdx += count;
					}
					const uint32_t targetTab = enabledTabs[static_cast<size_t>(nextIdx)];
					SafeSelectBankParentTab(bankPanel, targetTab, &didSwitch);
				}
			}
		}
	}

	// 2. Ctrl + Mouse Wheel: rotate strictly through Shared Stash pages (0..pageCount-1)
	if (pageStep != 0) {
		uint64_t pageCount        = 0;
		int64_t  activeSharedPage = -1;
		GetSharedStashPageInfo(bankPanel, &pageCount, &activeSharedPage);

		if (activeSharedPage >= 0 && pageCount > 0) {
			const int64_t totalShared = static_cast<int64_t>(pageCount);
			int64_t       nextPage    = (activeSharedPage + pageStep) % totalShared;
			if (nextPage < 0) {
				nextPage += totalShared;
			}
			if (nextPage != activeSharedPage) {
				bool subSwitched = false;
				SwitchSharedStashSubpage(bankPanel, static_cast<uint64_t>(nextPage), &subSwitched);
				didSwitch = didSwitch || subSwitched;
				if (subSwitched) {
					std::lock_guard<std::mutex> lock(g_StateMutex);
					CenterPagePickerOnSharedPageLocked(static_cast<size_t>(nextPage), static_cast<size_t>(pageCount));
				}
			}
		} else if (pageCount > 0) {
			// If on another parent tab (e.g. Personal), pre-write the stepped subpage and switch to Shared tab in 1 step
			uint8_t  isPrevSeason = 0;
			uint64_t curSubpage   = 0;
			if (g_Native.fnBankPanelIsPrevSeason != 0 &&
				SafeCall_BoolFromPtr(g_Native.fnBankPanelIsPrevSeason, bankPanel, &isPrevSeason)) {
				const auto* subpageSlot = reinterpret_cast<const uint64_t*>(
					bankPanel + 0x168 + (isPrevSeason != 0 ? 8ULL : 0ULL));
				if (!SafeReadU64(subpageSlot, &curSubpage) || curSubpage >= pageCount) {
					curSubpage = 0;
				}
			}
			const int64_t totalShared = static_cast<int64_t>(pageCount);
			int64_t       nextPage    = (static_cast<int64_t>(curSubpage) + pageStep) % totalShared;
			if (nextPage < 0) {
				nextPage += totalShared;
			}
			PrewriteSharedStashSubpageSlot(bankPanel, static_cast<uint64_t>(nextPage));
			bool tabSwitched = false;
			SafeSelectBankParentTab(bankPanel, 1, &tabSwitched);
			didSwitch = didSwitch || tabSwitched;
			if (tabSwitched) {
				std::lock_guard<std::mutex> lock(g_StateMutex);
				CenterPagePickerOnSharedPageLocked(static_cast<size_t>(nextPage), static_cast<size_t>(pageCount));
			}
		}
	}

	if (didSwitch) {
		std::lock_guard<std::mutex> lock(g_StateMutex);
		g_LastTabSwitchTickMs = nowTickMs;
	}
}

// Helper: Walks widget parent hierarchy (+0x30) up to 64 levels, multiplying node scale (+0x80)
auto ComputeWidgetHierarchyScale(uintptr_t widget, float* outTotalScale) noexcept -> bool {
	if (widget == 0 || outTotalScale == nullptr) {
		return false;
	}
	float     totalScale = 1.0f;
	uintptr_t curr       = widget;
	size_t    depth      = 0;
	while (true) {
		float nodeScale = 0.0f;
		if (curr >= UINTPTR_MAX - 0x80 ||
			!SafeReadFloat(reinterpret_cast<const float*>(curr + 0x80), &nodeScale) ||
			!std::isfinite(nodeScale) ||
			nodeScale <= 0.0f ||
			nodeScale > 8.0f) {
			return false;
		}
		const double nextScale = static_cast<double>(nodeScale) * static_cast<double>(totalScale);
		if (!std::isfinite(nextScale) || nextScale <= 0.0 || nextScale > 8.0) {
			return false;
		}
		totalScale = static_cast<float>(nextScale);

		uint64_t parent = 0;
		if (curr >= UINTPTR_MAX - 0x30 || !SafeReadU64(reinterpret_cast<const uint64_t*>(curr + 0x30), &parent)) {
			return false;
		}
		if (parent == 0) {
			break;
		}
		if (static_cast<uintptr_t>(parent) == curr || ++depth > 63) {
			return false;
		}
		curr = static_cast<uintptr_t>(parent);
	}
	if (!std::isfinite(totalScale) || totalScale <= 0.0f) {
		return false;
	}
	*outTotalScale = totalScale;
	return true;
}

// 0x68D0: Dims non-matching stash items and highlights matching items in the "grid" child widget
void DimNonMatchingGridItems(uintptr_t bankPanel) {
	if (bankPanel == 0) {
		return;
	}
	if (g_Native.fnBankPanelGetSelectedTab != 0) {
		uint8_t selectedTab = 0;
		if (SafeCall_BoolFromPtr(g_Native.fnBankPanelGetSelectedTab, bankPanel, &selectedTab) && selectedTab > 1) {
			return;
		}
	}
	std::wstring querySnapshot;
	{
		std::lock_guard<std::mutex> lock(g_StateMutex);
		querySnapshot = g_QueryText;
	}
	const std::wstring normalizedQuery = NormalizeWideTooltipLine(querySnapshot);
	if (normalizedQuery.empty() || ParseSlashPageCommand(querySnapshot).state != SlashCommandState::None) {
		return;
	}

	uintptr_t gridWidget = 0;
	if (!SafeCall_FindChild(g_Native.fnWidgetFindChild, bankPanel, "grid", &gridWidget) || gridWidget == 0) {
		return;
	}

	int32_t cellStepX = 0;
	int32_t cellStepY = 0;
	int32_t cols      = 0;
	int32_t rows      = 0;
	if (!SafeReadU32(reinterpret_cast<const uint32_t*>(gridWidget + 0x5B8), reinterpret_cast<uint32_t*>(&cellStepX)) ||
		!SafeReadU32(reinterpret_cast<const uint32_t*>(gridWidget + 0x5BC), reinterpret_cast<uint32_t*>(&cellStepY)) ||
		!SafeReadU32(reinterpret_cast<const uint32_t*>(gridWidget + 0x628), reinterpret_cast<uint32_t*>(&cols)) ||
		!SafeReadU32(reinterpret_cast<const uint32_t*>(gridWidget + 0x62C), reinterpret_cast<uint32_t*>(&rows)) ||
		cellStepX <= 0 || cellStepX > 4096 ||
		cellStepY <= 0 || cellStepY > 4096 ||
		cols <= 0 || cols > 64 || rows <= 0 || rows > 64) {
		return;
	}

	int32_t screenPos[2]{};
	int32_t widgetRect[4]{};
	if (!SafeCall_WidgetGetScreenPos(g_Native.fnWidgetGetScreenPos, gridWidget, screenPos) ||
		!SafeCall_WidgetGetRect(g_Native.fnWidgetGetSize, gridWidget, widgetRect) ||
		widgetRect[2] <= 0 || widgetRect[3] <= 0) {
		return;
	}

	float totalScale = 1.0f;
	if (!ComputeWidgetHierarchyScale(gridWidget, &totalScale)) {
		return;
	}

	int gridW = 0;
	int gridH = 0;
	if (!ScalePositiveCoord(widgetRect[2], totalScale, &gridW) ||
		!ScalePositiveCoord(widgetRect[3], totalScale, &gridH)) {
		return;
	}

	const int     gridX    = screenPos[0];
	const int     gridY    = screenPos[1];
	const int64_t gridEndX = static_cast<int64_t>(gridX) + static_cast<int64_t>(gridW);
	const int64_t gridEndY = static_cast<int64_t>(gridY) + static_cast<int64_t>(gridH);
	if (gridEndX < -2147483648LL || gridEndX > 2147483647LL ||
		gridEndY < -2147483648LL || gridEndY > 2147483647LL ||
		static_cast<int>(gridEndX) <= gridX ||
		static_cast<int>(gridEndY) <= gridY) {
		return;
	}

	const double stepX = static_cast<double>(cellStepX) * static_cast<double>(totalScale);
	const double stepY = static_cast<double>(cellStepY) * static_cast<double>(totalScale);
	if (!std::isfinite(stepX) || stepX < 1.0 || !std::isfinite(stepY) || stepY < 1.0) {
		return;
	}

	const uint64_t nowTickMs = GetTickCount64();
	if (g_GridTooltipCacheTickMs == 0 || nowTickMs < g_GridTooltipCacheTickMs || (nowTickMs - g_GridTooltipCacheTickMs) > 1000) {
		g_GridTooltipCache.clear();
		g_GridTooltipCacheTickMs = nowTickMs;
	}

	struct CachedItemMatch {
		uintptr_t itemUnit = 0;
		bool      matches  = false;
	};
	struct MatchingItemBounds {
		uintptr_t itemUnit = 0;
		int       minX     = 0;
		int       minY     = 0;
		int       maxX     = 0;
		int       maxY     = 0;
	};
	std::vector<CachedItemMatch>    matchCache;
	std::vector<MatchingItemBounds> matchingBounds;

	for (int32_t row = 0; row < rows; ++row) {
		for (int32_t col = 0; col < cols; ++col) {
			const int32_t cellCoord[2] = { col, row };
			uintptr_t     itemUnit     = 0;
			if (!SafeCall_GetGridCellItem(g_Native.fnInventoryGridGetItems, gridWidget, cellCoord, &itemUnit)) {
				continue;
			}

			bool isMatch = false;
			if (itemUnit != 0) {
				bool found = false;
				for (const auto& entry : matchCache) {
					if (entry.itemUnit == itemUnit) {
						isMatch = entry.matches;
						found   = true;
						break;
					}
				}
				if (!found) {
					const std::wstring* cachedTooltip = nullptr;
					for (const auto& tc : g_GridTooltipCache) {
						if (tc.itemUnit == itemUnit) {
							cachedTooltip = &tc.fullTooltip;
							break;
						}
					}
					if (cachedTooltip == nullptr) {
						std::wstring fullTooltip;
						std::wstring shortName;
						BuildItemFullTooltipText(itemUnit, fullTooltip, shortName);
						g_GridTooltipCache.push_back({ itemUnit, std::move(fullTooltip) });
						cachedTooltip = &g_GridTooltipCache.back().fullTooltip;
					}
					if (!cachedTooltip->empty()) {
						isMatch = DoesItemMatchQuery(itemUnit, normalizedQuery, *cachedTooltip);
					}
					matchCache.push_back({ itemUnit, isMatch });
				}
			}

			const double x0f = std::round(static_cast<double>(col) * stepX);
			const double y0f = std::round(static_cast<double>(row) * stepY);
			const double x1f = std::round(static_cast<double>(col + 1) * stepX);
			const double y1f = std::round(static_cast<double>(row + 1) * stepY);
			if (!std::isfinite(x0f) || x0f < 0.0 || x0f > 2147483647.0 ||
				!std::isfinite(y0f) || y0f < 0.0 || y0f > 2147483647.0 ||
				!std::isfinite(x1f) || x1f < 0.0 || x1f > 2147483647.0 ||
				!std::isfinite(y1f) || y1f < 0.0 || y1f > 2147483647.0) {
				continue;
			}
			const int64_t x0 = static_cast<int64_t>(gridX) + static_cast<int64_t>(x0f);
			const int64_t y0 = static_cast<int64_t>(gridY) + static_cast<int64_t>(y0f);
			int64_t       x1 = static_cast<int64_t>(gridX) + static_cast<int64_t>(x1f);
			int64_t       y1 = static_cast<int64_t>(gridY) + static_cast<int64_t>(y1f);
			if (col + 1 == cols && std::abs(x1 - gridEndX) <= 1) {
				x1 = gridEndX;
			}
			if (row + 1 == rows && std::abs(y1 - gridEndY) <= 1) {
				y1 = gridEndY;
			}
			if (x0 < -2147483648LL || x0 > 2147483647LL ||
				y0 < -2147483648LL || y0 > 2147483647LL ||
				x1 <= x0 || y1 <= y0 ||
				x1 < -2147483648LL || x1 > 2147483647LL ||
				y1 < -2147483648LL || y1 > 2147483647LL) {
				continue;
			}

			const int cellLeft   = static_cast<int>(x0);
			const int cellTop    = static_cast<int>(y0);
			const int cellRight  = static_cast<int>(x1);
			const int cellBottom = static_cast<int>(y1);

			if (isMatch) {
				bool foundBounds = false;
				for (auto& b : matchingBounds) {
					if (b.itemUnit == itemUnit) {
						b.minX      = std::min(b.minX, cellLeft);
						b.minY      = std::min(b.minY, cellTop);
						b.maxX      = std::max(b.maxX, cellRight);
						b.maxY      = std::max(b.maxY, cellBottom);
						foundBounds = true;
						break;
					}
				}
				if (!foundBounds) {
					matchingBounds.push_back({ itemUnit, cellLeft, cellTop, cellRight, cellBottom });
				}
				continue;
			}

			SafeCall_DrawFilledRect(
				g_Native.fnDrawFilledRect,
				cellLeft,
				cellTop,
				cellRight,
				cellBottom,
				&kColorGridDimOverlay.r);
		}
	}

	// Draw outer & inner highlight border on the edge of every matching item on the active stash page
	const int outerThick = ScaleBorderThickness(3, totalScale);
	const int innerThick = ScaleBorderThickness(1, totalScale);
	for (const auto& b : matchingBounds) {
		const IntRect outerRect{ b.minX, b.minY, b.maxX - b.minX, b.maxY - b.minY };
		DrawRectBorder(outerRect, kColorMatchEdgeOuter, outerThick);
		if (outerRect.width > outerThick * 2 + 2 && outerRect.height > outerThick * 2 + 2) {
			const IntRect innerRect{
				outerRect.x + outerThick,
				outerRect.y + outerThick,
				outerRect.width - outerThick * 2,
				outerRect.height - outerThick * 2
			};
			DrawRectBorder(innerRect, kColorMatchEdgeInner, innerThick);
		}
	}
}

// Helper: Draws a crisp filled triangle chevron (down ▼ when !pointUp, up ▲ when pointUp) inside btnRect
void DrawChevronIcon(const IntRect& btnRect, bool pointUp, const Color4f& color, float scale) noexcept {
	if (!btnRect.IsValid()) {
		return;
	}
	const int halfW = std::clamp(ScaleBorderThickness(7, scale), 3, std::max(3, btnRect.width / 3));
	const int cx    = btnRect.x + btnRect.width / 2;
	const int cy    = btnRect.y + btnRect.height / 2;
	const int topY  = cy - halfW / 2;
	for (int i = 0; i <= halfW; ++i) {
		const int spanHalf = halfW - i;
		const int rowY     = pointUp ? (topY + (halfW - i)) : (topY + i);
		const IntRect bar{ cx - spanHalf, rowY, spanHalf * 2 + 1, 1 };
		DrawFilledRect(bar, color);
	}
}

// Helper: Draws the search box background, chevron button highlight, divider, and border with a rounded top-right corner
void DrawSearchBoxWithRoundedTopRight(
	const IntRect& boxRect,
	const IntRect& btnRect,
	bool           drawBtnBg,
	const Color4f& btnBgColor,
	const Color4f& borderColor,
	bool           pagePickerOpen,
	int            borderThickness,
	float          scale) noexcept {
	if (!boxRect.IsValid()) {
		return;
	}
	const int T      = std::max(1, borderThickness);
	const int radius = std::clamp(ScaleBorderThickness(12, scale), 3, std::max(3, boxRect.height / 3));
	const int rightX = boxRect.x + boxRect.width;
	const int botY   = boxRect.y + boxRect.height;
	const int innerR = std::max(1, radius - T);

	// 1. Scanlines for the top-right rounded corner (dy = 0 .. radius - 1)
	for (int dy = 0; dy < radius && dy < boxRect.height; ++dy) {
		const double vy        = static_cast<double>(radius - dy) - 0.5;
		const double outerDx   = std::sqrt(std::max(0.0, static_cast<double>(radius * radius) - vy * vy));
		const int    rowRightX = std::clamp((rightX - radius) + static_cast<int>(std::round(outerDx)), boxRect.x + 1, rightX);
		const int    rowY      = boxRect.y + dy;

		DrawFilledRect(IntRect{ boxRect.x, rowY, rowRightX - boxRect.x, 1 }, kColorSearchBoxBg);
		if (drawBtnBg && btnRect.IsValid() && rowRightX > btnRect.x) {
			DrawFilledRect(IntRect{ btnRect.x, rowY, rowRightX - btnRect.x, 1 }, btnBgColor);
		}
	}

	// 2. Rectangular body below the top-right rounded corner (dy = radius .. height - 1)
	if (boxRect.height > radius) {
		DrawFilledRect(IntRect{ boxRect.x, boxRect.y + radius, boxRect.width, boxRect.height - radius }, kColorSearchBoxBg);
		if (drawBtnBg && btnRect.IsValid() && btnRect.height > radius) {
			DrawFilledRect(IntRect{ btnRect.x, btnRect.y + radius, btnRect.width, btnRect.height - radius }, btnBgColor);
		}
	}

	// 3. Chevron button divider & triangle icon
	if (btnRect.IsValid()) {
		DrawFilledRect(IntRect{ btnRect.x, btnRect.y, T, btnRect.height }, borderColor);
		DrawChevronIcon(btnRect, pagePickerOpen, kColorFocusedBorder, scale);
	}

	// 4. Straight border edges (top-left to start of curve, bottom, left, and right below curve)
	if (boxRect.width > radius) {
		DrawFilledRect(IntRect{ boxRect.x, boxRect.y, boxRect.width - radius, T }, borderColor);
	}
	DrawFilledRect(IntRect{ boxRect.x, botY - T, boxRect.width, T }, borderColor);
	DrawFilledRect(IntRect{ boxRect.x, boxRect.y, T, boxRect.height }, borderColor);
	if (boxRect.height > radius) {
		DrawFilledRect(IntRect{ rightX - T, boxRect.y + radius, T, boxRect.height - radius }, borderColor);
	}

	// 5. Rounded top-right border arc (dy = 0 .. radius - 1)
	for (int dy = 0; dy < radius && dy < boxRect.height; ++dy) {
		const double vy        = static_cast<double>(radius - dy) - 0.5;
		const double outerDx   = std::sqrt(std::max(0.0, static_cast<double>(radius * radius) - vy * vy));
		const int    rowRightX = std::clamp((rightX - radius) + static_cast<int>(std::round(outerDx)), boxRect.x + 1, rightX);
		int          arcLeftX  = rightX - radius;
		if (dy >= T && vy < static_cast<double>(innerR)) {
			const double innerDx = std::sqrt(std::max(0.0, static_cast<double>(innerR * innerR) - vy * vy));
			arcLeftX             = std::min(rowRightX - T, (rightX - radius) + static_cast<int>(std::round(innerDx)));
		}
		arcLeftX = std::max(boxRect.x, arcLeftX);
		if (rowRightX > arcLeftX) {
			DrawFilledRect(IntRect{ arcLeftX, boxRect.y + dy, rowRightX - arcLeftX, 1 }, borderColor);
		}
	}
}

// 0x5210: Computes the screen-space layout of the search bar, page-picker button/grid, dropdown rows, and scrollbar
auto ComputeSearchBarLayout(
	uintptr_t                                              bankPanel,
	bool                                                   showSearchDropdown,
	size_t                                                 matchCount,
	size_t                                                 selectedIndex,
	size_t                                                 scrollOffset,
	bool                                                   pagePickerOpen,
	size_t                                                 sharedPageCount,
	size_t                                                 pagePickerScrollRow,
	const void*                                            textStyleBuf,
	std::wstring_view                                      personalLbl,
	std::wstring_view                                      gemsLbl,
	std::wstring_view                                      materialsLbl,
	std::wstring_view                                      runesLbl,
	std::wstring_view                                      customPersonalName,
	const std::vector<std::pair<uint64_t, std::wstring>>&  customSharedNames,
	bool                                                   isRenamingTab,
	bool                                                   renamingTabIsShared,
	uint64_t                                               renamingTabPageIndex,
	std::wstring_view                                      tabRenameText) -> SearchBarLayout {
	SearchBarLayout layout{};
	if (bankPanel == 0) {
		return layout;
	}
	uintptr_t anchor = 0;
	uintptr_t tabBar = 0;
	SafeCall_FindChild(g_Native.fnWidgetFindChild, bankPanel, "StashSearchAnchor", &anchor);
	SafeCall_FindChild(g_Native.fnWidgetFindChild, bankPanel, "BankTabs", &tabBar);
	const uintptr_t scaleSource = (anchor != 0) ? anchor : tabBar;
	if (scaleSource == 0) {
		return layout;
	}

	float totalScale = 1.0f;
	if (!ComputeWidgetHierarchyScale(scaleSource, &totalScale)) {
		return layout;
	}

	int32_t screenPos[2]{};
	int32_t widgetRect[4]{};
	bool    hasValidAnchor = false;
	if (anchor != 0 &&
		SafeCall_WidgetGetScreenPos(g_Native.fnWidgetGetScreenPos, anchor, screenPos) &&
		SafeCall_WidgetGetRect(g_Native.fnWidgetGetSize, anchor, widgetRect) &&
		widgetRect[2] > 0 && widgetRect[3] > 0) {
		hasValidAnchor = true;
	}

	// Align search bar to the exact top and bottom of the "Runes" tab in "BankTabs"
	// (x = BankTabs.x + 990, y = BankTabs.y + 2, width = 584, height = 56 in 4K layout coordinates).
	// If "StashSearchAnchor" is not present in the mod's bankexpansionlayouthd.json, fall back directly to "BankTabs".
	int32_t tabScreenPos[2]{};
	if (tabBar != 0 &&
		SafeCall_WidgetGetScreenPos(g_Native.fnWidgetGetScreenPos, tabBar, tabScreenPos)) {
		int offsetY = 0;
		int offsetX = 0;
		if (ScalePositiveCoord(2, totalScale, &offsetY) &&
			ScalePositiveCoord(990, totalScale, &offsetX)) {
			if (!hasValidAnchor || screenPos[1] > tabScreenPos[1] + 50) {
				screenPos[0] = tabScreenPos[0] + offsetX;
			}
			screenPos[1]   = tabScreenPos[1] + offsetY;
			widgetRect[2]  = 584;
			widgetRect[3]  = 56;
			hasValidAnchor = true;
		}
	}
	if (!hasValidAnchor) {
		return layout;
	}

	int boxW = 0;
	int boxH = 0;
	if (!ScalePositiveCoord(widgetRect[2], totalScale, &boxW) ||
		!ScalePositiveCoord(widgetRect[3], totalScale, &boxH)) {
		return layout;
	}

	const int     boxX    = screenPos[0];
	const int     boxY    = screenPos[1];
	const int64_t boxEndX = static_cast<int64_t>(boxX) + static_cast<int64_t>(boxW);
	const int64_t boxEndY = static_cast<int64_t>(boxY) + static_cast<int64_t>(boxH);
	if (boxEndX < -2147483648LL || boxEndX > 2147483647LL ||
		boxEndY < -2147483648LL || boxEndY > 2147483647LL ||
		static_cast<int>(boxEndX) <= boxX ||
		static_cast<int>(boxEndY) <= boxY) {
		return layout;
	}

	const HWND gameHwnd = FindGameWindow();
	RECT       clientRc{};
	if (gameHwnd == nullptr || !GetClientRect(gameHwnd, &clientRc) ||
		clientRc.left != 0 || clientRc.top != 0 ||
		clientRc.right <= 0 || clientRc.bottom <= 0) {
		return layout;
	}
	const int clientW = clientRc.right;
	const int clientH = clientRc.bottom;

	if (boxX < 0 || boxY < 0 || boxW <= 0 || boxH <= 0 ||
		static_cast<int>(boxEndX) > clientW ||
		static_cast<int>(boxEndY) > clientH) {
		return layout;
	}

	const int minBoxW  = ScaleBorderThickness(280, totalScale);
	const int textPadX = ScaleBorderThickness(18, totalScale);
	const int btnW     = boxH;
	if (boxW < minBoxW || boxW - btnW - textPadX * 2 <= 0) {
		return layout;
	}

	layout.valid               = true;
	layout.scale               = totalScale;
	layout.searchBox           = IntRect{ boxX, boxY, boxW, boxH };
	layout.pageButtonRect      = IntRect{ boxX + boxW - btnW, boxY, btnW, boxH };
	layout.searchTextRect      = IntRect{ boxX + textPadX, boxY, boxW - btnW - textPadX * 2, boxH };
	layout.dropdownScale       = totalScale;
	layout.clampedScrollOffset = scrollOffset;

	const int rowTextPadX = ScaleBorderThickness(18, totalScale);

	int dropdownGapY   = 0;
	int bottomMargin   = 0;
	int dropdownPad    = 0;
	int scrollbarWidth = 0;
	int minThumbHeight = 0;
	if (!ScalePositiveDimension(4, totalScale, &dropdownGapY) ||
		!ScalePositiveDimension(12, totalScale, &bottomMargin) ||
		!ScalePositiveDimension(6, totalScale, &dropdownPad) ||
		!ScalePositiveDimension(14, totalScale, &scrollbarWidth) ||
		!ScalePositiveDimension(28, totalScale, &minThumbHeight)) {
		return layout;
	}

	const int     rowHeight   = boxH;
	const int     dropdownX   = boxX;
	const int64_t dropdownY64 = static_cast<int64_t>(boxY) + static_cast<int64_t>(boxH) + static_cast<int64_t>(dropdownGapY);
	const int     dropdownW   = boxW;
	const int64_t maxBottom   = static_cast<int64_t>(clientH) - static_cast<int64_t>(bottomMargin);
	const int64_t availH      = maxBottom - static_cast<int64_t>(dropdownPad) * 2 - dropdownY64;
	if (rowHeight <= 0 || dropdownY64 < 0 || dropdownY64 > 2147483647LL || availH < rowHeight) {
		return layout;
	}

	const int    dropdownY  = static_cast<int>(dropdownY64);
	const size_t maxFitRows = static_cast<size_t>(availH / rowHeight);

	// 1. PoE-style vertical side tab flyout (docked to the right outer edge of the Stash panel)
	if (pagePickerOpen) {
		int sideGapX    = 0;
		int pickerW     = 0;
		int pickerRowH  = 0;
		int dividerGapH = 0;
		if (ScalePositiveDimension(96, totalScale, &sideGapX) &&
			ScalePositiveDimension(280, totalScale, &pickerW) &&
			ScalePositiveDimension(48, totalScale, &pickerRowH) &&
			ScalePositiveDimension(8, totalScale, &dividerGapH)) {
			// Determine which non-Shared parent tabs (0 = Personal, 2 = Gems, 3 = Materials, 4 = Runes) are enabled in BankTabs
			constexpr uint64_t kCandidateParentTabs[kPagePickerParentTabRows] = { 0, 2, 3, 4 };
			uint64_t           enabledParentTabs[kPagePickerParentTabRows]{};
			size_t             enabledParentTabCount = 0;
			uintptr_t          bankTabsWidget        = 0;
			int32_t            tabCount              = 0;
			if (SafeCall_FindChild(g_Native.fnWidgetFindChild, bankPanel, "BankTabs", &bankTabsWidget) &&
				bankTabsWidget != 0 &&
				SafeReadU32(reinterpret_cast<const uint32_t*>(bankTabsWidget + 0x558), reinterpret_cast<uint32_t*>(&tabCount)) &&
				tabCount > 0 && tabCount <= 16) {
				for (uint64_t candTab : kCandidateParentTabs) {
					if (candTab < static_cast<uint64_t>(tabCount)) {
						uint8_t isEnabled = 0;
						if (SafeReadU8(reinterpret_cast<const uint8_t*>(bankTabsWidget + 0x1788 + static_cast<size_t>(candTab)), &isEnabled) &&
							isEnabled != 0) {
							enabledParentTabs[enabledParentTabCount++] = candTab;
						}
					}
				}
			}
			if (enabledParentTabCount == 0) {
				enabledParentTabs[enabledParentTabCount++] = 0;
			}

			// Dynamically expand pickerW so the right border comfortably fits the longest parent or custom tab name
			if (textStyleBuf != nullptr) {
				const IntRect measureBounds{ 0, 0, clientW, clientH };
				uint32_t      maxLabelW = 0;
				const auto    measureCandidate = [&](std::wstring_view cand) noexcept {
					if (!cand.empty()) {
						uint32_t w = 0;
						uint32_t h = 0;
						if (MeasureNativeText(cand, measureBounds, textStyleBuf, totalScale, &w, &h) && w < 0x10000u) {
							maxLabelW = std::max(maxLabelW, w);
						}
					}
				};
				for (size_t p = 0; p < enabledParentTabCount; ++p) {
					if (enabledParentTabs[p] == 0) {
						measureCandidate(personalLbl);
						if (!customPersonalName.empty()) {
							measureCandidate(customPersonalName);
						}
						if (isRenamingTab && !renamingTabIsShared && renamingTabPageIndex == 0) {
							const std::wstring activeEdit = std::wstring(tabRenameText) + L"|";
							measureCandidate(activeEdit);
						}
					} else if (enabledParentTabs[p] == 2) {
						measureCandidate(gemsLbl);
					} else if (enabledParentTabs[p] == 3) {
						measureCandidate(materialsLbl);
					} else if (enabledParentTabs[p] == 4) {
						measureCandidate(runesLbl);
					}
				}
				for (const auto& [idx, customName] : customSharedNames) {
					if (idx < sharedPageCount && !customName.empty()) {
						const std::wstring formatted = std::to_wstring(idx + 1) + L" - " + customName;
						measureCandidate(formatted);
					}
				}
				if (isRenamingTab && renamingTabIsShared && renamingTabPageIndex < sharedPageCount) {
					const std::wstring activeEdit =
						std::to_wstring(renamingTabPageIndex + 1) + L" - " + std::wstring(tabRenameText) + L"|";
					measureCandidate(activeEdit);
				}
				if (maxLabelW > 0) {
					const int cellPadX = ScaleBorderThickness(14, totalScale);
					const int extraPad = ScaleBorderThickness(18, totalScale);
					const int neededW  = static_cast<int>(maxLabelW) + cellPadX * 2 + dropdownPad * 2 + extraPad;
					int       maxPickW = neededW;
					if (ScalePositiveDimension(560, totalScale, &maxPickW)) {
						pickerW = std::clamp(std::max(pickerW, neededW), pickerW, maxPickW);
					} else {
						pickerW = std::max(pickerW, neededW);
					}
				}
			}

			const int rightMargin = ScaleBorderThickness(8, totalScale);
			int       pickerX     = boxX + boxW + sideGapX;
			if (pickerX + pickerW > clientW - rightMargin) {
				pickerX = std::max(0, clientW - rightMargin - pickerW);
			}

			const int     pickerY      = boxY;
			const int64_t pickerAvailH = maxBottom - static_cast<int64_t>(dropdownPad) * 2 - static_cast<int64_t>(pickerY);
			const int     parentSecH   = static_cast<int>(enabledParentTabCount) * pickerRowH;
			const int     sharedAvailH = static_cast<int>(pickerAvailH) - parentSecH - dividerGapH;

			size_t visibleSharedRows = 0;
			if (sharedPageCount > 0 && sharedAvailH >= pickerRowH) {
				const size_t maxFitShared = static_cast<size_t>(sharedAvailH / pickerRowH);
				visibleSharedRows         = std::min({ kMaxPageGridVisibleRows, sharedPageCount, maxFitShared });
			}
			const size_t maxScrollRow = (sharedPageCount > visibleSharedRows) ? (sharedPageCount - visibleSharedRows) : 0;
			const size_t clampedRow   = std::min(pagePickerScrollRow, maxScrollRow);

			const int sharedSecH = (visibleSharedRows > 0) ? (dividerGapH + static_cast<int>(visibleSharedRows) * pickerRowH) : 0;
			const int pickerH    = parentSecH + sharedSecH + dropdownPad * 2;
			const int pInnerX    = pickerX + dropdownPad;
			const int pInnerY    = pickerY + dropdownPad;
			const int pInnerW    = pickerW - dropdownPad * 2;

			if (pInnerW > 0 && pickerY + pickerH <= static_cast<int>(maxBottom)) {
				const bool pickerHasScroll = (visibleSharedRows < sharedPageCount);
				const int  cellW           = pInnerW - (pickerHasScroll ? (scrollbarWidth + ScaleBorderThickness(4, totalScale)) : 0);
				if (cellW > 0) {
					layout.pagePickerOpen          = true;
					layout.pagePickerBox           = IntRect{ pickerX, pickerY, pickerW, pickerH };
					layout.pagePickerTotalPages    = sharedPageCount;
					layout.pagePickerTotalRows     = sharedPageCount;
					layout.pagePickerVisRows       = visibleSharedRows;
					layout.pagePickerScrollRow     = clampedRow;
					layout.pagePickerHasScrollbar  = pickerHasScroll;

					// Top section: Enabled parent tabs (Personal, Gems, Materials, Runes)
					size_t cellCount = 0;
					for (size_t p = 0; p < enabledParentTabCount && cellCount < kMaxPageGridVisibleCells; ++p) {
						const int rowY = pInnerY + static_cast<int>(p) * pickerRowH;
						layout.pageCellRects[cellCount]    = IntRect{ pInnerX, rowY, pInnerW, pickerRowH };
						layout.pageCellIndices[cellCount]  = enabledParentTabs[p];
						layout.pageCellIsShared[cellCount] = false;
						++cellCount;
					}

					// Divider and bottom section: Shared Stash pages (1..N)
					if (visibleSharedRows > 0) {
						const int divH = ScaleBorderThickness(2, totalScale);
						const int divY = pInnerY + parentSecH + (dividerGapH - divH) / 2;
						layout.pagePickerDividerRect = IntRect{ pInnerX, divY, pInnerW, divH };

						const int sharedStartY = pInnerY + parentSecH + dividerGapH;
						for (size_t r = 0; r < visibleSharedRows && cellCount < kMaxPageGridVisibleCells; ++r) {
							const size_t pageIdx = clampedRow + r;
							if (pageIdx >= sharedPageCount) {
								break;
							}
							const int rowY = sharedStartY + static_cast<int>(r) * pickerRowH;
							layout.pageCellRects[cellCount]    = IntRect{ pInnerX, rowY, cellW, pickerRowH };
							layout.pageCellIndices[cellCount]  = pageIdx;
							layout.pageCellIsShared[cellCount] = true;
							++cellCount;
						}

						if (pickerHasScroll) {
							const int trackX = (pickerX + pickerW) - scrollbarWidth - dropdownPad;
							const int trackY = sharedStartY;
							const int trackW = scrollbarWidth;
							const int trackH = static_cast<int>(visibleSharedRows) * pickerRowH;
							int       thumbH = trackH;
							if (minThumbHeight < trackH) {
								const double rawThumbH = std::round(
									(static_cast<double>(trackH) * static_cast<double>(visibleSharedRows)) /
									static_cast<double>(sharedPageCount));
								thumbH = std::clamp(static_cast<int>(rawThumbH), minThumbHeight, trackH);
							}
							const size_t maxScroll = sharedPageCount - visibleSharedRows;
							int          thumbY    = trackY;
							if (maxScroll > 0 && trackH > thumbH) {
								const double rawOffset = std::round(
									(static_cast<double>(trackH - thumbH) * static_cast<double>(clampedRow)) /
									static_cast<double>(maxScroll));
								thumbY += std::clamp(static_cast<int>(rawOffset), 0, trackH - thumbH);
							}
							layout.pagePickerScrollbarTrack = IntRect{ trackX, trackY, trackW, trackH };
							layout.pagePickerScrollbarThumb = IntRect{ trackX, thumbY, trackW, thumbH };
						}
					}
					layout.pageCellCount = cellCount;
				}
			}
		}
	}

	if (!showSearchDropdown) {
		return layout;
	}

	size_t visibleRows = std::min({ kMaxDropdownVisibleRows, matchCount, maxFitRows });
	if (visibleRows == 0) {
		return layout;
	}

	size_t clampedSel    = selectedIndex;
	size_t clampedScroll = scrollOffset;
	AdjustDropdownSelectionAndScroll(matchCount, visibleRows, clampedSel, clampedSel, clampedScroll);
	if (matchCount > clampedScroll) {
		visibleRows = std::min(visibleRows, matchCount - clampedScroll);
	} else {
		visibleRows = 0;
	}
	if (visibleRows == 0) {
		return layout;
	}

	const int dropdownH = static_cast<int>(visibleRows) * rowHeight + dropdownPad * 2;
	const int innerX    = dropdownX + dropdownPad;
	const int innerY    = dropdownY + dropdownPad;
	const int innerW    = dropdownW - dropdownPad * 2;
	const int innerH    = static_cast<int>(visibleRows) * rowHeight;
	if (innerW <= 0 || innerH <= 0 || dropdownY + dropdownH > static_cast<int>(maxBottom)) {
		return layout;
	}

	const bool hasScrollbar = (visibleRows < matchCount);
	const int  rowTextW     = innerW - (hasScrollbar ? scrollbarWidth : 0) - rowTextPadX * 2;
	if (rowTextW <= 0) {
		return layout;
	}

	layout.visibleRowCount     = visibleRows;
	layout.dropdownBox         = IntRect{ dropdownX, dropdownY, dropdownW, dropdownH };
	layout.clampedScrollOffset = clampedScroll;
	layout.hasScrollbar        = hasScrollbar;

	if (hasScrollbar) {
		const int trackX = (dropdownX + dropdownW) - scrollbarWidth - dropdownPad;
		const int trackY = innerY;
		const int trackW = scrollbarWidth;
		const int trackH = innerH;
		if (trackX < innerX || trackW <= 0 || trackH <= 0) {
			return layout;
		}
		int thumbH = trackH;
		if (minThumbHeight < trackH) {
			const double rawThumbH = std::round(
				(static_cast<double>(trackH) * static_cast<double>(visibleRows)) / static_cast<double>(matchCount));
			thumbH = std::clamp(static_cast<int>(rawThumbH), minThumbHeight, trackH);
		}
		const size_t maxScroll = matchCount - visibleRows;
		int          thumbY    = trackY;
		if (maxScroll > 0 && trackH > thumbH) {
			const double rawOffset = std::round(
				(static_cast<double>(trackH - thumbH) * static_cast<double>(clampedScroll)) / static_cast<double>(maxScroll));
			thumbY += std::clamp(static_cast<int>(rawOffset), 0, trackH - thumbH);
		}
		layout.scrollbarTrack = IntRect{ trackX, trackY, trackW, trackH };
		layout.scrollbarThumb = IntRect{ trackX, thumbY, trackW, thumbH };
	}

	for (size_t i = 0; i < visibleRows; ++i) {
		const int rowY = innerY + static_cast<int>(i) * rowHeight;
		layout.dropdownRows[i]         = IntRect{ innerX, rowY, innerW, rowHeight };
		layout.dropdownTextRects[i]    = IntRect{ innerX + rowTextPadX, rowY, rowTextW, rowHeight };
		layout.dropdownMatchIndices[i] = clampedScroll + i;
	}

	return layout;
}

// 0x9DA0: Formats a dropdown row label, eliding with L"…" when wider than available pixel width while preserving the tab suffix
auto FormatDropdownRowWithEllipsis(
	std::wstring_view fullText,
	std::wstring_view preservedSuffix,
	const IntRect&    rowTextRect,
	const void*       styleBuf,
	float             scale,
	bool*             outIsTruncated) -> std::wstring {
	if (outIsTruncated != nullptr) {
		*outIsTruncated = false;
	}
	if (fullText.empty() || !rowTextRect.IsValid()) {
		return {};
	}
	uint32_t measuredW = 0;
	uint32_t measuredH = 0;
	if (!MeasureNativeText(fullText, rowTextRect, styleBuf, scale, &measuredW, &measuredH)) {
		return {};
	}
	if (static_cast<int>(measuredW) <= rowTextRect.width) {
		return std::wstring(fullText);
	}

	if (outIsTruncated != nullptr) {
		*outIsTruncated = true;
	}
	constexpr std::wstring_view kEllipsis = L"\x2026";
	uint32_t ellipsisW = 0;
	uint32_t ellipsisH = 0;
	if (!MeasureNativeText(kEllipsis, rowTextRect, styleBuf, scale, &ellipsisW, &ellipsisH) ||
		static_cast<int>(ellipsisW) > rowTextRect.width) {
		return {};
	}

	std::wstring_view bodyText   = fullText;
	std::wstring_view tailSuffix = {};
	if (!preservedSuffix.empty() &&
		fullText.size() > preservedSuffix.size() &&
		fullText.substr(fullText.size() - preservedSuffix.size()) == preservedSuffix) {
		std::wstring minWithSuffix;
		minWithSuffix.reserve(kEllipsis.size() + preservedSuffix.size());
		minWithSuffix.append(kEllipsis);
		minWithSuffix.append(preservedSuffix);
		uint32_t minW = 0;
		uint32_t minH = 0;
		if (MeasureNativeText(minWithSuffix, rowTextRect, styleBuf, scale, &minW, &minH) &&
			static_cast<int>(minW) <= rowTextRect.width) {
			bodyText   = fullText.substr(0, fullText.size() - preservedSuffix.size());
			tailSuffix = preservedSuffix;
		}
	}

	// Build UTF-16 codepoint boundaries so surrogate pairs are never split
	std::vector<size_t> boundaries;
	boundaries.reserve(bodyText.size() + 1);
	boundaries.push_back(0);
	for (size_t i = 0; i < bodyText.size();) {
		const auto u0 = static_cast<uint16_t>(bodyText[i]);
		if (static_cast<uint16_t>(u0 - 0xD800u) < 0x0400u && i + 1 < bodyText.size()) {
			const auto u1 = static_cast<uint16_t>(bodyText[i + 1]);
			if (static_cast<uint16_t>(u1 - 0xDC00u) < 0x0400u) {
				i += 2;
				boundaries.push_back(i);
				continue;
			}
		}
		++i;
		boundaries.push_back(i);
	}

	size_t       lo   = 0;
	size_t       hi   = boundaries.size() - 1;
	std::wstring best = std::wstring(kEllipsis) + std::wstring(tailSuffix);
	while (lo < hi) {
		const size_t mid       = lo + (hi - lo + 1) / 2;
		const size_t prefixLen = boundaries[mid];
		std::wstring candidate;
		candidate.reserve(prefixLen + kEllipsis.size() + tailSuffix.size());
		candidate.append(bodyText.substr(0, prefixLen));
		candidate.append(kEllipsis);
		candidate.append(tailSuffix);
		uint32_t candW = 0;
		uint32_t candH = 0;
		if (MeasureNativeText(candidate, rowTextRect, styleBuf, scale, &candW, &candH) &&
			static_cast<int>(candW) <= rowTextRect.width) {
			best = std::move(candidate);
			lo   = mid;
		} else {
			hi = mid - 1;
		}
	}
	return best;
}

// 0x7DE0: Draws a floating tooltip box when hovering over a truncated dropdown row
void DrawHoveredDropdownTooltip(
	const SearchBarLayout&         layout,
	const std::vector<MatchEntry>& matches,
	const bool*                    rowTruncated,
	const void*                    styleBuf,
	float                          fontScale) {
	if (layout.visibleRowCount == 0 || rowTruncated == nullptr) {
		return;
	}
	const HWND gameHwnd = FindGameWindow();
	if (gameHwnd == nullptr || GetForegroundWindow() != gameHwnd) {
		return;
	}
	POINT cursorPt{};
	RECT  clientRc{};
	if (!GetCursorPos(&cursorPt) ||
		!ScreenToClient(gameHwnd, &cursorPt) ||
		!GetClientRect(gameHwnd, &clientRc) ||
		clientRc.left != 0 || clientRc.top != 0) {
		return;
	}
	const int clientW = clientRc.right;
	const int clientH = clientRc.bottom;
	if (clientW <= 0 || clientH <= 0 || !std::isfinite(fontScale) || fontScale <= 0.0f ||
		cursorPt.x < 0 || cursorPt.x >= clientW || cursorPt.y < 0 || cursorPt.y >= clientH) {
		return;
	}

	const size_t rows = std::min(layout.visibleRowCount, kMaxDropdownVisibleRows);
	for (size_t i = 0; i < rows; ++i) {
		if (!layout.dropdownRows[i].Contains(cursorPt.x, cursorPt.y)) {
			continue;
		}
		const uint64_t matchIdx = layout.dropdownMatchIndices[i];
		if (matchIdx >= matches.size()) {
			return;
		}
		const std::wstring& fullText = !matches[matchIdx].hoverText.empty()
										   ? matches[matchIdx].hoverText
										   : matches[matchIdx].displayText;
		if (fullText.empty() || (!rowTruncated[i] && fullText == matches[matchIdx].displayText)) {
			return;
		}

		int padX   = 0;
		int padY   = 0;
		int gapY   = 0;
		int margin = 0;
		if (!ScalePositiveDimension(10, fontScale, &padX) ||
			!ScalePositiveDimension(6, fontScale, &padY) ||
			!ScalePositiveDimension(8, fontScale, &gapY) ||
			!ScalePositiveDimension(8, fontScale, &margin)) {
			return;
		}

		const IntRect screenRect{ 0, 0, clientW, clientH };
		uint32_t      textW = 0;
		uint32_t      textH = 0;
		if (!MeasureNativeText(fullText, screenRect, styleBuf, fontScale, &textW, &textH) ||
			textW == 0 || textH == 0 || textW > 0x7FFFFFFFu || textH > 0x7FFFFFFFu) {
			return;
		}

		const int boxW = static_cast<int>(textW) + padX * 2;
		const int boxH = static_cast<int>(textH) + padY * 2;
		if (boxW <= 0 || boxH <= 0 || clientW - margin < margin + boxW || clientH - margin < margin + boxH) {
			return;
		}

		int boxX = std::clamp<int>(static_cast<int>(cursorPt.x), margin, clientW - margin - boxW);
		int boxY = layout.dropdownRows[i].y - gapY - boxH;
		if (boxY < margin) {
			boxY = std::clamp(
				layout.dropdownRows[i].y + layout.dropdownRows[i].height + gapY,
				margin,
				clientH - margin - boxH);
		}

		const IntRect tooltipBox{ boxX, boxY, boxW, boxH };
		DrawFilledRect(tooltipBox, kColorDropdownRowBg);
		DrawRectBorder(tooltipBox, kColorFocusedBorder, ScaleBorderThickness(2, fontScale));

		const IntRect textRect{ boxX + padX, boxY + padY, static_cast<int>(textW), static_cast<int>(textH) };
		DrawNativeText(fullText, textRect, styleBuf, fontScale);
		return;
	}
}

// 0xE8C0: Resolves localized "LobbySearch" placeholder and parent tab names once per session
void EnsureLocalizedSearchPlaceholder() {
	{
		std::lock_guard<std::mutex> lock(g_StateMutex);
		if (g_PlaceholderResolved) {
			return;
		}
	}
	std::wstring localized = L"Search";
	if (!LookupLocalizedString("LobbySearch", 11, localized)) {
		localized = L"Search";
	}
	std::wstring personalLbl  = L"Personal";
	std::wstring gemsLbl      = L"Gems";
	std::wstring materialsLbl = L"Materials";
	std::wstring runesLbl     = L"Runes";
	LookupLocalizedString("personal", 8, personalLbl);
	LookupLocalizedString("gems", 4, gemsLbl);
	LookupLocalizedString("materials", 9, materialsLbl);
	LookupLocalizedString("runes", 5, runesLbl);

	std::lock_guard<std::mutex> lock(g_StateMutex);
	if (!g_PlaceholderResolved) {
		g_SearchPlaceholder   = std::move(localized);
		g_TabLabelPersonal    = std::move(personalLbl);
		g_TabLabelGems        = std::move(gemsLbl);
		g_TabLabelMaterials   = std::move(materialsLbl);
		g_TabLabelRunes       = std::move(runesLbl);
		g_PlaceholderResolved = true;
	}
}

// 0x87A0: Main UI rendering routine for the search box, page-picker button/grid, dropdown rows, scrollbar, and tooltip
void RenderSearchBarAndDropdown(uintptr_t bankPanel) {
	if (bankPanel == 0) {
		return;
	}
	EnsureLocalizedSearchPlaceholder();
	const uint64_t nowTickMs = GetTickCount64();

	uint64_t sharedPageCount  = 0;
	int64_t  activeSharedPage = -1;
	GetSharedStashPageInfo(bankPanel, &sharedPageCount, &activeSharedPage);
	uint8_t  activeParentTab  = 0;
	if (g_Native.fnBankPanelGetSelectedTab != 0) {
		SafeCall_BoolFromPtr(g_Native.fnBankPanelGetSelectedTab, bankPanel, &activeParentTab);
	}

	// Check cursor position for hover states and live scrollbar dragging before snapshotting layout inputs
	POINT cursorPt{ -1, -1 };
	int   rawCursorY    = 0;
	bool  hasRawCursorY = false;
	{
		const HWND gameHwnd = FindGameWindow();
		if (gameHwnd != nullptr && GetForegroundWindow() == gameHwnd) {
			POINT pt{};
			RECT  clientRc{};
			if (GetCursorPos(&pt) && ScreenToClient(gameHwnd, &pt) && GetClientRect(gameHwnd, &clientRc)) {
				rawCursorY    = pt.y;
				hasRawCursorY = true;
				if (pt.x >= 0 && pt.y >= 0 && pt.x < clientRc.right && pt.y < clientRc.bottom) {
					cursorPt = pt;
				}
			}
		}
	}

	const auto activeDragMode = static_cast<ScrollbarDragMode>(g_ScrollbarDragMode.load(std::memory_order_acquire));

	std::wstring                                   query;
	std::wstring                                   placeholder;
	std::wstring                                   personalLbl;
	std::wstring                                   gemsLbl;
	std::wstring                                   materialsLbl;
	std::wstring                                   runesLbl;
	std::wstring                                   imeComp;
	std::vector<MatchEntry>                        matches;
	bool                                           isFocused            = false;
	bool                                           placeholderSelected  = false;
	bool                                           placeholderCleared   = false;
	bool                                           imeComposing         = false;
	uint32_t                                       imeStart             = 0;
	uint32_t                                       imeEnd               = 0;
	uint32_t                                       imeCursor            = 0;
	uint32_t                                       selStart             = 0;
	uint32_t                                       selEnd               = 0;
	size_t                                         selectedIndex        = 0;
	size_t                                         scrollOffset         = 0;
	bool                                           pagePickerOpen       = false;
	size_t                                         pagePickerScrollRow  = 0;
	std::wstring                                   customPersonalName;
	std::vector<std::pair<uint64_t, std::wstring>> customSharedNames;
	bool                                           isRenamingTab        = false;
	bool                                           renamingTabIsShared  = true;
	uint64_t                                       renamingTabPageIndex = 0;
	std::wstring                                   tabRenameText;
	bool                                           saveClosedRename     = false;
	StashSearchConfig                              closedRenameCfg{};
	{
		std::lock_guard<std::mutex> lock(g_StateMutex);
		if (!g_WasPanelActive) {
			if (g_IsRenamingTab && CommitActiveTabRenameLocked()) {
				saveClosedRename = true;
				closedRenameCfg  = SnapshotPersistedConfigLocked();
			}
			g_WasPanelActive      = true;
			g_IsFocused           = false;
			g_IsRenamingTab       = false;
			g_TabRenameText.clear();
			g_PlaceholderSelected = false;
			g_PlaceholderCleared  = false;
		}
		if (activeDragMode != ScrollbarDragMode::None && hasRawCursorY && g_CurrentLayout.valid) {
			ApplyScrollbarDragAtYLocked(activeDragMode, rawCursorY);
		}
		if (g_PendingCenterPagePicker && sharedPageCount > 0) {
			CenterPagePickerOnSharedPageLocked(g_PendingCenterPageIndex, static_cast<size_t>(sharedPageCount));
			g_PendingCenterPagePicker = false;
		}
		query                = g_QueryText;
		placeholder          = g_SearchPlaceholder;
		personalLbl          = g_TabLabelPersonal;
		gemsLbl              = g_TabLabelGems;
		materialsLbl         = g_TabLabelMaterials;
		runesLbl             = g_TabLabelRunes;
		imeComp              = g_ImeCompositionText;
		matches              = g_Matches;
		isFocused            = g_IsFocused;
		placeholderSelected  = g_PlaceholderSelected;
		placeholderCleared   = g_PlaceholderCleared;
		imeComposing         = g_ImeComposing;
		imeStart             = g_ImeCursorStart;
		imeEnd               = g_ImeCursorEnd;
		imeCursor            = g_ImeCursorPos;
		selStart             = g_SelStart;
		selEnd               = g_SelEnd;
		selectedIndex        = g_SelectedIndex;
		scrollOffset         = g_ScrollOffset;
		pagePickerOpen       = g_PagePickerOpen;
		pagePickerScrollRow  = g_PagePickerScrollRow;
		customPersonalName   = g_CustomPersonalTabName;
		customSharedNames    = g_CustomSharedTabNames;
		isRenamingTab        = g_IsRenamingTab;
		renamingTabIsShared  = g_RenamingTabIsShared;
		renamingTabPageIndex = g_RenamingTabPageIndex;
		tabRenameText        = g_TabRenameText;
	}
	if (saveClosedRename) {
		SavePersistedConfig(g_PluginCtx, closedRenameCfg);
	}

	// Resolve font index (+0xB0) and text style params (+0xB4) from TextBoxWidget "gold_amount", falling back to "title"
	uintptr_t styleSourceWidget = 0;
	if (!SafeCall_FindChild(g_Native.fnWidgetFindChild, bankPanel, "gold_amount", &styleSourceWidget) || styleSourceWidget == 0) {
		SafeCall_FindChild(g_Native.fnWidgetFindChild, bankPanel, "title", &styleSourceWidget);
	}
	uint32_t fontIndex = 0;
	if (styleSourceWidget == 0 ||
		styleSourceWidget >= UINTPTR_MAX - 0xB4 ||
		!SafeReadU32(reinterpret_cast<const uint32_t*>(styleSourceWidget + 0xB0), &fontIndex)) {
		ClearCurrentLayout(nowTickMs);
		return;
	}

	alignas(16) uint8_t textStyleBuf[0xB0]{};
	if (!SafeCall_InitTextStyle(
			g_Native.fnInitTextStyle,
			textStyleBuf,
			fontIndex,
			reinterpret_cast<const float*>(styleSourceWidget + 0xB4))) {
		ClearCurrentLayout(nowTickMs);
		return;
	}

	const bool            hasQuery           = !query.empty();
	const bool            searchBoxFocused   = isFocused && !isRenamingTab;
	const bool            tabRenameFocused   = isFocused && isRenamingTab;
	const bool            showSearchDropdown = searchBoxFocused && hasQuery;
	const SearchBarLayout layout             = ComputeSearchBarLayout(
		bankPanel,
		showSearchDropdown,
		matches.size(),
		selectedIndex,
		scrollOffset,
		pagePickerOpen,
		static_cast<size_t>(sharedPageCount),
		pagePickerScrollRow,
		textStyleBuf,
		personalLbl,
		gemsLbl,
		materialsLbl,
		runesLbl,
		customPersonalName,
		customSharedNames,
		tabRenameFocused,
		renamingTabIsShared,
		renamingTabPageIndex,
		tabRenameText);
	if (!layout.valid) {
		if (!g_LoggedFirstStashFrame.exchange(true, std::memory_order_acq_rel) && g_PluginCtx != nullptr) {
			uintptr_t anchor = 0;
			SafeCall_FindChild(g_Native.fnWidgetFindChild, bankPanel, "StashSearchAnchor", &anchor);
			char msg[256]{};
			std::snprintf(
				msg,
				sizeof(msg),
				"StashSearch: BankPanel::Draw active (panel=0x%llX), but layout invalid (StashSearchAnchor=0x%llX).",
				static_cast<unsigned long long>(bankPanel),
				static_cast<unsigned long long>(anchor));
			g_PluginCtx->LogWarn(msg);
		}
		ClearCurrentLayout(nowTickMs);
		return;
	}

	{
		std::lock_guard<std::mutex> lock(g_StateMutex);
		if (layout.visibleRowCount != 0 && g_ScrollOffset == scrollOffset) {
			g_ScrollOffset = layout.clampedScrollOffset;
		}
		if (layout.pagePickerOpen && g_PagePickerScrollRow == pagePickerScrollRow) {
			g_PagePickerScrollRow = layout.pagePickerScrollRow;
		}
	}

	UpdateCurrentLayout(layout, true, nowTickMs);

	if (!g_LoggedFirstStashFrame.exchange(true, std::memory_order_acq_rel) && g_PluginCtx != nullptr) {
		char msg[256]{};
		std::snprintf(
			msg,
			sizeof(msg),
			"StashSearch: rendering search box at (%d, %d, %dx%d) scale=%.3f fontIndex=%u",
			layout.searchBox.x,
			layout.searchBox.y,
			layout.searchBox.width,
			layout.searchBox.height,
			static_cast<double>(layout.scale),
			fontIndex);
		g_PluginCtx->LogInfo(msg);
	}

	const int  borderThickness = ScaleBorderThickness(2, layout.scale);
	const bool caretBlinkOn    = ((nowTickMs / 500ULL) & 1ULL) == 0;

	// Draw search box background & border with a rounded top-right corner
	{
		const bool btnHovered = layout.pageButtonRect.IsValid() && layout.pageButtonRect.Contains(cursorPt.x, cursorPt.y);
		DrawSearchBoxWithRoundedTopRight(
			layout.searchBox,
			layout.pageButtonRect,
			layout.pagePickerOpen || btnHovered,
			layout.pagePickerOpen ? kColorDropdownRowSelBg : kColorPageBtnHoverBg,
			(searchBoxFocused || layout.pagePickerOpen) ? kColorFocusedBorder : kColorUnfocusedBorder,
			layout.pagePickerOpen,
			borderThickness,
			layout.scale);
	}

	// Draw PoE-style Side Tab Flyout (docked to the right of the Stash panel)
	if (layout.pagePickerOpen && layout.pageCellCount != 0) {
		DrawFilledRect(layout.pagePickerBox, kColorDropdownRowBg);
		DrawRectBorder(layout.pagePickerBox, kColorFocusedBorder, borderThickness);
		if (layout.pagePickerDividerRect.IsValid()) {
			DrawFilledRect(layout.pagePickerDividerRect, kColorFocusedBorder);
		}
		if (layout.pagePickerHasScrollbar) {
			const bool thumbActive = (activeDragMode == ScrollbarDragMode::PagePicker) ||
									 HitTestScrollbarRect(layout.pagePickerScrollbarThumb, layout.scale, cursorPt.x, cursorPt.y);
			DrawFilledRect(layout.pagePickerScrollbarTrack, kColorScrollbarTrack);
			DrawFilledRect(layout.pagePickerScrollbarThumb, thumbActive ? kColorMatchEdgeOuter : kColorScrollbarThumb);
		}

		const int    cellPadX  = ScaleBorderThickness(14, layout.scale);
		const size_t cellCount = std::min(layout.pageCellCount, kMaxPageGridVisibleCells);
		for (size_t i = 0; i < cellCount; ++i) {
			const IntRect& cellRect = layout.pageCellRects[i];
			const uint64_t pageIdx  = layout.pageCellIndices[i];
			const bool     isShared = layout.pageCellIsShared[i];
			const bool     isActive = isShared
										  ? (activeSharedPage >= 0 && pageIdx == static_cast<uint64_t>(activeSharedPage))
										  : (pageIdx == static_cast<uint64_t>(activeParentTab));
			const bool     isHovered          = cellRect.Contains(cursorPt.x, cursorPt.y);
			const bool     isRenamingThisCell = tabRenameFocused &&
												(renamingTabIsShared == isShared) &&
												(renamingTabPageIndex == pageIdx);
			bool           hasMatchOnPage = false;
			if (hasQuery && (isShared || pageIdx == 0)) {
				for (const auto& m : matches) {
					if (m.hasTarget && m.target.isShared == isShared && m.target.pageIndex == pageIdx) {
						hasMatchOnPage = true;
						break;
					}
				}
			}

			if (isRenamingThisCell) {
				DrawFilledRect(cellRect, kColorSearchBoxBg);
				DrawRectBorder(cellRect, kColorMatchEdgeOuter, ScaleBorderThickness(2, layout.scale));
			} else {
				if (isActive || isHovered) {
					DrawFilledRect(cellRect, isActive ? kColorDropdownRowSelBg : kColorPageBtnHoverBg);
				}
				if (hasMatchOnPage || isActive) {
					DrawRectBorder(
						cellRect,
						hasMatchOnPage ? kColorMatchEdgeOuter : kColorFocusedBorder,
						ScaleBorderThickness(hasMatchOnPage ? 2 : 1, layout.scale));
				}
			}

			if (cellRect.width > cellPadX * 2) {
				const IntRect lblRect{
					cellRect.x + cellPadX,
					cellRect.y,
					cellRect.width - cellPadX * 2,
					cellRect.height
				};

				std::wstring rowLabel;
				if (isRenamingThisCell) {
					const std::wstring prefix = isShared ? (std::to_wstring(pageIdx + 1) + L" - ") : std::wstring{};
					if (imeComposing && !imeComp.empty()) {
						const size_t startIdx    = std::min<size_t>(imeStart, tabRenameText.size());
						const size_t endIdx      = std::clamp<size_t>(imeEnd, startIdx, tabRenameText.size());
						const size_t caretInComp = std::min<size_t>(imeCursor, imeComp.size());
						rowLabel.append(prefix);
						rowLabel.append(tabRenameText.substr(0, startIdx));
						if (caretBlinkOn) {
							rowLabel.append(imeComp.substr(0, caretInComp));
							rowLabel.push_back(L'|');
							rowLabel.append(imeComp.substr(caretInComp));
						} else {
							rowLabel.append(imeComp);
						}
						rowLabel.append(tabRenameText.substr(endIdx));
					} else {
						const size_t s0 = std::min<size_t>(selStart, tabRenameText.size());
						const size_t s1 = std::clamp<size_t>(selEnd, s0, tabRenameText.size());
						if (s0 < s1) {
							uint32_t w0 = 0;
							uint32_t h0 = 0;
							uint32_t w1 = 0;
							uint32_t h1 = 0;
							const std::wstring pre0 = prefix + tabRenameText.substr(0, s0);
							const std::wstring pre1 = prefix + tabRenameText.substr(0, s1);
							if (!pre0.empty()) {
								MeasureNativeText(pre0, lblRect, textStyleBuf, layout.scale, &w0, &h0);
							}
							MeasureNativeText(pre1, lblRect, textStyleBuf, layout.scale, &w1, &h1);
							const int maxRight = lblRect.x + lblRect.width;
							const int hlX0     = std::clamp(lblRect.x + static_cast<int>(w0), lblRect.x, maxRight);
							const int hlX1     = std::clamp(
								lblRect.x + static_cast<int>(w1) + ScaleBorderThickness(4, layout.scale),
								std::min(hlX0 + ScaleBorderThickness(4, layout.scale), maxRight),
								maxRight);
							const int padY = ScaleBorderThickness(5, layout.scale);
							if (hlX1 > hlX0 && cellRect.height > padY * 2) {
								const IntRect selRect{
									hlX0,
									cellRect.y + padY,
									hlX1 - hlX0,
									cellRect.height - padY * 2
								};
								DrawFilledRect(selRect, kColorTextSelectionBg);
								DrawRectBorder(selRect, kColorFocusedBorder, 1);
							}
							rowLabel = prefix + tabRenameText;
						} else if (caretBlinkOn) {
							rowLabel.reserve(prefix.size() + tabRenameText.size() + 1);
							rowLabel.append(prefix);
							rowLabel.append(tabRenameText.substr(0, s1));
							rowLabel.push_back(L'|');
							rowLabel.append(tabRenameText.substr(s1));
						} else {
							rowLabel = prefix + tabRenameText;
						}
					}
				} else if (isShared) {
					std::wstring customName;
					for (const auto& [idx, name] : customSharedNames) {
						if (idx == pageIdx) {
							customName = name;
							break;
						}
					}
					rowLabel = customName.empty()
								   ? std::to_wstring(pageIdx + 1)
								   : (std::to_wstring(pageIdx + 1) + L" - " + customName);
				} else if (pageIdx == 0) {
					rowLabel = customPersonalName.empty() ? personalLbl : customPersonalName;
				} else if (pageIdx == 2) {
					rowLabel = gemsLbl;
				} else if (pageIdx == 3) {
					rowLabel = materialsLbl;
				} else if (pageIdx == 4) {
					rowLabel = runesLbl;
				}

				if (!rowLabel.empty()) {
					DrawNativeText(rowLabel, lblRect, textStyleBuf, layout.scale);
				}
			}
		}
	}

	// Draw Search Results Dropdown (when query is active and search box is focused)
	if (showSearchDropdown && layout.visibleRowCount != 0) {
		DrawFilledRect(layout.dropdownBox, kColorDropdownRowBg);
		DrawRectBorder(layout.dropdownBox, kColorUnfocusedBorder, borderThickness);

		const size_t rows = std::min(layout.visibleRowCount, kMaxDropdownVisibleRows);
		for (size_t i = 0; i < rows; ++i) {
			const uint64_t matchIdx = layout.dropdownMatchIndices[i];
			if (matchIdx == selectedIndex && matchIdx < matches.size() && matches[matchIdx].hasTarget) {
				DrawFilledRect(layout.dropdownRows[i], kColorDropdownRowSelBg);
				break;
			}
		}
		if (layout.hasScrollbar) {
			const bool thumbActive = (activeDragMode == ScrollbarDragMode::SearchDropdown) ||
									 HitTestScrollbarRect(layout.scrollbarThumb, layout.scale, cursorPt.x, cursorPt.y);
			DrawFilledRect(layout.scrollbarTrack, kColorScrollbarTrack);
			DrawFilledRect(layout.scrollbarThumb, thumbActive ? kColorMatchEdgeOuter : kColorScrollbarThumb);
		}
	}

	// Build search box display string, visual text selection highlight, and blinking '|' caret
	std::wstring displayQuery;
	bool         shouldDrawSearchText = true;
	if (searchBoxFocused && imeComposing && !imeComp.empty()) {
		const size_t startIdx    = std::min<size_t>(imeStart, query.size());
		const size_t endIdx      = std::clamp<size_t>(imeEnd, startIdx, query.size());
		const size_t caretInComp = std::min<size_t>(imeCursor, imeComp.size());
		displayQuery.append(query.substr(0, startIdx));
		if (caretBlinkOn) {
			displayQuery.append(imeComp.substr(0, caretInComp));
			displayQuery.push_back(L'|');
			displayQuery.append(imeComp.substr(caretInComp));
		} else {
			displayQuery.append(imeComp);
		}
		displayQuery.append(query.substr(endIdx));
	} else if (hasQuery) {
		const size_t s0 = std::min<size_t>(selStart, query.size());
		const size_t s1 = std::clamp<size_t>(selEnd, s0, query.size());
		if (searchBoxFocused && s0 < s1) {
			// Render visual highlight rectangle behind selected text (e.g. Ctrl+A or Shift+Arrows)
			uint32_t w0 = 0;
			uint32_t h0 = 0;
			uint32_t w1 = 0;
			uint32_t h1 = 0;
			if (s0 > 0) {
				MeasureNativeText(query.substr(0, s0), layout.searchTextRect, textStyleBuf, layout.scale, &w0, &h0);
			}
			MeasureNativeText(query.substr(0, s1), layout.searchTextRect, textStyleBuf, layout.scale, &w1, &h1);
			const int maxRight = layout.searchTextRect.x + layout.searchTextRect.width;
			const int hlX0     = std::clamp(layout.searchTextRect.x + static_cast<int>(w0), layout.searchTextRect.x, maxRight);
			const int hlX1     = std::clamp(
                layout.searchTextRect.x + static_cast<int>(w1) + ScaleBorderThickness(4, layout.scale),
                std::min(hlX0 + ScaleBorderThickness(4, layout.scale), maxRight),
                maxRight);
			const int padY = ScaleBorderThickness(6, layout.scale);
			if (hlX1 > hlX0 && layout.searchBox.height > padY * 2) {
				const IntRect selRect{
					hlX0,
					layout.searchBox.y + padY,
					hlX1 - hlX0,
					layout.searchBox.height - padY * 2
				};
				DrawFilledRect(selRect, kColorTextSelectionBg);
				DrawRectBorder(selRect, kColorFocusedBorder, 1);
			}
			displayQuery = query;
		} else if (searchBoxFocused && caretBlinkOn) {
			displayQuery.reserve(query.size() + 1);
			displayQuery.append(query.substr(0, s1));
			displayQuery.push_back(L'|');
			displayQuery.append(query.substr(s1));
		} else {
			displayQuery = query;
		}
	} else {
		const std::wstring& ph = placeholder.empty() ? L"Search" : placeholder;
		if (searchBoxFocused && placeholderSelected) {
			// Highlight "Search" when clicked into so the user sees it accepted the click and can type right over it
			uint32_t w1 = 0;
			uint32_t h1 = 0;
			MeasureNativeText(ph, layout.searchTextRect, textStyleBuf, layout.scale, &w1, &h1);
			const int maxRight = layout.searchTextRect.x + layout.searchTextRect.width;
			const int hlX0     = layout.searchTextRect.x;
			const int hlX1     = std::clamp(
                layout.searchTextRect.x + static_cast<int>(w1) + ScaleBorderThickness(4, layout.scale),
                std::min(hlX0 + ScaleBorderThickness(4, layout.scale), maxRight),
                maxRight);
			const int padY = ScaleBorderThickness(5, layout.scale);
			if (hlX1 > hlX0 && layout.searchBox.height > padY * 2) {
				const IntRect selRect{
					hlX0,
					layout.searchBox.y + padY,
					hlX1 - hlX0,
					layout.searchBox.height - padY * 2
				};
				DrawFilledRect(selRect, kColorTextSelectionBg);
				DrawRectBorder(selRect, kColorFocusedBorder, 1);
			}
			displayQuery = ph;
		} else if (searchBoxFocused && placeholderCleared) {
			if (caretBlinkOn) {
				displayQuery = L"|";
			} else {
				shouldDrawSearchText = false;
			}
		} else {
			displayQuery = ph;
		}
	}

	if (shouldDrawSearchText) {
		if (!DrawNativeText(displayQuery, layout.searchTextRect, textStyleBuf, layout.scale)) {
			ClearCurrentLayout(nowTickMs);
			return;
		}
	}

	// Draw dropdown row labels & hover tooltip
	if (showSearchDropdown && layout.visibleRowCount != 0) {
		bool         rowTruncated[kMaxDropdownVisibleRows]{};
		const size_t rows = std::min(layout.visibleRowCount, kMaxDropdownVisibleRows);
		for (size_t i = 0; i < rows; ++i) {
			const uint64_t matchIdx = layout.dropdownMatchIndices[i];
			if (matchIdx >= matches.size()) {
				continue;
			}
			const std::wstring rowText = FormatDropdownRowWithEllipsis(
				matches[matchIdx].displayText,
				matches[matchIdx].shortTabSuffix,
				layout.dropdownTextRects[i],
				textStyleBuf,
				layout.scale,
				&rowTruncated[i]);
			if (!rowText.empty()) {
				if (!DrawNativeText(rowText, layout.dropdownTextRects[i], textStyleBuf, layout.scale)) {
					ClearCurrentLayout(nowTickMs);
					return;
				}
			}
		}
		DrawHoveredDropdownTooltip(layout, matches, rowTruncated, textStyleBuf, layout.scale);
	}

	UpdateCurrentLayout(layout, true, nowTickMs);
}

// 0x3E30: Detour for BankPanel::Draw (D2R.exe+0x23AA60)
void __fastcall Detour_BankPanelDraw(void* panel) {
	const auto     panelAddr = reinterpret_cast<uintptr_t>(panel);
	const uint64_t nowTickMs = GetTickCount64();
	if (!g_ShuttingDown.load(std::memory_order_acquire) && panelAddr != 0) {
		ProcessPendingTabAndPageSwitch(panelAddr, nowTickMs);
		ProcessPendingWheelPageStep(panelAddr, nowTickMs);
	}
	if (!SafeCall_VoidFromPtr(reinterpret_cast<uintptr_t>(g_Native.originalBankPanelDraw), panel)) {
		return;
	}
	if (g_ShuttingDown.load(std::memory_order_acquire)) {
		return;
	}
	RefreshMatchesIfNeeded(nowTickMs);
	DimNonMatchingGridItems(panelAddr);
	RenderSearchBarAndDropdown(panelAddr);
}

// 0xC5A0: Initializes plugin state, verifies D2R.exe + D2RCore.dll signatures, starts worker thread, installs hook
auto InitializePlugin(const D2RL::PluginContext* ctx) -> bool {
	if (!D2RL::HasContext(ctx)) {
		return false;
	}
	g_PluginCtx                    = ctx;
	g_Native.originalBankPanelDraw = nullptr;
	g_LoggedFirstStashFrame.store(false, std::memory_order_release);
	ResetSearchState();
	g_ShuttingDown.store(false, std::memory_order_release);
	LoadPersistedConfig(ctx);

	const HMODULE coreModule = GetModuleHandleW(L"D2RCore.dll");
	if (ctx->exeBase == 0 || coreModule == nullptr || !ResolveNativePointers(ctx->exeBase, coreModule)) {
		ctx->LogError("StashSearch: signature or D2RCore verification failed.");
		ClearResolvedPointers();
		return false;
	}

	if (g_WorkerThreadHandle == nullptr && g_WorkerReadyEvent == nullptr) {
		g_WorkerFocusRequested.store(false, std::memory_order_release);
		g_WorkerReady.store(false, std::memory_order_release);
		g_WorkerThreadId.store(0, std::memory_order_release);

		g_WorkerReadyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
		if (g_WorkerReadyEvent != nullptr) {
			g_WorkerThreadHandle = CreateThread(nullptr, 0, &InputWorkerThreadMain, nullptr, 0, nullptr);
			if (g_WorkerThreadHandle == nullptr) {
				CloseHandle(g_WorkerReadyEvent);
				g_WorkerReadyEvent = nullptr;
			} else {
				const DWORD waitRes = WaitForSingleObject(g_WorkerReadyEvent, 5000);
				if (waitRes == WAIT_OBJECT_0 &&
					g_WorkerReady.load(std::memory_order_acquire) &&
					g_WorkerThreadId.load(std::memory_order_acquire) != 0) {
					if (ctx->InstallInlineHook(
							Rva::BankPanelDraw,
							kBytes_BankPanelDraw_23AA60,
							static_cast<uint32_t>(sizeof(kBytes_BankPanelDraw_23AA60)),
							&Detour_BankPanelDraw,
							&g_Native.originalBankPanelDraw)) {
						ctx->LogInfo("Pyre Stash Search 1.0.0 initialized and BankPanel::Draw hook installed.");
						return true;
					}
					ctx->LogError("PyreStashSearch: failed to install BankPanel::Draw inline hook.");
				} else {
					ctx->LogError("PyreStashSearch: input worker thread failed to initialize.");
				}
				StopInputWorkerThread();
			}
		}
	}

	g_ShuttingDown.store(true, std::memory_order_release);
	ResetSearchState();
	ClearResolvedPointers();
	return false;
}

// Static PluginInfo descriptor
const D2RL::PluginInfo kPluginInfo = {
	.infoSize    = D2RL::PluginInfoSize,
	.abiVersion  = D2RL_PLUGIN_ABI_VERSION,
	.id          = "pyre-stashsearch",
	.name        = "Pyre Stash Search",
	.version     = "1.0.0",
	.author      = "PyreFly",
	.description = "Cross-page stash search with boolean filters, item highlighting, and an expandable tab flyout with custom tab names. Inspired by yinyin333333.",
	.flags       = D2RL::PluginFlags::Client | D2RL::PluginFlags::NativeHooks,
	.reserved    = { 0, 0, 0, 0 },
};

} // namespace
} // namespace StashSearch

extern "C" __declspec(dllexport) auto __cdecl D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
	return &StashSearch::kPluginInfo;
}

extern "C" __declspec(dllexport) auto __cdecl D2RLoaderLoadPlugin(const D2RL::PluginContext* ctx) noexcept -> bool {
	return StashSearch::InitializePlugin(ctx);
}

extern "C" __declspec(dllexport) void __cdecl D2RLoaderUnloadPlugin() noexcept {
	if (StashSearch::g_PluginCtx != nullptr) {
		StashSearch::g_PluginCtx->LogInfo("Pyre Stash Search 1.0.0 unloading.");
	}
	StashSearch::g_ShuttingDown.store(true, std::memory_order_release);
	StashSearch::StopInputWorkerThread();
	StashSearch::ResetSearchState();
	StashSearch::ClearResolvedPointers();
	StashSearch::g_PluginCtx = nullptr;
}
