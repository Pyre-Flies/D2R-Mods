# Release and sharing

Use this reference when the user asks to package or share a D2RLoader plugin.

## Release contents

Unless the project already defines a different convention, prepare:

- a ready-to-install runtime archive with the expected `d2rloader/plugins` layout;
- a source archive containing the reproducible plugin source and required build metadata;
- concise installation and removal instructions;
- compatibility requirements, including exact build/provider hashes when native behavior is gated to them;
- known limitations and live-validation status;
- SHA-256 checksums for archives and the runtime DLL.

Exclude build caches, local absolute paths, logs containing personal information, backups, save files, unrelated binaries, and credentials.

## Pre-release checks

- Build cleanly with the project's configured warning level and runtime linkage.
- Run the relevant test suites and artifact verifier.
- Check exported loader entry points, plugin ID/version, ABI/resource manifest, and imported DLLs.
- Confirm the packaged DLL is byte-identical to the tested artifact.
- If installing locally, close D2R/D2RLoader as needed, preserve the previous working DLL, and confirm the deployed hash.
- Scan the package for machine-specific paths and accidental secrets.

Do not describe static or simulated coverage as live validation. State the exact build family supported and fail-open behavior for mismatches.
