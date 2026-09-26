# Controller QOL Updates 1.3.1+rev.5 — production release

Author: PyreFly. Qualified loader: D2RLoader 1.3.1-beta. Released 2026-09-24.

The exact tested DLL is promoted without rebuilding or behavior changes. SHA-256:
`E21E894B0B32BC37B9BB3F495737B7F032260FDF8B59741F3FAFD43B3CD3ECC0`.

## Validation

User reported successful gameplay testing for all three configurations, each with Stash Search and Potion Auto Pickup enabled:

| Platform | Controller | Result |
|---|---|---|
| Battle.net, Steam closed | DualShock | User-confirmed pass |
| Steam | Steam Controller | User-confirmed pass |
| Steam | DualShock | User-confirmed pass |

All eleven automated suites passed before deployment. The final native code witnesses and manager/input/index pointers were independently checked against the running Battle.net process. Both installed DLL hashes match this release. No separate Xbox hardware test is claimed, nor exhaustive validation of every shortcut, load order, reconnect sequence or future third-party plugin.

## Changes

Uses the game's normalized controller events for QOL input instead of requiring physical XInput. SDK-managed hooks observe native key transitions and resets; SDK game-thread tasks maintain snapshots and timed navigation. XInput hooks are installed only if native admission fails. Existing Stash Search and Potion Auto Pickup cooperation is retained. Debug and portal diagnostic logging default to false.

## Installation

Close the game and loader. Extract the DLL to `d2rloader/plugins`. For a new installation, use the supplied `d2rloader/config/controller-qol-updates.toml`; keep existing settings when upgrading. Restart the loader. Keep the previous DLL as a rollback outside the plugins directory.

## Remaining risk

This remains a private native integration, not a public controller SDK contract. An exact Core hash and instruction/slot checks reject unqualified builds before the native bridge activates. Even an unrelated Core rebuild can require review and a new qualification; do not merely substitute a new hash. Rejection falls back to XInput, whose coverage may exclude a directly connected DualShock without translation. This protects against known mismatches but is not a guarantee against every crash or behavioral regression.

The two new shared hook sites are game RVA `0x13EDD0` (key events) and `0x13DEF0` (input reset). Other plugins editing these functions may conflict or become load-order dependent. QOL does not replace shared Core input slots and leaves the held-key query and shared text renderer entries untouched. Digital trigger thresholds follow the game; raw analog threshold configuration applies to XInput fallback. Disconnect/focus/timing edge cases remain useful regression tests.

After any loader/game patch, qualify the new binaries, revalidate the documented event/reset path and code witnesses, run the suites, and repeat the controller/plugin matrix. A loader-owned ordered controller-input API remains the long-term route to reducing these dependencies.

See `NATIVE-INPUT-1.3.1-rev.5.md` for evidence, RVAs, ABI contracts and recovery steps. Earlier golden snapshots remain preserved.
