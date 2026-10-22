# D2R Mods - 2026.10.22.1

- **Controller QOL rev.91:** Fixes Arcane Sanctuary red teleporters and Act 2/3
  quest chests (including Khalim's chests) so they can be reliably used with the
  controller `A` button near ground items. Skill points on the Skill Tree now
  allocate on a single `A` press instead of requiring a hold. Also speeds up
  ground-loot label and held-shortcut checks alongside UnHoarder.
- **UnHoarder 1.2.16:** Improves loot-label performance and avoids unnecessary
  inventory scans. Hold keyboard ALT to pick up filter-hidden items with the mouse.
- **Gem Bag UI rev.1:** Adds a mouse/controller gem picker and Shift + right-click
  deposits for compatible gems and clusters. Install the same version on client
  and host for hosted games; shared-stash deposits need free inventory space.
- **Day Night Cycle rev.1:** Restores natural day/night lighting and adds saved
  controls for natural, daytime or nighttime lighting. Runs entirely on the client.
- The other plugins retain their current released behavior.

Close the game before replacing DLLs and keep one active copy of each plugin.
Existing settings and filters are preserved; packaged defaults are references.
Buff HUD needs both its DLL and MPQ. The UnHoarder Reimagined ZIP includes its
sound tables; retain your existing filter.json.

Automated checks pass. Red portal and quest-chest interactions, single-press skill
allocation, and ground-loot performance were user-tested in-game. The plugin guides
describe remaining live-test limits, including fixed lighting controls and broader
pickup/hosted coverage.
