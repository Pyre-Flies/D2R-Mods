# D2R Controller QoL Plugin (`controller-qol`)

A native C++ plugin for **Diablo II: Resurrected** (loaded via **D2RLoader** / **Reimagined Launcher**) that introduces **Project Diablo 2 (PD2)-style controller shortcuts and quality-of-life features**.

---

## Features

### 1. Instant Quick Identify (`pSpell = 1`)
* **Shortcut:** Hold **Left Trigger (LT)** + Press **A** (Xbox) / **Cross** (PlayStation) while hovering over an unidentified item in your inventory or cube.
* **Behavior:**
  * Checks for a Tome of Identify (`ibk`) or Scroll of Identify (`isc`) in your inventory.
  * Consumes 1 scroll/charge and immediately identifies the item in 1 frame.
  * Plays the identification sound effect and reveals affixes instantly without requiring the tedious "Hold to Use -> Aim -> Press" workflow.

### 2. Quick Transfer (Stash / Cube)
* **Shortcut:** Hold **Left Trigger (LT)** + Press **X** (Xbox) / **Square** (PlayStation).
* **Behavior:** Instantly moves the hovered item between Player Inventory and Stash (or Horadric Cube if open).

### 3. Quick Drop
* **Shortcut:** Hold **Left Trigger (LT)** + Press **Y** (Xbox) / **Triangle** (PlayStation).
* **Behavior:** Immediately drops the hovered item onto the ground.

### 4. Feed Mercenary Potion
* **Shortcut:** Hold **Left Trigger (LT)** + Press **D-Pad (Left / Up / Down / Right)**.
* **Behavior:** Directly feeds the potion in the corresponding belt slot to your active mercenary instead of your character.

### 5. Belt Auto-Restock
* **Shortcut:** Press **Right Thumbstick Click (R3)**.
* **Behavior:** Automatically pulls matching potions from your inventory into any empty slots in your equipped belt.

---

## Shortcut Reference Table

| Button Chord | Context | Action |
| :--- | :--- | :--- |
| **LT + A** (Xbox) / **LT + ✕** (PS) | Hovering unidentified item | **Instant Quick-Identify** |
| **LT + X** (Xbox) / **LT + ◻** (PS) | Hovering item (Stash/Cube open) | **Quick Transfer to Stash / Cube** |
| **LT + Y** (Xbox) / **LT + △** (PS) | Hovering item | **Quick Drop** |
| **LT + D-Pad** | Anywhere in game | **Feed Belt Potion to Mercenary** |
| **R3** (Right Stick Click) | Anywhere in game | **Auto-Restock Belt Potions** |

---

## Installation

1. Build the plugin (or use the precompiled binary in `build/Release/controller-qol.dll`).
2. Copy `controller-qol.dll` into your mod's plugin directory:
   ```
   <Diablo II Resurrected>/mods/Reimagined/d2rloader/plugins/controller-qol.dll
   ```
3. Launch the game using **D2RLoader** or the **Reimagined Launcher**.

---

## Building from Source

### Requirements
* Windows 10/11 64-bit
* CMake 3.20 or newer
* Visual Studio 2022 (MSVC v143 or newer with C++20 support)
* Windows SDK (10.0.19041.0 or newer)

### Build Steps
```powershell
# From the d2r-reimagined-mod workspace root:
cmake -S "plugins\controller-qol" -B "plugins\controller-qol\build" -A x64
cmake --build "plugins\controller-qol\build" --config Release
```

The compiled binary will be generated at:
```
plugins/controller-qol/build/Release/controller-qol.dll
```
