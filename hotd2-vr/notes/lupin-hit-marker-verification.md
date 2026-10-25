# Original GDS-0018: hit framing, native markers and shot-flash audit

Status: measured flat-player research, 2026-10-25. Applies only to original
GDS-0018 (`lupinshoo`, `gds-0018.chd`), not Rev A. These results establish the
profile's input/marker policy; they do not establish complete game or headset
validation. The research player used a private checkout and UDP port 27026.
No game data, render captures, artwork or extracted textures belong in this tree.

## Causal hit framing

Widening the drawing lens does not change the native hit-test lens. Three fresh
boots used the same first-train camera and the same three preparation shots.
Each preparation sequence left score 0, accepted-shot counter 3 and ammo 4.
The fourth shot was scheduled four seconds after first ammo initialization.
Each accepted fourth shot advanced the counter 3 -> 4 and ammo 4 -> 3.

| Test / capture directory | Native calibrated input | Location of the stock ray in the widened image | Score after shot | Observed actor outcome |
| --- | --- | --- | --- | --- |
| A `hit-framing-rendered-wide-v8` | (285,225) | (304.82,233.49) | 100 | Front enemy wounded, remains alive |
| B `hit-framing-stock-projected-v8` | (238,202) | (284.44,223.52) | 500 | Front enemy killed, rear remains alive |
| C `hit-framing-back-stock-projected-v8` | (141,161) | (242.37,205.74) | 500 | Rear enemy killed, front remains alive |

C is the decisive control: (141,161) lies on sky in the widened render, while the
stock ray reaches the rear enemy's head near (242,206). Merely observing an
onscreen crack at the input coordinate would give the wrong world-aim policy.
Actor outcomes were checked in the pre-target, impact, reaction and later PNGs;
scores/counters/ammo were independently read from the private RAM snapshots.

For this camera, stored angle = 7281, stock focal = 879.425903, widened live
focal = 381.418915. Image centre = (320,240). The comparison above uses
`widePosition = centre + (nativeInput - centre) * liveFocal / stockFocal`.
Native calibration and EEPROM were unchanged in all three boots.

The guarded FOV experiment changed the original half-angle shift at RAM offset
`0x075E1E` from `0x4421` to `0x0009`; stored descriptor angle remained unchanged.
The drawing lens is the live float at `0xBD6120`. The stock lens can be recovered
from descriptor base `0x9A58FC`, stride 248, angle at +92, with active signed
32-bit index at `0x9A6994`. Native code accepts indices 0..3 and uses -1 as an
unavailable sentinel; captures observed index 0 only. Angle is loaded as a full
32-bit word: require 1..65535, not a truncated 16-bit value.

Use `(width / 2) / tan((angle >> 1) * 2*pi / 65536)` with integer angle halving.
Width was 640 at `0xBD6124`, height 480 at `0xBD6128`, width mode 1 at
`0x991CC8` in all measured cases. Angle zero occurs in setup/menu and requires a
live-lens fallback. Descriptor +84/+88 are aspect scales, not dimensions.

| Descriptor angle | Calculated stock focal | Measured unpatched focal |
| --- | --- | --- |
| 2756 | 2408.040223 | 2408.040039 |
| 7281 | 879.425957 | 879.425903 |

This supports dynamic descriptor-derived stock focal, with original-revision
code guards, rather than a fixed 879 value or reversal of the widened float.
Exact collision routine/call chain, further camera indices, low-resolution mode,
other scenes and headset aim remain unverified by this experiment.

## Native marker classification

`markerWidened = false`. The P1 red crack remains at the native input in A/B/C:
(285,225), (238,202), (141,161), respectively. P2 blue cracks likewise follow input.
They do not move to the stock-ray world-hit positions in the preceding table.
Preserve actual world hit stars, muzzle effects and enemy reactions.

The original marker geometry is a translucent four-vertex quad using texture
address `0x92D800`, at original W = 0.5263158083 (stored reciprocal depth z = 1.9).
An original W band of 0.52630..0.52633 covers the measured marker geometry.
A complete polygon UV bounding rectangle must fit within one of these regions:

| Region | u minimum | v minimum | u maximum | v maximum |
| --- | --- | --- | --- | --- |
| P1 red crack/glow | 0.25 | 0 | 0.5 | 0.375 |
| P2 glow | 0.5 | 0.5 | 0.5625 | 0.5625 |
| P2 blue crack | 0.875 | 0.375 | 1 | 0.5 |

The regions include clipped subsets; a rectangle crossing a region boundary is
not accepted. Fresh replay through the audit found 21 distinct P1 rectangles,
19 P2 rectangles and 119 unrelated same-depth HUD rectangles. All 40 measured
marker rectangles matched, none of the 119 HUD rectangles matched, and the two
sets had no overlap. Across the three controlled grid captures, 649 marker
quads were found in 108 passes.

Texture address and depth alone are insufficient: the same atlas and W also
contain ranking/failure glyphs, including a 171-quad text pass. Runtime suppression
additionally requires a recent 3D shot and proximity to its original, unwidened
screen coordinate. UI selection shots must retain their native feedback.
Classify before UI depth snapping; a snap can move original W 0.5263 near 0.599
(or another profile's overlay), invalidating an original-depth test after snap.
Disable broad generic atlas matching for a profile using this earlier classifier.

A/B/C raw rip requests started about 0.2 seconds after the target shot and missed
the brief crack geometry. Their earlier impact PNGs establish marker position;
the atlas, W and UV measurements come from the controlled grid bursts below.
Do not claim that A/B/C rips independently contain those marker quads.

## Full-screen shot-flash audit

The evidence supports `noShotFlash = true` for this original-revision profile.
No bright full-screen UI polygon was found in the controlled gameplay samples.
Preserving native fades/damage effects is preferable to guessing a CRT-flash rule.
This is sampled absence, not proof that every later scene has been examined.

| Controlled raw capture | Passes | Marker quads | Bright full-screen UI candidates |
| --- | --- | --- | --- |
| `aim-grid-original-stock-rip-v2` | 29 | 194 | 0 |
| `aim-grid-original-wide-rip-v2` | 29 | 215 | 0 |
| `aim-grid-original-inverse-wide-signed-v6` | 50 | 240 | 0 |
| `boundary-original-wide-v7` | 20 | 0 | 0 |
| A/B/C combined | 18 | 0 | 0 |

Grid rip bursts were requested approximately 50 ms after P1 shots and 100 ms
after the final P2 shots, with three consecutive passes. Existing per-frame
shot diagnostics provide an additional check within the first 200 ms:

| Capture | Logged shot-polygon entries at 0..200 ms | Bright full-screen all-2D entries |
| --- | --- | --- |
| Stock grid v2 | 2100 | 0 |
| Widened grid v2 | 2392 | 0 |
| Inverse widened v6 | 5184 | 0 |

These are logged polygon entries, not unique frames. The old player's diagnostic
already ran after its UI snap, so its `all-2D` label is separate from raw W.

The broader offline census covered 179 passes / 222735 polygons in 13 capture
directories, with no invalid geometry. Five broad bright UI candidates were
found outside the controlled gameplay bursts: `flat-attract-original` pass 2,
`flat-play-original` passes 1/2, `flat-route-original` passes 0/4. All five are
opaque, textured, 640x480, W 0.5454545691, YUV format 3, atlas addresses
`0xBAD800` or `0xB6D800`: the full-screen movie layer. White vertex colour does
not make its decoded movie image a white flash.

The audit also counted 1434 bright full-screen world-polygon bounds across the
broader census. That count includes ordinary white-modulated world triangles;
it is not a count of visible flashes or effects. Large world muzzle/impact
billboards also occur. Such world geometry must not be removed as a UI flash.

## Reproduce without launching a player

The companion audit uses only Python's standard library. It accepts RIP1 pass
files or directories containing `passNNNN.bin`, reads only those files, validates
length/count bounds, and emits numeric metadata. It never reads or extracts
VRAM, palettes, textures, images or RAM. `--output` requires a new output file.
The geometry classifier alone does not establish a recent accepted shot.

```powershell
python hotd2-vr/re/lupin_marker_audit.py --self-test
$privateVrtest = 'C:\private\vrtest' # Existing, untracked research captures
python hotd2-vr/re/lupin_marker_audit.py `
  "$privateVrtest\aim-grid-original-stock-rip-v2\rip" `
  "$privateVrtest\aim-grid-original-wide-rip-v2\rip" `
  "$privateVrtest\aim-grid-original-inverse-wide-signed-v6\rip" `
  --output "$env:TEMP\lupin-marker-grid-new.json"
```

For HUD negatives, audit `flat-route-original` passes 2/3,
`state-rounds-original-v1` pass 6, `state-rounds-original-v2` passes 5/6,
and `state-reload-original-v3` pass 4. Deduplicate `uv` rectangles in
`shared_atlas_depth_other` candidates: expected 119. The three grid directories
above yield 40 distinct `marker_uv_rectangles`, 649 marker candidates, 108 passes,
and zero `ui_flash_candidate` results. Built-in tests cover all three UV regions,
clipping, HUD negatives, wrong textures/lists/depth, non-quads, malformed or
truncated passes, and separation of bright UI candidates from world geometry.

A flash candidate means polygon bounds cover at least 90% of each framebuffer
axis, first-vertex RGB each >= 192, and every vertex lies in original LP UI W
0.49..0.75 or on the capture header's overlay (within 0.002, W <= 1.15). This is
a deliberate superset of a snapped overlay test, not a texture-whiteness test.
There is no automatic causal shot timing inferred from a pass file alone.

Private evidence names: `hit-framing-report.json`,
`hit-framing-measured-evidence.json`, `stock-focal-descriptor-evidence.json`,
`shot-marker-uv-evidence.json`, `original-marker-band-census.json`, and
`marker-flash-audit-v1.json`, under the isolated research `state-research` folder.
These reports and their raw captures remain untracked.
