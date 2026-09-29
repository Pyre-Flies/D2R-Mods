---
name: d2r-gameplay-context
description: Analyze Diablo II: Resurrected gameplay, classes, skills, items, quests, act and difficulty progression, player interaction semantics, and mod-design implications across vanilla D2R and D2R Reimagined. Use when gameplay knowledge affects advice, testing, UI behavior, or feature design; do not use for native hooks, RVAs, ABI reconstruction, or ordinary installation troubleshooting.
---

# D2R Gameplay Context

Use gameplay context to explain D2R clearly and to judge whether a mod or plugin fits how players actually progress and interact with the game. Establish the game variant before relying on mechanics: vanilla D2R, D2R Reimagined, another mod, or an explicitly compared combination.

## Keep variants separate

- Never silently apply vanilla class, skill, item, recipe, drop, difficulty, or progression assumptions to Reimagined.
- Label claims as **vanilla D2R**, **D2R Reimagined**, or **shared behavior** when the distinction matters.
- Treat balance values, skill formulas, item properties, recipes, drop locations, patch behavior, and online/ladder rules as version-sensitive. Verify current data rather than relying on memory.
- If the user's installed files or named version differ from current public documentation, identify the mismatch and answer for the requested version when evidence permits.

Read [references/vanilla-context.md](references/vanilla-context.md) for the stable vanilla framework. Read [references/reimagined-context.md](references/reimagined-context.md) whenever Reimagined is involved.

## Answer at the useful level

- For orientation, explain the gameplay loop, act or difficulty context, and why the mechanic matters before listing details.
- For classes and skills, distinguish class identity and role from current numerical balance. Verify exact prerequisites, synergies, caps, breakpoints, and formulas when they affect the answer.
- For quests, distinguish story context, progression gates, optional quests, repeatable-per-difficulty rewards, and one-time character consequences.
- For items, distinguish base type, quality, affixes, sockets, runewords, sets, uniques, crafted items, cube transformations, and mod-added systems.
- For recommendations, ask or infer the relevant mode, class/build goal, difficulty, progression point, player count, platform/control method, and mod version. State material assumptions.

## Support feature and test design

Translate gameplay into observable player behavior. Consider which surface is active, what the native action normally does, what state is authoritative, and what the player sees. Preserve native semantics unless the requested mod intentionally changes them.

Read [references/player-surfaces.md](references/player-surfaces.md) for inventory, controller, UI, and progression-aware testing. Use `$d2rloader-development` separately when implementation requires native hooks, RVAs, provider ABIs, byte guards, deployment, or release engineering.

## Verify through the right source

For current or exact claims, use the source hierarchy in [references/source-routing.md](references/source-routing.md). Prefer live Reimagined game-data pages for player-facing catalogs and calculators, the official wiki for system explanations and patch context, and the mod repository or installed data for implementation-level truth. Cite the version, page, repository revision, or installed artifact when precision matters.

Do not copy a large, fast-changing game database into this Skill. Keep this Skill as the routing and reasoning layer, and retrieve current facts only when the task needs them.
