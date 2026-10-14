# Validation and evidence

Use this reference when designing tests, interpreting runtime results, deploying a diagnostic build, or reporting completion.

## Evidence layers

Qualify each conclusion at the strongest layer actually observed:

1. **Static:** source, disassembly, tables, imports, or exact-byte analysis.
2. **Automated:** unit, policy, adapter, generator, or compatibility tests.
3. **Artifact:** correct DLL version, ABI/resource manifest, exports, imports, runtime linkage, and hash.
4. **Loader admission:** the intended plugin and compatibility profile load without refusal.
5. **Runtime path:** the expected hook/service executes with valid inputs and bounded logging.
6. **Authoritative state:** the game's relevant service/state reports the intended result.
7. **Visible behavior:** the UI/gameplay result matches the intended native behavior.
8. **Persistence:** save/reload or session transition preserves the result when required.

Do not collapse these layers into “works.” State skipped layers and why.

## Live-test discipline

- Use a reversible, low-value test state and a narrow operation first.
- Snapshot relevant preconditions, identity, coordinates/state, session, container/page, table revision, and target selection when applicable.
- For transactions, validate all participants and unaffected observable entries before and after. Check client/UI state separately from authoritative state.
- For controller or UI hooks, test press/release behavior, native competing actions, scoped surfaces, prompts/glyphs, and unload/session transitions.
- For tooltip or rendering changes, retain native text, values, ranges, colors, and unrelated lines; omit uncertain additions instead of guessing.
- Stop mutation after an unexpected mismatch, latch the failure if repeated calls could compound damage, and retain diagnostic evidence.

Never automate irreversible or high-risk live mutations merely to complete a checklist. Leave clearly scoped user validation instructions when the user must perform the in-game action.

## Plugin coexistence coverage

When changing hooks, patches or shared-state overrides, test supported predecessor admission, unknown-patch refusal, partial setup, later-owner restoration refusal and teardown lifetime as applicable. Exercise combined effects and both relevant load orders when the loader permits them, including session transitions and inactive-feature behavior. Check that predecessors run with preserved ABI state and that actions or rendering are not duplicated.

Static composition tests do not prove live compatibility. Report exact companion versions, load orders and live combinations checked; do not claim arbitrary third-party compatibility from a single successful pair.

## Completion report

Report:

- build and artifact identity;
- tests and artifact checks actually run;
- deployed path and built/deployed hash comparison, if deployed;
- live behaviors confirmed and by whom;
- remaining uncertainty and unsupported builds/surfaces;
- research/compatibility records updated.
