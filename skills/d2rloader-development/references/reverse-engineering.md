# Reverse-engineering workflow

Use this reference when locating native code, reconstructing an ABI, choosing a hook point, or interpreting runtime structures.

## Establish provenance

1. Identify the exact PE involved: `D2R.exe`, `D2RCore.dll`, a D2RLoader provider, or another loaded module.
2. Record file version, size, SHA-256, architecture, and relevant loader/provider version when available.
3. Distinguish analysis images, installed files, deployed copies, and source-generated artifacts. Confirm which one the process loads.
4. Search existing research, known-RVA files, disassemblies, logs, probes, and compatibility tests before rescanning.

## Promote claims carefully

Track discoveries with an explicit evidence level:

- **Candidate:** pattern, cross-reference, decompiler interpretation, or semantic resemblance.
- **Static witness:** exact bytes/instructions establish a bounded property for a named artifact.
- **Runtime observed:** logging or inspection confirms the path executes and the observed fields behave as described.
- **Live validated:** the user-visible behavior and relevant authoritative state were checked in game.

One level does not imply the next. Function boundaries, calling conventions, unwind safety, object lifetime, thread affinity, full structures, and multiplayer behavior each need their own evidence.

## Prefer stable integration points

- Start from public SDK/provider services when they express the required operation.
- When native work is necessary, hook the earliest synchronous consumer that can suppress or extend the behavior without undoing already-applied state.
- Preserve raw observations needed by the plugin even when consuming the competing game action.
- Prefer module-relative RVAs over absolute virtual addresses. Never reuse an RVA without validating its artifact and expected bytes.
- Copy transient message/payload data only after proving size and lifetime; validate pointers and ranges before dereferencing.
- Perform UI-affecting inspection or mutation on the required game/UI thread.

## Shared hooks and mutable state

- Inventory overlapping entry points, call sites, byte ranges, vtable/provider slots and state writes across our plugins and relevant companions before choosing a patch. Prefer supported listeners or a proven non-overlapping caller; moving a hook still requires evidence for equivalent semantics.
- Separate immutable artifact/code witnesses from legitimately mutable runtime values. Validate mutable state by its established type, bounds, identity and lifecycle; do not demand a historical value another plugin may validly change. For temporary overrides, use the proven ownership protocol and restore only while the value remains ours.
- An already-modified entry is a compatibility investigation, not automatic permission to overwrite it. Recognize predecessor patches only through bounded contracts: admitted artifact/code shape, executable destination ownership, established ABI, relocated instruction behavior and callable lifetime. An executable address alone is insufficient.
- Where composition is proven, preserve arguments, return value, required registers/flags/SIMD and stack state; call the appropriate predecessor once unless a deliberately scoped consumed action requires otherwise. Prevent recursion and duplicate effects. Do not bypass another plugin by jumping to a remembered pristine entry.
- Publish against the expected current bytes or slot and verify ownership before restoration, using an atomic exchange where the proven protocol requires it. Avoid overwriting intervening changes. Partial setup rolls back only owned changes.
- A later hook may retain our adapter or trampoline. Do not free callable storage or unload its code without a proven detach/lifetime contract; use the supported ownership mechanism or remain safely inert when required.
- Without a valid chaining contract, prefer a different proven integration point, a narrowly documented known companion contract, or feature-local refusal. Log the conflict clearly and preserve the game and other plugin behavior.

## Design the proof

Choose one discriminating outcome and log enough context to explain both success and refusal. A useful probe should:

- avoid mutating unrelated state;
- refuse unsupported builds or ambiguous targets;
- identify the path taken and the guard that admitted it;
- expose a quick user-visible pass/fail action;
- cleanly unload or remain inert if the experiment fails.

Archive the minimal disassembly, byte window, runtime log, or generated evidence needed to reproduce the conclusion. Do not archive secrets, personal paths unnecessarily, or huge undifferentiated dumps.
