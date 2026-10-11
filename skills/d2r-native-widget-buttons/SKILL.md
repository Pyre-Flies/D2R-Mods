---
name: d2r-native-widget-buttons
description: Add, align, or debug native D2RLoader inventory buttons with DLL-embedded artwork, sibling-relative placement, and correct input and visual states. Use for native panel child widgets and their lifecycle; not for general overlay frameworks, gameplay rules, or ordinary mod installation.
---

# D2R Native Widget Buttons

Use the Inventory Deposit implementation as a worked example of a plugin-owned
native button. Keep the requested action separate from its presentation; this
skill does not authorize a release or changes to other plugins.

## Locate the implementation

Resolve the user's D2R-Mods checkout first. Do not interpret repository paths
relative to this installed skill directory. Verify that the selected checkout
contains `plugins/inventory-deposit` and its pinned PluginSDK. Read its
`AGENTS.md`, the affected plugin's current README, and only the relevant entries
in [references/source-map.md](references/source-map.md).

If the checkout is unavailable, the principles below remain useful for design,
but obtain the target SDK and build evidence before implementing native calls.
Current guard headers and canonical research records outrank remembered RVAs
or historical deployment notes.

## Extend a panel without replacing it

- Prefer `PanelService::registerChildLayout` where the target SDK supports the
  stock panel. Embed a strict JSON layout and original sprites as DLL resources;
  register them through `ResourceService` in a unique plugin namespace. A
  companion MPQ is unnecessary for resources served this way.
- Choose the input-mode flag deliberately. Inventory Deposit uses
  `KeyboardMouseOnly`; the demonstrated child API targets HD inventory layouts,
  not legacy non-HD UI. An injected child does not claim the single-owner custom
  inventory-page/controller-route extension used by other plugins.
- Start the child hidden. Register resources, the child, and exact namespaced
  message routing with rollback for partial registration. An action payload
  masquerading as `PanelManager:OpenPanel` must be consumed only on an exact
  target/command/payload match, including when the action is unavailable.
- Use the SDK UI-thread queue for widget work. Coalesce repeated refreshes and
  clicks, invalidate queued callbacks on session changes and unload, and
  revalidate context and action availability when a queued click executes.
  Do not hold a plugin lock across unregister calls that may wait for callbacks.
- Resource re-registration is not a proven live layout reload. Already-created
  native children can remain cached; use a game restart for layout/art changes.
  Unregistering ownership does not prove destruction of cached widgets.

## Keep visibility, input, and artwork separate

Visibility answers whether this context should display the button. Enablement
answers whether a new request may start. Holding an item, an empty eligible
inventory, or a busy batch can disable an otherwise visible button. Keep batch
continuation independent from the new-request predicate: a running batch must
not cancel itself because it is busy or has removed its last eligible item.

In the documented D2R build, SDK `setWidgetEnabled` changes the input byte only.
The native `ButtonWidget` has a separate disabled flag and frame transition.
Declaring `disabledFrame` or rearranging visibility calls does not bridge them.
Read `BUTTON-STATE.md` before extending this adapter; do not generalize the
limitation to a newer SDK without checking its implementation.

Use the proven native enable/disable transitions, with their supporting guards
and read-back, for this admitted widget type. Do not replace that operation with
an arbitrary frame or tint write: native transitions also handle pressed/hover
restoration and embedded text positioning. If binding or state application
fails, hide/disable the optional child and refuse its action. A return to the
enabled state must never submit an action without a fresh click.

## Anchor in the correct coordinate space

Read a live sibling rectangle, such as `gold_amount`, through the SDK. Compute
placement in their shared parent's local units; a screenshot's pixels are not
layout coordinates. The demonstrated policy places x at the sibling's right
edge plus a gap and vertically centers the current button height plus an offset.

The pinned SDK has a rectangle getter but no position setter; native layout
parent anchors do not establish a sibling-name anchor field. Use a supported
setter if the target SDK supplies one. Otherwise follow the narrowly guarded
native adapter in `GOLD-ANCHOR.md`, moving only the plugin-owned child. Bind SDK
paths to fresh native objects by name, parent, type, and matching rectangles;
an SDK handle is not a native pointer. Verify the actual widget's hit-test
implementation consumes the moved geometry. Recheck before click dispatch.

Keep native calls behind the target artifact identity, complete adopted code
witnesses, bounded child lookup, object/layout checks, and excluded diagnostic
branches. Never retain native widget pointers across callbacks. Record any new
RVA, byte guard, field, ABI, or rejected hypothesis in the canonical project
record with provenance and validation limits.

## Build and verify the artwork

Read `BUTTON-SPRITES.md` before changing the codec or resource bindings. Keep
normal, hover, pressed, and disabled art as explicit source images; conversion
must not invent missing interaction states. Generate the HD/half-size low-end
pair into the build tree and embed both. Keep frame indices, resource IDs,
virtual filenames, logical dimensions, and click bounds consistent.

Inspect decoded native-size frames and alpha bounds when fitting artwork to a
stone lip: transparent margins can make correct logical bounds look too small.
Measure against the user's current screenshot and confirm the final fit live.

Validate codec round trips, actual DLL resources/ABI/exports, callback wiring,
state transitions, queued-click refusal, stale sessions, and failure paths.
Mocks must represent separate input and artwork states. Isolated execution of
guarded native bodies can establish ABI and mutation footprint, but cannot
prove real widget lifetime or rendering. Report automated evidence separately
from exact-DLL live appearance, hover/press, alignment, input-mode, and low-end
checks. Before diagnosing an apparent regression, verify the deployed DLL hash
and its load log; a source build alone does not identify the running artifact.
