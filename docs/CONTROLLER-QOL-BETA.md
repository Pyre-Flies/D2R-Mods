# Controller QOL rev.78 validation

The tested rev.78-beta.4 implementation is published as rev.78 in
release-2026.10.13.1.

- Start distant LB loot travel during the native client update. This corrects
  dispatch timing associated with intermittent slow approaches and reported
  invisible blocked ground. The user could no longer reproduce those symptoms
  with rev.78-beta.4; rev.78 preserves that implementation.
- Use native running for explicit loot travel without changing the player's
  walk/run preference. Already-mapped, still-targetable items can initiate travel
  beyond the mapping radius; repeat suppression is reduced to 250 ms.
- Keep each LB loot assignment fixed until release/repress. Missing labels and
  newly dropped items cannot replace existing assignments mid-hold. Execution
  still rechecks visibility, item identity, session, held input and request age.
- Restore hosted identify, transfers and bulk-deposit admission for the reviewed
  Global Chat and Maps versions. Existing offline routing and older supported
  profiles are retained; unsupported native profiles fail open.
- Respect filtered-pickup blocking for neutral controller A before submission.
  Gold is exempt, including automatic pickup; native capacity and pickup settings
  still apply. Mouse/keyboard behavior is preserved.
- Reorder loot-filter rules with LB+Up/Down, with matching bumper hints.

### Configuration

Existing settings are preserved. `ground_pickup_distance` controls the LB mapping
radius, not maximum travel to an already-mapped item. Set it to `20` in the client
configuration for the wider tested radius; the shipped default is unchanged.
No server configuration change is needed for that mapping preference.

### Validation and limits

All 33 Controller QOL automated suites pass, including 483 production audit checks
and DLL ABI/resource/export/version checks. The user confirmed hosted item actions,
advanced-stash totals with rejuvenation potions, vendor purchases/sales, wider LB
mapping and the final travel behavior in a self-hosted Reimagined session.

A fresh offline gameplay retest, all filter/reorder combinations, broader hosted
configurations and save/rejoin persistence are not fully qualified. A prior beta.3
shutdown produced a leaked-stats assertion followed by a graphics-driver access
violation; its cause is undetermined and this release does not claim to fix it.
A clean shutdown on the final candidate is unconfirmed. Report regressions with
the input sequence, active mod, client/server context and plugin versions.
