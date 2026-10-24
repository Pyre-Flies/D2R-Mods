# Gem Bag UI

Withdraw gems directly from a compatible Gem Bag, or deposit eligible gems and
gem clusters with Shift + right-click or LB+B. Version **1.3.1+rev.2** supports
mouse, keyboard and controller use with D2RLoader.

## Controls

| Action | Control |
| --- | --- |
| Open the gem picker | Right-click the bag, or focus it and press LB+A. |
| Choose a gem | Click its icon, use arrows and Enter, or D-pad and A. |
| Close the picker | X, Escape, B, right-click or click outside. |
| Deposit compatible gems and clusters | Shift + right-click the bag, or focus it and press LB+B. |

Each withdrawal spends one bag credit and creates one identified gem in a free
cell beside the bag, in the same inventory, Cube, personal stash or normal
shared-stash page. The bag stays in place. No physical grabber is required;
existing Cube recipes remain available. Holding a button does not repeatedly
withdraw gems. Gem names follow the game's language and mod definitions.

Deposits collect eligible items from the bag's current storage page and use
the mod's existing recipe values, including random cluster values. An input
stack that would exceed the credit limit stays in place. Shared-stash deposits
need enough free space in normal inventory for one whole input stack at a time.

## Install and update

Close the game and back up the previous DLL. Extract this ZIP into the game
directory or active mod directory so `Gem Bag UI.dll` is under
`d2rloader/plugins/`. Keep one active copy and restart through D2RLoader.
No MPQ, table edits or configuration file is required.

**Hosted games require this same version on both client and host**, with the
same active mod data. The server validates and performs item changes. A client
installation alone cannot enable withdrawals or deposits on an unsupported host.

To uninstall, close the game and remove the DLL. Gems and bag credits remain
ordinary game save data; the plugin does not introduce a separate save format.

## Compatibility and support

Requires the reviewed D2R/D2RLoader 1.3.1 build family and compatible Gem Bag
recipes. It discovers supported recipes from the active mod; it does not add
bags or recipes to vanilla Diablo II. Unknown native builds, unsupported bag
definitions, a moved bag, an occupied cursor or a full destination refuse the
action. Leave the game's ordinary recipes available as a fallback.

Live testing covers mouse/controller withdrawals and loose-gem shared-stash
deposits, including a hosted save/rejoin. Shared cluster staging and
full-inventory refusal have automated coverage but still need live testing.
The release carries forward beta.19 behavior with release version metadata.

Check `d2rloader/logs/gem-bag-ui.log` when reporting a problem. Include the
plugin, game and mod versions, storage location and reproduction steps.
See [development](DEVELOPMENT.md), [compatibility](NATIVE-CONTRACT.md),
[deposit details](DEPOSITS.md) and [changelog](CHANGELOG.md).
