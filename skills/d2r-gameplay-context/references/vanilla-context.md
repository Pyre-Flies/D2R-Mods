# Vanilla D2R framework

Use this reference for stable orientation. Verify current patch-specific values, ladder availability, balance, and exact formulas before presenting them as current.

## Core loop

D2R is an action RPG built around defeating monsters, gaining levels and skill points, improving equipment, completing quests, and advancing through acts and difficulties. Character power comes from interacting systems: attributes, skills and synergies, equipment, resistances, attack/cast recovery breakpoints, mercenaries, charms, socketables, and difficulty penalties.

The seven established player classes are Amazon, Assassin, Barbarian, Druid, Necromancer, Paladin, and Sorceress. Each has three skill trees. Class identity is stable, but viable builds and exact skill behavior can change with patches and are substantially different in mods.

## Story and act progression

- **Act I — The Sightless Eye:** aid the Rogues and pursue the Dark Wanderer; defeating Andariel opens the journey east.
- **Act II — The Secret of the Vizjerei:** assemble the Horadric Staff, find Tal Rasha's Tomb, and confront Duriel.
- **Act III — The Infernal Gate:** recover Khalim's relics, break the Compelling Orb, and defeat Mephisto.
- **Act IV — The Harrowing:** enter Hell, resolve Izual's fate, destroy Mephisto's Soulstone, and defeat Diablo.
- **Act V — Lord of Destruction:** defend Harrogath, pursue Baal through Mount Arreat, and defeat him in the Worldstone Chamber.

Completing the campaign advances through Normal, Nightmare, and Hell. Each difficulty repeats the acts and quests with stronger enemies, harsher resistance conditions, higher-level drops, and another opportunity for most quest rewards. Act access and quest-credit details can differ when rushing, joining another player's game, or using modded progression; verify them when they are central to the answer.

## Quest rewards that often affect design

Important vanilla rewards include skill points, Akara's per-difficulty respecialization, permanent life or attribute gains, resistance rewards, Larzuk socketing, personalization, rune rewards, mercenary access, and item rewards. For exact reward amounts, eligibility, repeatability, or mod behavior, consult a current authoritative source.

When designing a feature, do not treat all quests alike:

- some gate act or campaign progression;
- some unlock services or travel;
- some permanently modify the character;
- some grant an item that can be delayed, saved, or used strategically;
- some are optional and commonly skipped during efficient progression.

## Item model

Keep these concepts distinct:

- base type and normal/exceptional/elite tier;
- item quality such as low-quality, normal, superior, magic, rare, set, unique, or crafted;
- affixes versus fixed unique/set/runeword properties;
- sockets and inserted gems, jewels, or runes;
- ethereal, durability, quantity, stack, charge, and personalized state;
- inventory footprint versus equipped slot and container position;
- displayed tooltip text versus compiled table data and runtime state.

Runewords require the right non-magical base category, exact socket count, and rune order. Mods can change any of these assumptions, so verify Reimagined recipes and bases independently.
