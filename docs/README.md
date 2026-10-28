# Documentation map

Start with the [repository README](../README.md) for current plugins and downloads.
Agents start with [AGENTS.md](../AGENTS.md) and one affected developer guide.

| Work | Read |
| --- | --- |
| Controller QOL | [Developer guide](../plugins/controller-qol/DEVELOPMENT.md), [topic map](../plugins/controller-qol/docs/README.md) |
| Ultrawide | [Player & config guide](../plugins/ultrawide/README.md), [native contract](../plugins/ultrawide/NATIVE-CONTRACT.md) |
| Item Roll Ranges | [Developer guide](../plugins/item-roll-ranges/DEVELOPMENT.md), [topic map](../plugins/item-roll-ranges/docs/README.md) |
| Floating Damage | [Developer guide](../plugins/floating-damage/DEVELOPMENT.md) |
| Always Show Sockets | [Developer guide](../plugins/always-show-sockets/DEVELOPMENT.md) |
| Inventory Deposit | [Developer guide](../plugins/inventory-deposit/DEVELOPMENT.md) |
| Map Assistance | [Developer guide](../plugins/map-assistance/DEVELOPMENT.md) |
| Gem Bag UI | [Developer guide](../plugins/gem-bag-ui/DEVELOPMENT.md), [deposit contract](../plugins/gem-bag-ui/DEPOSITS.md) |
| Day Night Cycle | [Developer guide](../plugins/day-night-cycle/DEVELOPMENT.md), [native contract](../plugins/day-night-cycle/NATIVE-CONTRACT.md) |
| Pyre Loot Filter | [Player & feature guide](../plugins/pyre-lootfilter/README.md), [native registry](reverse-engineering/README.md) |
| Build or contribute | [CONTRIBUTING.md](../CONTRIBUTING.md) |
| Local folders and cleanup | [Workspace layout](WORKSPACE.md) |
| Package or publish | [Release process](RELEASING.md), [companion sources](COMPANION-SOURCES.md) |
| Change a native hook or layout | [Native registry](reverse-engineering/README.md), then the owning feature record |
| Change player documentation | [README convention](README-GUIDE.md) |
| Shared saved chat settings | [Settings contract](CHAT-COMMAND-SETTINGS.md) |

Each plugin has one player README, one changelog and one developer entry point.
Native evidence remains beside its owning feature. Dated/revision records and
superseded guides live in `docs/history/` under the repository or plugin.
Read history for a specific investigation; do not use its version labels as current status.

Default ripgrep searches skip history. Use an explicit file path or
`rg --no-ignore <term> plugins/<plugin>/docs/history` when historical evidence is needed.
