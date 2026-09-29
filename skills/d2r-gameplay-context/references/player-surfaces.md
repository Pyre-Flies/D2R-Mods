# Player surfaces and native semantics

Use this reference when gameplay context informs a plugin, UI change, controller mapping, or live test.

## Identify the active surface

Do not treat “inventory” as one universal context. Distinguish:

- player inventory;
- equipment and alternate weapon set;
- belt;
- Horadric Cube;
- personal stash;
- each shared or mod-added stash page;
- mercenary inventory;
- vendor buy/sell/repair/gamble panels;
- trade window;
- ground items and labels;
- item-on-cursor state;
- skill tree and skill binding surfaces;
- quest log, waypoints, automap, menus, and NPC dialogue.

The same click, controller button, modifier, or item operation may have different meaning by surface. Identify focus, cursor state, open panels, ownership, target container/page, and competing native actions before designing or testing a feature.

## Preserve expected behavior

- Reuse the native interaction path when extending quick transfer, identification, navigation, tooltip display, or item movement.
- Keep mouse and controller semantics scoped. A mapping that is helpful in stash may conflict with skill use, ground pickup, vendor actions, or menu navigation elsewhere.
- Preserve native feedback: prompts, glyphs, sounds, tooltip values, refusal behavior, and focus changes unless the feature intentionally replaces them.
- Do not infer a mod-added tab's identity from its visible order alone; validate raw page/container identity.

## Gameplay-aware validation

Choose a test that a player can understand and reverse:

- state the character, difficulty, act, quest state, and relevant mod version;
- use low-value items and preserve a backup when save state could be affected;
- test the smallest supported surface first, then adjacent surfaces explicitly;
- compare authoritative state with visible UI state;
- check persistence after save/reload when the feature changes lasting state;
- verify unrelated native actions still work;
- record mod-specific exceptions separately from vanilla behavior.

If implementation reaches native code, providers, transactions, DLL deployment, or build-specific layouts, switch to `$d2rloader-development` for those portions of the task.
