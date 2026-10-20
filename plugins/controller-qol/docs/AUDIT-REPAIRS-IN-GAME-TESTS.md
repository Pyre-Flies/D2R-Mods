# rev.76-beta.4 single-client acceptance cases

Beta.4 **Cube self-insertion, Shared Stash escape and Cube behavior through
rejoins are user-confirmed fixed** on 2026-10-11. The user subsequently accepted
non-online gameplay as ready; **online validation remains open**. The checklist
below remains a reference, not a request to repeat accepted local gameplay.
Unreported subcases remain coverage limits. Pickup reach/no-approach behavior
is unchanged and documented; overall acceptance does not claim a pickup fix.
Beta.3 T04 was
user-cleared, T01/T03 partial, and T05 failed: **LB+X actually moved the Cube
into itself**. Stop that test on beta.3. See
[the live findings and repair](AUDIT-LIVE-FOLLOWUP-BETA4.md).
This checklist assumes **one running game
client and one tester**. No case asks you to open a second client, arrange a
second player, or change a server's plugins.

The local Release candidate passes automated checks. The three beta.4 live
results above are confirmed for the tested single-client setup; they do not
clear every subcase or hosted-peer behavior. Use low-value items and backed-up
test characters for remaining cases.
The tested beta.4 is now installed locally in Reimagined with a verified beta.3
backup. All 28 config files and 39 other plugins stayed byte-identical.
Close D2R before replacing/restoring a DLL. Preserve existing QOL and skill
profiles; this candidate adds no configuration key. Beta.3 was installed
locally in Reimagined at your request. The previous beta.2 DLL has a
verified backup and guarded rollback; all 28 configuration files remained
byte-for-byte unchanged during installation. No game was launched by the agent.

Record candidate version/SHA256, game/loader version, mod, input device, process
role, steps, observed result and relevant log lines. Keep live logs/device
details in private `outputs/`, not public packages. Xbox names below correspond
to PlayStation L1/R1, L2/R2, Cross/Square/Triangle/Circle for LB/RB, LT/RT, A/X/Y/B.

## Suggested order and feasible environments

1. Offline with your single client: T01, T03-T08, T09 and T10's local cases.
2. If your setup offers a host/listen-server game that you can run alone, close
   the offline game and use that same client to repeat T01 and T03-T10. Hosting
   alone exercises the host's local actor and authoritative route; it cannot
   establish guest isolation or simultaneous-player behavior.
3. A joined-client run is optional **only if an external compatible server
   already runs the identical candidate/plugin set**. The host and every client
   must have matching plugin sets, versions and DLL hashes. Do not pair beta.4
   with an older host. If no such server is available, mark joined-client
   coverage unavailable and continue the local checklist.
4. For T04, restart your one client with master disabled, test, then restore its
   prior configuration and restart. No other participant is needed.

T02 records the multi-actor automated evidence and live limitations; it has no
manual step. The missing peer coverage does not prevent you from completing the
feasible tests. It does prevent a claim of full multiplayer qualification.

Treat a wrong destination, duplicate/disappearing item, unintended native
button action or crash as a failed case. A log saying "submitted" is not enough:
verify actual inventory state and save/rejoin persistence. Keep the candidate
local while results and the remaining multiplayer limits are reviewed.

## T01 - New game and session cancellation (issues 1/2)

- Prepare a character with three low-value items in Inventory. Enter a fresh
  game and stay at the spawn position. Without moving or casting, open
  Inventory, drop those items through native controls, close Inventory and
  wait for normal labels. Hold LB, wait for assignments and press one assigned
  button. Expected: the displayed item is requested for your character without
  needing a preceding movement/cast packet. Dropping prepares the pile; this
  is not a claim that no native packet of any kind has been sent.
- Hold LB+X, release LB first, then Save & Exit. Start a new game with the same
  character, then repeat with another character, one client at a time.
  Expected: no old pickup request, stale assignment, unexpected transfer or
  crash. Fresh assignments describe only the current game's ground items.
- Repeat leave/rejoin five times, including opening a menu immediately after
  joining. Expected: labels/pickups resume and held keys do not dispatch a
  second shortcut merely because LB is repressed. Release the action button
  fully before the next intended shortcut.
- A request may finish before Save & Exit. That does not exercise delayed
  cancellation; the automated lifecycle fixture supplies that controlled case.
- Dropped ground items do not persist into a newly created offline game.
  Check only fresh-game cancellation/assignments here. Persistence checks apply
  to items moved into saved storage. The beta.3 two-of-three stationary pickup
  result remains partial; beta.4 does not add automatic approach movement.

## T02 - Guest isolation: automated coverage, no manual test (issue 2)

You cannot validate a distinct live guest with one client. Hosting alone,
changing characters between games or switching to keyboard/mouse does not
create a second actor in the same game.

The production regression fixture supplies separate local and peer native
units. It checks that peer pickup passes through while the local modifier is
held, a peer cannot claim the local one-use permit, guest Guided Arrow packets
are not inspected/projected using host state, and old local work cannot cross
session generations. These are simulated boundary checks, not a live network
test or a plugin-handshake test.

Still unverified live: host modifier/filter effects on a guest, different-item
ownership during concurrent actions, both players requesting the same item,
and independently persisted host/guest inventories. Record this case as
**automated only; live peer test unavailable**. Do not attempt mismatched or
missing-plugin peers as a substitute.

## T03 - Chord lifetime and ordinary input (issues 3/7)

- With two low-value transferable items in Inventory/stash, hold LB+X for a
  transfer. Keep X held, release LB and wait one second. Repress LB without
  releasing X. Expected: one shortcut only; no native drop, sale, cast or
  second transfer. Fully release X and make a fresh LB+X press; it works again.
- Repeat LB-first release for Y (Cube), B, RB and R3 in inventory/stash, then
  in world context with the buttons assigned to skills/actions. Expected: no
  extra native press/action when LB releases. Check R3's existing skill-tree
  behavior separately in T08.
- Reverse release order: action button first, LB second. Expected: normal
  release; the next ordinary unmodified physical press works.
- Hold LB+X/Y while closing/opening a panel. Expected: no late native action
  or transfer redirected to a new focused item. Release all keys; verify
  D-pad, ordinary A/L3 and unmodified skills still work.
- For LT/RT ground slots, release LB before the trigger; fully release the
  trigger and make a fresh ordinary press. Expected: no leaked weapon/skill
  action from the consumed trigger; the fresh ordinary trigger works.
- In normal Shared Stash, test LB+LT/RT. Expected: previous/next page once per
  press, no extra page turn from holding, and usable normal navigation after
  returning to Inventory/Personal/Cube.
- Hold LB+LT, then press/hold RT without releasing LT. Repeat with LB+RT first
  and LT second. Expected: first trigger turns a Shared page once; the second
  is consumed, with no main-tab escape to Gems/Personal and no direction change.
  Release LB first, then one trigger at a time. No late tab/page action occurs.
  Fully release both triggers; fresh LB+LT/RT and ordinary LT/RT work again.

## T04 - Master disable (issue 4)

- Save your prior value, set `[qol] enabled=false`, restart your one client
  and verify the loaded candidate version. Try LB+X/Y/B/RB/R3, LB+LT/RT,
  A/L3 and D-pad in world, inventory, stash and vendor contexts.
- Expected: native controls receive the ordinary combinations and QOL does
  not consume them or perform its shortcuts. Use low-value items because
  native drop/sell behavior is active. Check ordinary ground pickup too.
- Restore the prior value and restart. Expected: QOL shortcuts resume.
  Existing aim/profile settings remain saved; the QOL master gate also
  prevents aim initialization when disabled.

## T05 - Closed/changed storage context (issue 5)

First, with beta.4 and a backed-up test character, open the Cube and highlight
the carried Cube in Inventory. Press LB+X once, inspect Inventory and Cube,
then try LB+Y once. **Expected: the Cube remains in Inventory; no self-insertion,
duplicate or disappearance. Stop immediately on any failure.** Repeat LB+Y on
a Cube in Personal/Shared stash when that storage view is available. It must
remain on its source page. Then move an ordinary low-value item with LB+X/Y;
it still enters Cube and can be withdrawn. Save & Exit/rejoin and verify stored
items and Cube placement. No save repair or automatic recovery is performed.

- Highlight Inventory item A, close Inventory and immediately press LB+Y
  within one second. Expected: A remains in Inventory. World LB+Y may request
  its assigned ground item only; it must not also move A to Cube.
- Open Inventory, highlight A, press LB+Y and immediately close the panel.
  Repeat ten times. Expected: already accepted A may move; work that sees the
  closed source panel cancels. It never chooses another item.
- Press LB+Y on A and immediately focus B/empty cell. Expected: either A was
  already accepted or the unsubmitted request cancels. B never replaces A.
- Repeat from Personal Stash, normal Shared and Cube; close the required
  source panel while another panel remains open. Expected: unrelated open UI
  does not authorize transfer from a closed source.
- Verify Inventory -> Cube and Cube -> Inventory still work; the Cube cannot
  be moved into itself. Check embedded Materials Cube LB+X/Y and one advanced
  counter withdrawal separately for regression.

## T06 - Normal Shared -> Cube and persistence (issue 6)

- Put distinct low-value items A/B on the first normal Shared page (SDK index
  0) and another normal Shared page. Carry an empty Cube in Inventory. Focus A,
  press LB+Y once and inspect source, Inventory and Cube.
- Expected: exactly A moves directly to Cube, source is removed, and no
  Inventory fallback appears. Repeat with B on the other page. Save & Exit
  and rejoin using the same client; verify both item counts/identities and
  Cube placement persisted.
- Fill Cube so the selected item cannot fit; press LB+Y once. Expected: one
  refusal, source remains on its exact Shared page, and no Inventory/alternate
  page transfer or automatic retry. Repeat with Cube absent.
- Verify ordinary Shared LB+X still targets Inventory and Inventory LB+X
  still targets the selected normal Shared page.
- A previous-season/remove-only page or a loader without SharedStashWrite
  does not support this authoritative direct Cube route: expect no normal
  Shared Cube hint and no silent Inventory substitution. Test such a context
  only if it is already available; do not change your loader to create it.

## T07 - Shared selection/session while waiting (issues 5/6)

- Select A on the first Shared page, press LB+Y and immediately change page.
  Repeat ten times. Switch normal/previous-season only if available.
- Expected: already accepted A may reach its captured destination; otherwise
  it cancels. A same-cell item on the new page never becomes the target.
- Request the move and immediately Save & Exit. Rejoin, then repeat by
  selecting a different character after leaving. Expected: queued work cannot
  move an item for the new session/character. Inspect source/destination and
  save/rejoin state after any operation that did complete.
- Fast local completion may make it impossible to catch pending work manually.
  The automated fixture controls page/season/owner changes, missing local owner,
  request expiry and session cancellation across dispatch.
- If the compatible external-server option is actually available, repeat these
  steps there using your single client. Otherwise joined-client/server-owned
  selection and persistence remain unverified.

## T08 - Provider/coexistence regression (issue 7)

- Repeat T03 with your usual controller/Steam Input configuration. Record
  whether logs admit native normalized input or a naturally selected XInput
  fallback. If another input device is already available, switch devices
  sequentially in the same client; a second device is not required.
- Expected: one physical chord -> one shortcut, no stuck buttons/UI hang,
  ordinary controls resume after release/disconnect/reconnect.
- Check Item Roll Ranges R1/RB behavior, Mod Information LB/RB tabs, aim
  skill-tree R3 toggles, identify, belt refill and Deposit All once each.
  Expected: their existing context behavior remains intact.
- Concurrent GetState/GetStateEx, different pads, recursion and callback-draining
  shutdown have automated coverage. Do not patch bytes or bypass guards to
  force an input provider.

## T09 - Solo load/responsiveness and local aim (issues 2/8)

- With `debug_logging=false`, spend five minutes moving/casting near a large
  item pile and periodically use ground slots. Expected: correct assignments
  and pickup behavior, with no new noticeable pause or crash. Repeat in a
  solo-host session if available.
- Compare local Guided Arrow with your established `[aim]`/manual-aim settings.
  Expected: the pre-existing local behavior remains usable. The retained
  legacy correction was not removed or fully consolidated by these repairs.
- Responsiveness and missing log lines cannot prove diagnostic scans are gone:
  the old scans also ran silently when debug was off. The source change and
  automated hook fixture establish removal of the loot observer installation.
  No measured FPS improvement or guest aim isolation is claimed by this case.

## T10 - Rejection and rapid exit; remote delay optional (issues 1/2/6)

- Fill Inventory so a selected ground item cannot fit. Press its LB slot once
  and wait without repeating input. Expected: it remains on the ground under
  native rejection rules; no automatic request loop. Free space and make a
  fresh press. Repeat full-Cube rejection from T06.
- After one loot/Cube shortcut, open a menu, release LB or immediately Save &
  Exit. Expected: no different item/destination, duplication or leftover request
  in the next game. A request already submitted may finish under native rules.
- Save/rejoin after successful operations and verify item counts/placement.
  This checks your one character's persistence, not a distinct peer's save.
- If an already compatible external server is available, observe ordinary
  delay/disconnect with the same single client. Do not create artificial network
  delays or change server files for this checklist. Otherwise network latency,
  disconnect during server processing and concurrent peer pickup remain open.

## Result sheet

| Case | Offline: one client | Solo host: same client, if available | Joined client: compatible external server only | Evidence limit / notes |
| --- | --- | --- | --- | --- |
| T01 | Pending | Optional, pending | Optional/unavailable | Controlled late callbacks also tested automatically. |
| T02 | No live peer test | No live peer test | No arranged peer test | Automated actor isolation only; live multiplayer unavailable. |
| T03 | Pending | Optional, pending | Optional/unavailable | |
| T04 | Pending | Optional, pending | Skip server admission while changing local config | Restore prior value after local test. |
| T05 | Pending | Optional, pending | Optional/unavailable | |
| T06 | Pending | Optional, pending | Optional/unavailable | Verify save/rejoin counts and destination. |
| T07 | Pending | Optional, pending | Optional/unavailable | A fast completed action does not exercise pending cancellation. |
| T08 | Pending | Optional, pending | Optional/unavailable | Concurrency and shutdown are automated fixture checks. |
| T09 | Pending | Optional, pending | Optional/unavailable | Solo observation only; no guest/FPS claim. |
| T10 | Local rejection/exit pending | Optional, pending | Optional/unavailable | Network/peer race coverage remains open if unavailable. |

## How to interpret completion

Passing your feasible cases supports single-client input, local ownership,
host-local operation where available, current storage context, item destination
and your character's save persistence. It does **not** establish cross-player
isolation, competitive pickup races, a remote client's authoritative behavior
on an unavailable server, or independent host/guest persistence. Keep those
entries marked unverified; the automated fixtures reduce the uncertainty but
do not replace live network evidence.

See [AUDIT-REPAIRS-REV76.md](history/AUDIT-REPAIRS-REV76.md) for the per-issue source
reassessment and automated evidence.
