# Live durability and derived-charge baseline - 2026-10-17

Read-only PID50664 snapshot; existing socket-profile witnesses passed. All eight
prepared charge-affix items and runtime unique rows894/900 were present. Private
capture files stay in <workspace>/outputs (flatmax-charge-durability-20261017.json
and durability-20261017-details.json). No native call or memory write was made.

Gemini Coat row894/class316: root primary stat73=32, aggregate73=40; permanent
owned modifier stat75=25. Current durability72=26. Xanadu Dreams row900/class319:
root primary73=45, aggregate73=90; owned modifier75=100; current72=38. Both owned
lists are state0/flags0x40/expiry-1, parent/owner reciprocal. Stat75 is absent
from both aggregate lists. The observed values agree with 32*1.25=40 and45*2=90.
This confirms these two fixed samples, not every native rounding case. Looking
only at aggregate stats would miss the actual durability-percent operands.

Charged-stat204 layers decode as skill=layer>>6 and level=layer&63. For the
Miasma Bolt throwing knives item27/class43, raw36487=0x8e87; low byte135 and next
byte142 match the user's reported135/142. Layer25291 yields skill395/level11.
The consume-one comparison below verifies which field changes.
Other observed candidates (item,skill,level,low byte,next byte):
22,395,6,29,35;23,393,8,114,120;24,401,3,40,82;
28,401,1,22,22;29,396,4,8,30;30,396,7,40,112;31,393,5,12,32.
Do not assume newly spawned items have full charges. Negative function19 source
operands remain distinct from these realized levels and charge counts; their
complete derivation has not been established by this snapshot.

All table/stat/list offsets were already documented; no new native entrypoint
or ABI was introduced. The subsequent user report confirms missing numeric prefixes for both
durability armors. Charge-use persistence remains untested.

## One-use comparison

Same process50664/item27/class43 and saved affix IDs. After the user cast once,
stat204/layer25291 changed36487 ->36486: remaining135 ->134, capacity142 and
skill395/level11 unchanged. This validates the packed remaining/capacity fields
for this captured item. Aggregate stat70 (quantity) also changed140 ->173 during
the interval; the capture does not establish its cause, so this is not a claim
that charges were the only item state to change. Private after capture:
flatmax-charge-after-one-20261017.json. No save/rejoin test has been performed.
