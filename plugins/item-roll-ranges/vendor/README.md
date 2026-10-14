# Native chat support provenance

These four user-owned source/header files were copied from the sibling
`d2rl-plugins` repository's `libs/chat-sdk` and `libs/image-scan`, 2026-10-11.
Only native command registration and its image reader are included; no external
DLL or full chat routing stack is required. Preserve the original source comments.

Local adaptations guard WIN32_LEAN_AND_MEAN/NOMINMAX against redefinition and
log resolved getter/registration/caller RVAs for the compatibility ledger.
Item Roll Ranges' exact executable/Core admission runs before this backend.

The backend checks unique code patterns, .pdata function starts and native
definition field readers before registering through the UI thread. Parser-held
callbacks pin this DLL until process exit. Stop drains/disables callbacks;
there is no native unregister. Restart the game to update/remove this plugin.
See docs/reverse-engineering/floating-damage-chat-command.md for
the registration layout, byte patterns, lifetime and validation limits.
