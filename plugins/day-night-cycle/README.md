# Day Night Cycle

Restores the game's native time-of-day lighting updates during normal gameplay,
without enabling debug mode. It uses the game's existing clock and timing.

Version **1.3.1+rev.1**, for the reviewed D2R **3.3.93787** and
D2RLoader **1.3.1-beta** build. Unsupported or altered builds retain native
behavior and log a warning. Other builds need separate qualification.

## Install

Close the game, then copy the runtime archive's `d2rloader/` folder into either
the game directory or your active mod directory. Use one installation location.
Start through D2RLoader. Its embedded default creates
`d2rloader/config/day-night-cycle.toml` when missing; the runtime archive also
includes a reference under `defaults/`. Preserve existing configuration.

To remove it, close the game and delete `Day Night Cycle.dll` from that location's
`d2rloader/plugins/` folder. Restart before replacing the DLL.

## Chat controls

Enter these in normal game chat:

- `/daynight` or `/daynight toggle`: toggle between the natural cycle and daylight.
- `/daynight on` or `/daynight auto`: resume the current natural time.
- `/daynight off`: stop updates and return immediately to daylight.
- `/daynight day` and `/daynight night`: hold noon or night for instant comparison.
- `/daynight status`: sample current renderer values; `/daynight help`: show usage.

Changes apply on the next UI update and save `enabled` and `mode` under
`[day-night-cycle]` in the active TOML. The natural clock keeps running while
lighting is held; `auto` returns to its current time. Defaults are enabled with
mode `auto`. No cycle-speed control is included.

The active configuration uses these defaults:

```toml
[day-night-cycle]
enabled = true
mode = "auto"
```

`mode` accepts `auto`, `day` or `night`. Prefer the chat controls while playing;
they save the active file. For manual edits, close the game, edit that file and
restart. Files under `defaults/` are references rather than active settings.

This is entirely client-side: no server component or network message is needed.
Fixed lighting does not change server time, quests or other players' views.

Replies appear in the loader console and plugin log. Status includes the actual
renderer stage, blend and override flag, plus whether those values changed since
the previous status sample. Failed saves attempt to restore the previous renderer
and enabled state. Another owner's manual time override is preserved and the
requested change is refused. With global debug mode enabled, the loader can
resume its own updates even when this plugin is off; use `day` to hold daylight.

## Test status

Five automated suites pass: native machine-code replay, code/binding/ownership
guards, profile consistency, loader ABI/resources and actual command dispatch
through a substituted parser/SDK UI queue. Tests cover the real setter's day/night
writes, daylight on off, TOML preservation/restart, rollback, later ownership,
queued shutdown and an unchanged global debug flag.

The user confirmed visible natural cycling on beta.2; its loaded DLL, normal-mode
flags, changing renderer values and chat dispatch were also verified. The release preserves beta.3 behavior; instant controls still need in-game testing. Area changes and hosted behavior
remain unqualified. Source includes the developer guide and compatibility record.
