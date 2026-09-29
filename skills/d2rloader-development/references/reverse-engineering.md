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

## Design the proof

Choose one discriminating outcome and log enough context to explain both success and refusal. A useful probe should:

- avoid mutating unrelated state;
- refuse unsupported builds or ambiguous targets;
- identify the path taken and the guard that admitted it;
- expose a quick user-visible pass/fail action;
- cleanly unload or remain inert if the experiment fails.

Archive the minimal disassembly, byte window, runtime log, or generated evidence needed to reproduce the conclusion. Do not archive secrets, personal paths unnecessarily, or huge undifferentiated dumps.
