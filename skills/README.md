# Reusable D2R skills

These are the versioned source copies of the personal Codex skills used while
working on this repository. Each `SKILL.md` describes when to apply the skill;
its `references/` files contain the procedure and source-routing detail. The
`agents/openai.yaml` files provide optional Codex display metadata.

| Skill | Use |
|---|---|
| [d2rloader-development](d2rloader-development/SKILL.md) | Native plugin investigation, build guards, ABI checks, validation, deployment and packaging |
| [d2r-gameplay-context](d2r-gameplay-context/SKILL.md) | Gameplay and player-facing feature reasoning across vanilla D2R and Reimagined |

An AI system without Codex skill support can read the Markdown directly. These
files give process and source priorities; the current native contracts and
validation results live in [the research handoff](../docs/AI-HANDOFF.md) and its
linked feature records. Verify volatile game and mod facts against the target
version before using them.

To install a skill locally in Codex, copy its directory to the local Codex
skills directory. The repository copy does not auto-install or change an
existing personal skill.
