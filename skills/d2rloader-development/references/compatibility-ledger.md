# Compatibility ledger

Use this reference whenever work discovers or consumes an RVA, address, byte signature, native layout, provider ABI, table mapping, or build-specific path.

## Find the canonical record

Search the active project for files such as `known-rvas.json`, `findings.md`, `research/`, `compatibility/`, `VALIDATION.md`, deployment manifests, or plugin-specific handoffs. Extend the closest canonical record. Do not create a competing global database when the project already owns the fact.

If no appropriate record exists, add a concise versioned research file near the affected plugin. Structured data is preferable for facts consumed by generators or tests; Markdown is appropriate for rationale, rejected hypotheses, and validation narrative.

## Minimum record for a native claim

Capture the fields that apply:

- logical name and module;
- module-relative RVA or field offset, plus whether it is a function entry, interior instruction, data address, vtable slot, or structure member;
- target build identifier, file version, size, and SHA-256;
- exact expected-byte window or another reproducible witness;
- calling convention, parameters, return value, ownership, lifetime, and thread assumptions that are actually established;
- discovery source and supporting artifact paths;
- confidence/evidence level;
- live and automated validation status;
- known limits, rejected interpretations, and follow-up questions;
- first/last verified date when useful.

Do not state an image-base address as though it were a portable RVA. Do not infer a complete structure from a few accessed fields. Preserve unknown values as unknown.

## Keep code and records aligned

- When code adds or changes a native constant, update the ledger in the same task.
- Make generators and compatibility tests consume structured records when the project already follows that pattern.
- Ensure a guard's byte window and the documented witness match exactly.
- Record superseded values and why they changed when that history prevents rediscovery; otherwise update the canonical entry cleanly.
- Keep build-specific values out of the Skill itself so updating D2R does not make the workflow instructions stale.

At handoff, name each record changed and clearly label any discovery that remains only a candidate.
