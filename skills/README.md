# Reusable D2R skills

These are versioned sources for the Codex skills used on this project. The public
checkout carries the three shared skills below. The specialist skills listed below are maintained in a separate private research
checkout, which is required for their linked evidence.

| Skill | Purpose | Source checkout |
| --- | --- | --- |
| [D2RLoader development](d2rloader-development/SKILL.md) | Native investigation, build admission, plugin coexistence, validation, deployment and release | D2R-Mods |
| [Gameplay context](d2r-gameplay-context/SKILL.md) | Vanilla, Reimagined and other packs; standalone and universal QOL design | D2R-Mods |
| [Native widget buttons](d2r-native-widget-buttons/SKILL.md) | Embedded artwork, sibling alignment, input/visual states and callback lifetime | D2R-Mods; research mirror |
| Controller softmod panels | JSON controller focus, scrolling, Back and layout-only delivery | Private research checkout |
| Native observation overlays | Owned native observations, combat correlation and overlay lifetime | Private research checkout |
| Softmod test fixtures | Temporary definitions, byte-preserving installation and safe retirement | Private research checkout |

Runtime release work uses [the development skill's release reference](d2rloader-development/references/release.md)
and [the project release procedure](../docs/RELEASING.md). Older notes naming
`d2r-mods-runtime-release` refer to this workflow; no separate installed skill is required.

An AI without Codex skill support can read these Markdown files directly. Exact
native contracts and validation results remain in [the research handoff](../docs/AI-HANDOFF.md)
and linked feature records, rather than becoming timeless skill constants.

To install, copy the complete source skill folder into the machine's Codex skills
directory. Repository edits do not auto-install. After an update, synchronize the
installed folder and any maintained research mirror, including references and UI
metadata, and compare file inventories and hashes. Resolve divergent edits before
copying. The research checkout's public submodule remains pinned; update it through its normal
submodule workflow rather than editing its nested skill copies independently.
