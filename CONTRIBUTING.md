# Contributing

## Workflow

1. Create a short-lived branch from `main`.
2. Keep changes scoped to one plugin or one shared concern.
3. Add or update automated tests where the behavior can be isolated.
4. Build the affected plugin with MSVC and run its complete CTest suite.
5. Record manual D2R validation separately from automated-test results.
6. Add a concise entry to the affected plugin's `CHANGELOG.md`.

## Reverse-engineering records

Any newly discovered path, module, RVA, absolute address, byte signature,
structure offset, ABI, call contract, or provider hash must be recorded under
`docs/reverse-engineering/` or the relevant plugin's `docs/` directory in the
same change. State:

- the module and exact build/hash;
- whether the value is an RVA or a process address;
- expected bytes or another compatibility guard;
- how the result was derived;
- what was automated-test and live-game validated;
- known failure behavior and whether the code fails open.

Do not generalize a result to untested builds. Do not commit proprietary game
code or binaries; keep only the minimum derived evidence required to reproduce
the finding.

## Pull requests

Complete the pull-request template. A successful transaction return code alone
is not enough for stateful game operations: verify authoritative state, client
publication, visible rendering, and persistence when applicable.

## Release checklist

- Version metadata, README, and changelog agree.
- Clean Release build and all tests pass.
- Compatibility hashes and guards match the tested deployment.
- Runtime ZIP contains only installable DLLs, reference defaults and user documentation.
- Reference TOMLs live under `defaults/`; archives contain no live `d2rloader/config/` paths.
- Configuration documentation explains migration, preserved values and rollback.
- Run `python tools/test_package_runtime.py` and audit extraction/checksums.
- Source ZIP excludes build output, logs, backups, and private game artifacts.
- `SHA256SUMS` or the release manifest covers every published artifact.
