# Embedded Deposit All button artwork

Beta.8 retains beta.7's dark carved-stone border and fine recessed antique-gold
inlay. The gold arrow/tray remains the
central symbol. Normal, hover, pressed and disabled frames share the construction.
The current 144-by-136 size and gap 85/offset 1 fit are documented in `GOLD-ANCHOR.md`; generation
prompts and source-image provenance are in `assets/art/PROMPTS.md`. Earlier
validation sections below retain their historical sizes and hashes.

## Asset and build contract

The four original PNG sources live in `assets/art/`: `normal.png`, `hover.png`,
`pressed.png`, and `disabled.png`. They must use equal square canvases, contain
visible artwork and transparent pixels, and be at least 144 pixels wide. The
conversion step does not paint, recolor, crop, or derive interaction states.

Run from the plugin directory:

```powershell
python tools/build_button_sprites.py --input-dir assets/art --output-dir <build-output>/button-art
python tools/build_button_sprites.py --self-test
```

Python with Pillow is a build dependency only. The tool writes these generated
files into the requested build directory; generated output is not source:

| File | Content |
| --- | --- |
| `deposit-button.sprite` | Four 144 by 136 RGBA frames, strip 576 by 136, 313,384 bytes |
| `deposit-button.lowend.sprite` | Four 72 by 68 RGBA frames, strip 288 by 68, 78,376 bytes |
| `button_art.rc` | RCDATA resources 2101 (HD) and 2102 (low-end) |
| `button-sprites-manifest.json` | Source and output hashes, state indices and dimensions |
| `deposit-button-preview.png` | PNG contact strip decoded from the generated HD sprite for visual review |

State indices are normal 0, hover 1, pressed 2, disabled 3. The generated RC IDs
must match `src/button_art_resource.h`. The PNGs are resampled independently to
each target size using Pillow's RGBA Lanczos conversion, preserving transparency.
The native button uses a logical 144 by 136 rectangle in beta.8 (144 by 140 in beta.7). The low-end file uses
half the raster dimensions, following the existing sprite-pair convention.

Intended virtual paths registered from the DLL are:

```text
data/hd/global/ui/d2rloader/inventory-deposit/deposit-button.sprite
data/hd/global/ui/d2rloader/inventory-deposit/deposit-button.lowend.sprite
```

The native layout uses the extensionless filename
`d2rloader/inventory-deposit/deposit-button`. These paths belong to the plugin,
so neither the stock TransmuteButton sprite nor a Charm Inventory asset is
replaced. There are no copied game pixels or donor sprite headers in the build.

## Format evidence, 2026-10-11

The supported form is an uncompressed `SpA1` sprite with format value 31. It has
a 40-byte header followed by a row-major RGBA8 strip; frames occupy consecutive
horizontal regions. All multibyte header fields are little-endian.

| Offset | Size | Emitted value |
| --- | --- | --- |
| `0x00` | 4 | ASCII `SpA1` |
| `0x04` | 2 | 31, uncompressed RGBA form |
| `0x06` | 2 | One frame's raster width |
| `0x08` | 4 | Full strip width |
| `0x0C` | 4 | Raster height |
| `0x10` | 4 | Zero |
| `0x14` | 4 | Frame count, 4 |
| `0x18..0x27` | 16 | Zero |
| `0x28` | remaining | Exactly strip width times height times 4 bytes |

The local Move Only helper at
`<d2rl-plugins-root>/plugins/move-only/tools/sprite.py` reads this form, including
the frame width, total dimensions, count and payload. Its companion assets were
read only for format comparison:

| Local asset | Header dimensions | SHA-256 |
| --- | --- | --- |
| `plugins/move-only/companion/data/hd/global/ui/d2rloader/move-only/skillicon.sprite` | Frame 132 by 130, strip 7128 by 130, 54 frames | `E9167F9DCEC7A3E43A24A1497A40F9A8361BDEC36A42D9D402C6F0C1C297B1DC` |
| `plugins/move-only/companion/data/hd/global/ui/d2rloader/move-only/skillicon.lowend.sprite` | Frame 66 by 65, strip 3564 by 65, 54 frames | `6FBEFA7FF486792CC7A6FB17217760F1C2512CE343345159ABF47C0CEA3D05AB` |

The meaning of every unused word in every sprite variant is not established.
In particular, the local HD sample has nonzero words at `0x20` and `0x24`, while
its low-end sample has zeros. These words are not copied or assigned invented
meanings. The independent [SpriteEdit writer](https://github.com/eezstreet/D2RModding-SpriteEdit/blob/master/D2RModding-SpriteEdit/MainForm.cs#L180-L274)
constructs new uncompressed sprites with those words zero, writes RGBA channels
directly, and creates a half-size low-end counterpart. Its reader treats format
31 as RGBA. This provides the construction precedent rather than inheriting
possibly unrelated values from the local donor. Compressed format 61 and other
sprite variants are outside this encoder's scope.

The namespaced resource and child layout contract is documented in the pinned
SDK `README.md`, Resources and Companion MPQs (lines 548-582) and native child
layouts (lines 951-964). Resource registration from embedded bytes avoids a
runtime MPQ dependency; it does not provide a guarantee of future game-format
compatibility.

## Validation and limits

Every build checks source dimensions/transparency, four distinct interaction
states, complete encoded dimensions and payload length, and byte-for-byte frame
round trips for both raster sizes. The codec self-test checks known asymmetric
pixels independently for header bytes, RGBA ordering, row ordering, frame
boundaries, and alpha preservation. It rejects malformed/truncated payloads,
duplicate states and empty art. The build records hashes and writes complete
files atomically; unchanged bytes do not change timestamps.

These checks establish the emitted file structure and preservation of the input
art. They do not prove native texture loading, visual quality, hover/press state
selection, click bounds, low-end scaling, or gold-relative placement in game.
Those require a live run of the exact built DLL with both texture variants.

On 2026-10-11, the codec self-test and conversion of all four original 1254 by
1254 RGBA PNGs passed. Both output files passed the pixel round-trip and state
checks. A contact strip decoded from the HD sprite was visually inspected at
its 120-pixel frame size: the four states keep the same framed arrow/tray
composition, with bright hover, darker press, and gray disabled artwork.

| Validated output | SHA-256 |
| --- | --- |
| HD | `DC92164F1D99499219F0BCFEFB48CEF87B5CE280DF3C3D9CFE4B0DD001DA565B` |
| Low-end | `C919ADFDA6419270FB94EA416FA6F8A9DAFBD85EA5AB17189D3F25508D2B0013` |

This is asset-format and offline visual validation. It does not count as a
live-game test of the custom sprite.

## Build and local deployment, 2026-10-11

The beta.3 Release build passed all seven CTest suites: sprite-codec, UI,
batch-policy, transfer-policy, artifact, settings, and gold-anchor. The actual
DLL's RCDATA resources 2101 and 2102 matched the validated sprite files byte for
byte. Its imports are limited to KERNEL32.dll and bcrypt.dll. The runtime ZIP
was checked for exact entries, integrity, matching DLL bytes and absence of
external PNG, sprite or MPQ dependencies.

| Artifact | SHA-256 |
| --- | --- |
| `Inventory Deposit.dll` (501760 bytes) | `53AE699E7D8B088DAB888CCC27960AC7396010EBE244C7CCA6745A5A4B400AAF` |
| `Inventory-Deposit-0.1.0-beta.3-runtime.zip` | `80C96858988A98121E9F7B4BE195D3716C85D24F79800209E3AA3EDC423FA11D` |

With D2R and D2RLoader closed, the verified DLL was installed in the local game
root's `d2rloader/plugins/Inventory Deposit.dll`. The previous beta.2 DLL and
configuration were backed up under
`d2rloader/backups/inventory-deposit-custom-art-20261011-072422/` before replacement.
The installed beta.3 hash matches the build. The existing configuration was
unchanged (SHA-256
`1DFE1C543CC6465A0ABE6A2D63708905C4F2B0950014C5D47F74B3CF3E5F5090`),
retaining gold anchoring, gap 122, and vertical offset 0. This is a local
installation, not a published release. Native loading and interaction with the
new artwork remain pending live-game validation.

## Beta.5 stone-frame fit and local deployment, 2026-10-11

The user approved the second frame revision, which uses matte carved stone
instead of metal rails. Matching interaction states were generated from that
normal frame. All four source canvases are 1254-by-1254 RGBA with transparency.
The source PNGs are generated originals; only format conversion/resampling is
performed by the build. Full prompts and hashes are in `assets/art/PROMPTS.md`.

The button and click rectangle now measure 132-by-132 UI units, with HD132 and
low-end66 frames. `gold_gap=110` shifts the left edge 12 units left while the
larger size keeps the right edge fixed. Vertical gold centering extends each
vertical edge 6 units outward. The screenshot suggests about seven additional
screen pixels of leftward coverage at that scale. No new native ABI, address,
structure offset or guard is needed; the existing anchor computes using the
live button dimensions. Deposit and tab-feedback behavior remain unchanged.

Release build and all nine automated suites passed. The updated anchor case
produces `(848,1614,132,132)` from the reference gold rectangle. Artifact and
packaging checks confirmed the 132/66-pixel embedded sprites, 132-unit native
layout, four states, version/ABI/exports and DLL equality. The decoded contact
strip was visually inspected at native HD size. These checks do not establish
final in-game alignment, hover/press appearance or low-end rendering.

| Artifact | SHA-256 |
| --- | --- |
| HD sprite, 278824 bytes | `B6D0CE823961D54F9FCB72D81B2A5A2ABDF62EAC8C3A34A0C9AF5F83493DE8FA` |
| Low-end sprite, 69736 bytes | `1FB2942921D9C1360B010BD1E2BD30EFEBD89510B35B0D1268FE8095FF797705` |
| `Inventory Deposit.dll`, 572928 bytes | `7718D748871D1BED6D4A34F6CCDADA1394C1CA18FC1BBE602FE59F92DC1BCAAF` |
| `Inventory-Deposit-0.1.0-beta.5-runtime.zip` | `98DDBBC6D331DBC04FD88B18610BF58A237B281528099ADDEC9886813979B162` |

The local install was updated with D2R and D2RLoader closed. Beta.4 DLL and
config backups are under
`d2rloader/backups/inventory-deposit-stone-fit-20261011-075857/` in the game root.
Installed DLL hash matches the build above. Only `gold_gap` changed in the live
configuration, from 122 to 110; config SHA-256 is now
`E2A386E63CDB4084F9D748F89B34F6E68FA6822AC60E24FB5AF9EC19E72588C2`.
For rollback, restore both backed-up files. The runtime ZIP includes reference
defaults only and does not overwrite a live config. Final in-game fit remains
pending; this is a local candidate, not a published release.

## Beta.6 measured lip fit and local deployment, 2026-10-11

The next user screenshot identified remaining space above the stone frame and
marked a strip to its left. The original four PNGs are unchanged. Resampling
now produces HD144/low-end72 sprites, matching the 144-unit native button and
click rectangle. Defaults and the requested local settings are `gold_gap=85`,
`gold_offset_y=-1`; the screenshot measurement is recorded in `GOLD-ANCHOR.md`.
No native contract, transfer behavior or feedback code changed.

The beta.5 runtime log confirms the prior DLL loaded at 08:01:07 and gold-anchor
placement/read-back succeeded with gap110/offset0 at 08:01:32 on 2026-10-11.
That establishes the previous screenshot's configuration, not the new fit.

The beta.6 Release build and all nine automated suites passed. The anchor case
resolves to `(823,1607,144,144)`. Artifact/package checks verified the actual
embedded sprite bytes, source hashes, layout size, states, version, ABI,
exports, imports, ZIP contents and packaged DLL identity. The decoded native
sprite preview was visually inspected. Final beta.6 alignment and low-end
rendering remain pending an in-game check.

| Artifact | SHA-256 |
| --- | --- |
| HD sprite, 331816 bytes | `4D1082E62D94B695B9E0A3220AFF5CF4DC3D10A9780A41B161ECE2A1F7DC68E8` |
| Low-end sprite, 82984 bytes | `3AAA2F2BB24D6515BB8706550BF88381E500DE5FDCB09AC6332D1278788838EB` |
| `Inventory Deposit.dll`, 639488 bytes | `640968E981B681388D3A74D8469D21086B2B8978F6456FF9C4736E49FF348549` |
| `Inventory-Deposit-0.1.0-beta.6-runtime.zip` | `1FB7532388B21BF22E5D8B4567FF4BF1D6DA2EE992229C225997E585B57240E7` |

Installed with D2R and D2RLoader closed into the game root's
`d2rloader/plugins/Inventory Deposit.dll`; the deployed hash matches the build.
Previous DLL and config are backed up under
`d2rloader/backups/inventory-deposit-lip-fit-20261011-081218/`.
Only `gold_gap` (110 to 85) and `gold_offset_y` (0 to -1) changed in the live
config, whose SHA-256 is now
`99375BAAF801A450684CFB0FBB6DEEC80BE14B1D0A91E2441248DFCFA4D01CDC`.
Restore both backed-up files to roll back. This remains a local candidate;
no commit or public release was made.

## Beta.7 shorter button and fine gold inlay, 2026-10-11

The user requested a tiny height reduction because beta.6 covered the footer's
upper gold lip, and clarified that the additional gold should be fine frame
inlay rather than a brighter center symbol. Four built-in image edits provide
the same stone construction with a clearer narrow antique-gold line inside
the existing perimeter groove. Original PNG outputs and full prompts are
recorded in `assets/art/PROMPTS.md`; no game pixels were extracted or copied.

The rectangle and native sprites are now HD144-by-140 / low-end72-by-70. Width
and config remain unchanged. Gold anchoring resolves the reference button to
`(823,1609,144,140)`, revealing approximately 1.2 screen pixels at each lip.
The converter resamples the square source to this subtly shorter rectangle.
Its existing codec test now uses asymmetric 2-by-3 pixels to verify independent
width/height, row/channel ordering, alpha and exact payload size. The format
and native contracts are unchanged.

The Release build and all nine automated suites passed. Actual DLL/package
validation checked rectangular sprite bytes, resource dimensions, source hashes,
layout, version, ABI, exports, imports and package identity. The four native-size
decoded states were visually inspected. The earlier beta.6 runtime log confirms
loading at 08:14:56 and gap 85/offset-1 placement at 08:15:23 on 2026-10-11;
beta.7 visual fit, interaction states and low-end appearance remain unverified
in game.

| Artifact | SHA-256 |
| --- | --- |
| HD sprite, 322600 bytes | `81CB60D4C178C23ABE985BFEE2D0F421BC0A622349E27E2C08E00CC1AF386223` |
| Low-end sprite, 80680 bytes | `59A85CA08F25301B37488CDA9AA3A4F86E5E8616B9194F9D17E3B7AA78B4C8D2` |
| `Inventory Deposit.dll`, 627712 bytes | `DF4159D92D7B4C877CEB663F40FB8236C7C6D40E16629660CD9D40C45BBA95F0` |
| `Inventory-Deposit-0.1.0-beta.7-runtime.zip` | `EC7215CE77C95D3A70AD1004BA97336F32F40E47C418431FB7BA80C1CD62352E` |

Installed with game and loader closed; the installed DLL matches the tested
build. The previous DLL and config are backed up under
`d2rloader/backups/inventory-deposit-inlay-trim-20261011-082458/` in the game root.
Config remains byte-identical with SHA-256
`99375BAAF801A450684CFB0FBB6DEEC80BE14B1D0A91E2441248DFCFA4D01CDC`.
Restoring the backed-up DLL rolls back this update. This is a local candidate,
with no commit or public release.

## Beta.8 top-lip correction and deployment, 2026-10-11

The beta.7 screenshot still shows overlap. Its installed DLL and runtime log
were verified before changing geometry; the prior build did load correctly.
The translucent fringe above the opaque stone frame needs additional clearance.
Beta.8 uses HD144-by-136 / low-end72-by-68 with `gold_offset_y=1`, preserving
gap 85 and the bottom/horizontal edges. All four source PNG hashes are unchanged
from beta.7. Measurements and their limits are in `GOLD-ANCHOR.md`.

Release build and all nine suites passed, including a geometry case that
checks the preserved bottom edge 1749. Actual artifact/package checks passed
for sprite bytes, layout, version, ABI, exports, imports and package identity.
The decoded sprite preview was visually inspected. Live beta.8 clearance and
low-end rendering remain pending; no new native facts or contracts were used.

| Artifact | SHA-256 |
| --- | --- |
| HD sprite, 313384 bytes | `E9F9A1C2DE2889F4251647F105686A4705A282F4E091180D2E7C0D1D94FAE72A` |
| Low-end sprite, 78376 bytes | `D3AC39EB25E6AFF65139B4612FCE06796DE99E8C0247573793E32D1ED76B1D68` |
| `Inventory Deposit.dll`, 616448 bytes | `5B95D65C73227CBB9A58EB6F2983911B386B6689BA16FB5DF5F352361B9920E6` |
| `Inventory-Deposit-0.1.0-beta.8-runtime.zip` | `22B4DCE1CBAE30EB4E0565E742C7D0C2641BAC4EC5C7E3EB8275428197C633F8` |

Installed with game and loader closed, and verified the installed DLL hash.
Prior DLL/config backup:
`d2rloader/backups/inventory-deposit-top-lip-clearance-20261011-084928/`.
Only `gold_offset_y` changed from -1 to 1; current config SHA-256:
`154283F7EBD5CCB13D8056D533E8E8F4C487594BB7211040B3723B8241571E2A`.
Restore both backed-up files to roll back. Local candidate only; not published.
