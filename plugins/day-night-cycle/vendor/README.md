# Support source provenance

Five user-owned support files were copied on October 21, 2026 from the current
D2R-Mods checkout: native chat registration/image scanning from
`plugins/always-show-sockets/vendor/`, and the boolean editor from
`plugins/shared/chat_config.h`. [provenance.json](provenance.json) records the
original paths and SHA-256 values. Preserve the existing source comments.

The chat backend registers the game's normal slash parser on the SDK UI queue.
It verifies unique patterns, function starts and record layout without installing
another hook. Retained callbacks pin this DLL; Stop drains/disables them.
Restart the game to remove/update the DLL or reuse the command name.
There is no cross-process named chat mapping in this backend.
