# D2R Mods release 2026.10.12.2

| Plugin | Version |
| --- | --- |
| Controller QOL Updates | 1.3.1+rev.76 |
| Item Roll Ranges | 1.3.1+rev.34 |
| Map Assistance | 1.3.1+rev.1 |
| Inventory Deposit | 0.1.0-beta.13 |
| Floating Damage | 0.1.0-beta.12 |

## Item Roll Ranges

Expand proven overlapping host affixes, socketed jewels/runes/gems and completed
corruption bonuses into a combined total followed by gray source rows. Socket
sources use insertion order; corruption bonuses use [C]. Individual rolls stay
unknown when a saved combined value cannot uniquely identify them. Enchantments
remain deferred; unsupported or ambiguous source operations retain native text.

Show compact fixed modifier bounds such as [+38], including Loremaster and
fixed speed/proc prefixes. Preserve explicit Base endpoints on Defense and
physical-damage headers, with normal, generated-ED, ethereal and supported
low-quality handling. Use the active level cap for verified scaling affixes,
and distinguish property/coefficient families even when affix names repeat.

Restore ranges on identified items without a native range clone. Correct the
observed extended stat-list flags, Enhanced Defense handler and flat Thorns
fallback. Godly/Silver-Wrought and compatible Eagleeye/Trump overlaps receive
source breakdowns with full bounds. Chronicle-hidden unique/set definition
support and the compact [U] marker remain included; set labels are unchanged.

Socketed items show Max Sockets in the normal header position while ranges are
held. Weapons retain Required Level / Max Sockets / Attack Speed order. Native
Ctrl and controller range queries remain untouched; release restores ordinary
tooltips. No Item SDK, gameplay writes or new input interception is added.

Seven automated suites pass. User checks confirm the reviewed socket/corruption
breakdowns, Defense headers, Shadow Mantle's ED/Thorns and corrected socket
header placement. Every item family, localization, controller combination and
unsupported operation has not been validated. Please report missing or incorrect
ranges with the item tooltip, affixes and active mod version.

## Controller QOL rev.76

Publish the reviewed rev.76 beta.4 implementation as rev.76, without a beta
version suffix. Retain ordinary-chest priority defaults, guarded Cube
self-insertion prevention, overlapping LT/RT Shared-page ownership, serialized
shortcut routing and audited local/guest loot handling. Existing explicit
settings are preserved. Includes native Mod Information bumper navigation and
stash-tab feedback for bulk deposits.

All 33 automated suites pass. The user confirmed Cube self-insertion prevention,
Shared Stash escape prevention and Cube behavior through rejoins, and accepted
non-online gameplay. Online qualification and some navigation/feedback coverage
remain open; review the included audit evidence. Please report bugs with the
input sequence, active storage page, mod and local/hosted/remote context.

## Other included packages

Floating Damage beta.12 retains viewport scaling and critical-style choices
from the previous release. Inventory Deposit beta.13 and Map Assistance rev.1
are included at their existing versions. Floating Damage remains a local-player
HP-loss overlay; resolution/critical-style visual coverage is limited. Inventory
Deposit's broader remote/resolution/persistence coverage remains incomplete.

## Install

Close the game and loader, back up installed DLLs and extract only the wanted
plugin ZIPs into the game directory or active mod directory. Keep one active
copy per plugin. DLLs install under d2rloader/plugins/. Reference TOMLs are under
defaults/, never live configuration paths; preserve customized settings. Item
Roll Ranges requires Core item stat ranges enabled. A cold restart is required.
Each ZIP has internal SHA256SUMS; the release's SHA256SUMS covers all five ZIPs.
