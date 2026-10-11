# Native chat support provenance

These four user-owned source/header files were copied from the sibling
`d2rl-plugins` repository's `libs/chat-sdk` and `libs/image-scan`, 2026-10-11.
The sibling source baseline is commit
`478b6ef87002df82a485c1b5e1c0a80544cf0ffe`; the source remains covered by
the project copyright notice. Only native command registration and its image reader are included; no external
DLL or full chat routing stack is required. Preserve the original source comments.

Local adaptations guard WIN32_LEAN_AND_MEAN/NOMINMAX against redefinition and
log resolved getter/registration/caller RVAs for the compatibility ledger.
Floating Damage's exact executable/Core admission runs before this backend.

The backend checks unique code patterns, .pdata function starts and native
definition field readers before registering through the UI thread. Parser-held
callbacks pin this DLL until process exit. Stop drains/disables callbacks;
there is no native unregister. Restart the game to update/remove this plugin.
See NATIVE-CONTRACT.md and the command record in docs/reverse-engineering for
the registration layout, byte patterns, lifetime and validation limits.
