# Death Crimson OX native state and calibration

Research date: 2026-10-25. Scope verified: private `deathcoxj.zip` Japan Rev A (`epr-23524a.ic22`), game ID exactly `DEATH CRIMSON OX`. Original Japan/USA/revisions were not runtime verified. All numbers below are offsets in 32 MB NAOMI RAM; SH4 cached address = `0x8C000000 + offset`. No ROM, RAM, screenshot, or native mesh data is included in this note.

## Native player state

| Meaning | P1 offset | P2 offset | Evidence |
|---|---:|---:|---|
| Player block | 0x18BFC8 | 0x18C004 | Native stride 60; initializer 0x36638 |
| Ammo, byte | 0x18BFC9 | 0x18C005 | Initialized/refilled to 10; native decrement 0x36E08..0x36E18 |
| Accepted shot pulse, byte | 0x18BFC8 | 0x18C004 | Set to 1 in the same positive-ammo branch; reset each update |
| Player active, byte | 0x18BFD5 | 0x18C011 | Many native draw/update functions check ==1; death 0x38E5E clears it |
| Health, float | 0x0974AC | 0x0974B0 | Native damage subtract 0x3CAD0..0x3CAD6; death check then clears active |
| Accepted shots, u32 | 0x097AE0 | 0x097AE4 | Increment 0x3D708..0x3D718 when the shot pulse is 1; native debug label is Shoot count |
| On-screen flag, byte | 0x18BFD8 | 0x18C014 | JVS parser sets after calibrated ADC bounds pass; clear otherwise |
| Calibrated X/Y, s16 | 0x18BFCC/CE | 0x18C008/0A | Written by JVS reader 0x74096/0x740C0 |
| Continue flag, u32 | 0x096634 | 0x096638 | Set when player dies; distinct from active flag |

The magazine size is a native constant 10, not a demonstrated RAM field. Ammo is also reset to 10 after damage/death, so an ammo increase alone does not prove a deliberate reload. Off-screen input clears the on-screen flag and refills ammo; the trigger branch requires on-screen. Empty pulls do not set the accepted-shot pulse. Native timed empty-magazine reload logic also exists, using player+32 and 0x09658A. These behaviors are supported by code; a full controlled 10-shot/empty/reload sequence is still needed.

The helper `core/rend/vr/death_gun.h` validates the measured instruction words and address literals before reading. It returns `known` separately from `active`, requires active==1 and finite positive health for active, bounds ammo to 0..10, and exposes the full u32 shot total. Its callback arguments are RAM offsets. The existing renderer GunState may truncate the counter to u16.

## Calibration and reload

Live calibration consists of min/centre/max u32 triples:

| Axis | P1 triple | P2 triple | Measured default |
|---|---:|---:|---|
| X | 0x097A68 | 0x097A8C | 11264, 33024, 60416 |
| Y | 0x097AB0 | 0x097AD4 | 13056, 36736, 64256 |

Pointers at 0x7E3EEC/0x7E3EF0 select each player's X triple; 0x7E3EF4/0x7E3EF8 select Y. The JVS path only uses min and max; its stored centre is unused on that path. Bounds are inclusive. Raw values are full **unsigned 16-bit** words (0x7407A / 0x740A4), not Lupin's signed-low-byte model.

The forward mapping, in this order:

1. Emulator JVS ADC: `raw = floor(inputPixel * 65535 / (extent-1))`.
2. Reject outside `[min,max]`, clearing on-screen.
3. Native SH4 float arithmetic and truncation: `screen = trunc(float(raw-min) * float(extent) / float(max-min))`.

Extent is 640 for X, 480 for Y. Default center input `(320,240)` becomes native `(280,185)`; this was observed in actual RAM as well as derived. Corrected input `(350,283)` becomes `(320,240)`. Sending zero ADC remains the native reload sentinel; the helper never modifies negative/off-screen inputs. No EEPROM or game RAM calibration writes are required.

`readCalibration` reads live bounds under the same code guards. `calibrateInput` receives an explicit enable gate plus a caller-owned `CalibrationMap`, rebuilds only when bounds change, and searches the real quantized forward model for the nearest available input. It refuses bounds that cannot represent all on-screen targets within one pixel. The adapter can enable this for calibrated menu targeting too; it should not confuse the enable gate with the player's active gameplay flag.

## Hit testing uses the live lens

The native accepted-shot mesh test starts at 0x59CF2: it reads each player's shot pulse, converts calibrated X/Y to floats, makes small screen bounds, and calls 0x59E74. That routine obtains object mesh parameters through 0x7613C, projects mesh vertices at 0x59FD8..0x59FE2 with the library function 0xAE45C, and tests the projected polygons against the shot bounds.

The projection function 0xAE45C loads focal address 0x86A588 and at 0xAE470..0xAE480 computes transformed X * live focal / Z (plus viewport scale and center). Y uses the same focal. Thus the field-of-view builder patch at 0xB846C changes hit geometry as well as rendering. The camera adapter should use the **live widened focal** for aim, rather than deriving a separate stock focal. This is traced code evidence; an off-center causal hit/score experiment with the widening enabled remains useful.

## Capture evidence and limits

The route `dc-route-port` crosses gameplay, death, continue, title, and P2 title/start states. Screenshots at65s and95s show Continue, at80s active P1, at115s title. It is not a monotonic three-shot experiment.

| Capture time | P1 ammo | P1 accepted shots | P1 active | P1 health |
|---:|---:|---:|---:|---:|
| 62 | 10 | 0 | 1 | 0.396 |
| 76 | 9 | 1 | 1 | ~2.349 |
| 91 | 10 | 1 | 0 | 0 |
| 106 | 10 | 1 | 0 | 0 |
| 125 | 10 | 1 | 0 | 3 |

The 75s fire was accepted; 90s/105s fires were during inactive states and did not increment the counter. P2 never has an accepted shot in those captures, so P2 stride/counter assignment currently rests on code rather than a causal P2 shot.

A second private `dc-state-controlled` run used Start70/72/74; capture78, shots80/82/84, reload86, P2Start88, P2fire90 with a capture0.6s after each. All seven captures show P1 active1, health3, ammo10, shot total0, and P2 inactive. Its first scene was still the intro (sequence block at0x18BF24, second s16=6), reaching gameplay near the last captures. It did not causally verify shots or reloads. The harness ended and restored config/saves before the parent resumed runtime tests.

A possible native shooting-inhibit gate is 0x094FD4: the gun updater skips input if zero. It is also zero during some continue/title transitions, so it is **not** a demonstrated cutscene flag. Native checks of movie/pause/sequence state also precede gun input. Do not ship a cutscene detector based on that one word. Render evidence or additional controlled scene captures are needed.

For longer private experiments, health at0x0974AC can be restored to3.0 (`0x40400000`) under the test-only POKE path; never ship that write. Keep a full 10-shot/empty/reload experiment under10s or prevent death in the private harness. Observe per-player active, ammo, shot count, and health together.

## Verification

`hotd2-vr/re/death_gun_test.ps1` builds a standalone MSVC C++17 test. It was first run against stubs and failed on the measured forward values, inverse mapping, and state fields; implemented helpers then passed. Tests cover both native state layouts, inactive/zero-health/noisy state, bad code signatures, invalid calibration, cache refresh, all640 X targets/all480 Y targets to <=1px with the independent cabinet oracle, and reload/out-of-range preservation.

Private numeric inspection tool: `build-win/state-research/inspect_state.py`; it only reads the input RAM paths and prints fields.
## Established native movie mode (follow-up)

The native mode resolver at **0x0550A2** explains the tight route's ignored shots. Let `root=read32(0x09A23C)`. Once `root+136` is nonzero:

- Root's signed16 mode at+0 is returned directly for1,2,4.
- Otherwise signed16 scene index is+2 and signed16 frame index is+4. Root+6 is a changing frame timer, not a count. Scene index-1 returns mode0.
- The scene table is `read32(root+36)`, with16-byte entries. The scene descriptor is `read32(table+sceneIndex*16+8)`.
- Its frame table is `read32(descriptor+20)`, with64-byte entries. The current mode is signed16 at `frames+frameIndex*64+2`.

Native debug function **0x069D82..0x069DDA** compares the same frame mode with2. For mode2, frame+60==1 displays MOVIE; other values display MOVIE NO SKIP. Other frame modes display SHOOTING. This is exact in-game terminology, not an interpretation of the camera motion. The gun updater at **0x036842..0x03684C** calls0x0550A2 and suppresses gun input whenever its return is nonzero.

Numeric fixture results from the existing private RAM captures:

| Capture group | Native mode | P1 active | Ammo / shots | Meaning |
|---|---:|---:|---:|---|
| dc-reva-baseline0001 | 2 | 0 |10 /0 | Attract movie, exclude with active gate |
| dc-reva-baseline0002 | 2 | 0 |10 /0 | Direct root mode2, exclude with active gate |
| dc-route-port0000,0001,0002 |0 |1,1,0 |10/0,9/1,10/1 | Combat, then continue transition |
| dc-state-controlled0000..0004 |2 |1 |10 /0 | Active player's intro movie |
| dc-state-controlled0005..0006 |0 |1 |10 /0 | Interactive sequence reached after scripted P1 fires |

`death::readScene(read16,read32)` now returns `Scene {known,movie,mode}`, where only native mode2 is movie. Use `known && movie` together with a known active-player mask, and the renderer's normal debounce. Unavailable or malformed state returns known=false, movie=false. The helper verifies mode-parser code signatures, root/table/descriptor/frame pointers for4-byte alignment and complete cached32MB RAM spans, conservative signed scene indices0..511 and frame indices0..4095. No independent native scene/frame count fields have been established, so the helper does not invent one. Table-prefix bounds cover every accessed entry.

The offline fixture mirrors the captured intro root0x18BF24 -> scene table0xB69320 -> descriptor0xB692E0 -> frames0xB68DC0, frame1 mode2 at0xB68E02. It also exercises mode0, direct root mode2/4, scene index-1, all negative/out-of-range indices, null/misaligned/out-of-RAM/end-of-RAM pointers at every chain level, unloaded root, and signature mismatch. Added scene tests first failed5 assertions against a stub, then passed after implementation. RAM callbacks assert aligned, in-bounds reads in these fixtures.

## Zero ADC reload needs no trigger

The native interactive gun updater at **0x0368C6..0x0368DC** checks player+16(the on-screen flag). When it is0 and ammo is below10, it writes10 and invokes the reload UI update. No trigger bit is tested in that branch. Trigger processing follows separately at **0x036DB0..0x036E76**; positive ammo and on-screen state are required to set the accepted-shot pulse and decrement ammo.

Therefore sending zeroADC by merely aiming offscreen is sufficient to reload while the player is active and the scene is interactive. The digital offscreen sensor bit is ignored by Death Crimson. The shared `analogOffscreenReload` strategy, which holds zero ADC and preserves trigger semantics, matches this native code. This conclusion is static-code verified; the upcoming parent-owned gameplay route can confirm causally with an offscreen aim action carrying no button bits.

Private executable probe source: `build-win/state-research/probe_state.cpp`, which includes the real helper and reads private captures without copying them. Production tests remain `hotd2-vr/re/death_gun_test.ps1`.
Real-helper validation corrected an early count assumption: root+6 initially held9 but became0 at the next intro frame. A fixture with this zero timer was added and failed against the count-based bound. The bound was removed, the fixture then passed, and the compiled real helper was run against all7 dc-state-controlled captures. Outputs were known1/mode2/movie1 for0000..0004 and known1/mode0/movie0 for0005..0006, with calibrated center350,283 in all7. Thus the final helper does not reject legitimate frame transitions because their timer is0.
## Shot latency, damage refills, and coordinate-valid flag

The parent-owned `dc-rounds-keyed` log records ammo10->9 at game time65.337s, then accepted-shot counter0->1 at65.373s,36ms later. This matches the native update ordering: updater0x03681C invokes prior-shot consumer0x03D6D4 at0x0368BE before calling current-input handling0x036DB0. The latter requires positive ammo at0x036E08..0x036E0C, decrements ammo at0x036E14/16, and writes the accepted-shot pulse1 at0x036E1A. On the following native update,0x03D6F2/0x03D6F6 tests that pulse==1, then increments the per-player u32 counter at0x03D708..0x03D718. Recoil should therefore follow the accepted-shot counter directly; requiring an ammo drop in that same snapshot can lose a real shot.

This is the **shoot** counter, independent of hit success: debug strings at0x092BB0/0x092BC8/0x092BE0 label separate Shoot/Hit/Kill counts. The hit-count increment is later at0x03D794..0x03D7A4, after the non-null hit-result branch. Positive accepted-shot deltas while active are evidence that a round fired, even on a miss. Normal JVS empty pulls do not set that pulse.

The same log records ammo9->10 at66.554s with no explicit reload. Native damage processing at0x03CAC6..0x03CAF6 subtracts the current damage amount from health, calls the damage feedback routine, then refills that player's ammo to10. Periodically restoring health to3.0 prevents death but does **not** prevent attacks or their magazine refills. Thus even a single positive damage event can defeat a ten-round empty-magazine experiment. The keyed log did not watch native health or coordinate-valid at that exact instant, so its refill cannot be assigned causally from that log alone; the damage path is a concrete independent explanation.

For an isolated **private research run only**, the verified RevA damage-refill store may be replaced with NOP:

| Offset | Stock word | Private replacement | Meaning |
|---:|---:|---:|---|
|0x03CAEE |0xD020 |unchanged | Load ammo base0x8C18BFC9 |
|0x03CAF0 |0xE10A |unchanged | Load r1=10 |
|0x03CAF2 |0x223F |unchanged | Multiply stride60 by player index |
|0x03CAF4 |0x031A |unchanged | Move product into r3 |
|0x03CAF6 |0x0314 |0x0009 | Skip only `mov.b r1,@(r0,r3)` |

After validating this signature, test harness action `poke 8c03caf6 2 0009` skips only that damage refill for both players. Damage/health handling remains active; ordinary offscreen and empty-magazine reload paths remain intact. This patch must never enter the production profile, APK, shipped configuration, or gameplay code.

Player+16 is coordinate-valid/on-screen, not trigger release:

- JVS parser0x073E58 reads P1 raw u16 X/Y at0x7F0B80/0x7F0B82 (P2+12), validates calibration bounds at0x074028..0x074064, sets player+16=1 at0x07406A/0x07406C, then writes calibrated X/Y at0x074096/0x0740C0.
- Failure clears player+16 at0x0740C4/0x0740C8 and leaves the prior X/Y latched. In-range latched X/Y therefore do not by themselves prove the ADC remained valid; watch the flag and raw ADC words too.
- Alternate device updater0x074138 can clear coordinate-valid on an unchanged alternative pointing device, but skips all players when its selected-device getter returns negative. Getter0x0D6CFE reads0x86A51C. All twelve existing route/controlled captures hold-1 there, so this alternative path is disabled in those captures.
- Debug/attract input can force coordinate-valid1 at0x0654CC. Normal captured auto-input mode0x09A268 and neighboring flags are0.
- No trigger-release writer to player+16 was found. The normal trigger state lives separately in the input block referenced by player+8.

The native raw JVS parser0x07B51C assembles fresh digital current/rising/falling words and copies analog words into the per-player raw blocks. The calibrated player-state updater0x073E58 is a separately scheduled task. The alternate-device updater0x074138 is called from0x04AA48. These are separate operations; a snapshot should not assume every field changed atomically.

The keyed log's cabinet pair331,312 for target295,277 maps exactly back to295,277 with the measured calibration. Native X/Y subsequently move before the actual shot is accepted (the log includes298,273 ->296,272 ->292,275 ->294,275). A marker near292,275 is therefore consistent with input changing during native acceptance latency; it is not evidence of a fixed calibration offset.

At this follow-up, `dc-rounds-repeat-start` contains only a pending `rip.request` and no RAM dumps, so no new snapshot of its flags could be inspected. No runtime was launched for this follow-up.