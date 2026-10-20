# D2R Mods - 2026.10.20.1

- **Controller QOL rev.81:** Standardizes the tested beta.7 controller and
  opacity fixes, including Cube controls and filtered-loot pickup behavior.
- **Buff HUD 1.1.1:** Keeps buff icons, countdowns, and empty slots consistent
  when switching between controller and keyboard/mouse. Includes the required
  companion MPQ. This fork contains the fix submitted in upstream PR #8.
- **UnHoarder 1.2.9:** Includes the current fork release with controller labels,
  background opacity, drop alerts, sound routing, and shared hidden-loot state.
- **Item Roll Ranges rev.55, Floating Damage rev.13, Inventory Deposit rev.13,
  Always Show Sockets rev.2, and Map Assistance rev.1:** Retain their current
  released behavior in this consolidated update.

Install one copy of each plugin and close the game before replacing DLLs.
Existing settings and filters are preserved; packaged defaults are references.
Buff HUD requires both its DLL and MPQ. The UnHoarder Reimagined package also
updates its sound tables; retain your existing filter.json.

Clean builds and automated checks are verified. This release carries forward
previously tested behavior; it does not add new live-game or hosted coverage.
Experimental and diagnostic plugins are maintained separately in research.
