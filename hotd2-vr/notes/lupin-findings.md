# Lupin VR findings: original GDS-0018

Implementation remains in progress. This record contains measured numeric metadata only; game dumps and captures remain outside tracked source.

- Source CM e6bd81a97, independent branch codex/lupin-vr. User approved full feature parity on2026-10-24.
- Supplied combined CHD SHA1 3b7d5dd917b9a2cc9b7b51aad22f8e75383c3301 matches original GDS-0018; raw SHA1 ded9d1aa791e64b0db962a5ea762f4ff28b2e76a. Correct original pairing is lupinshoo.zip and lupinshoo/gds-0018.chd, PIC317-0325-jpn.pic. Original misleadingly named files remain untouched; normalized research copies are private.
- Runtime game ID LUPIN THE THIRD  -THE SHOOTING- (two spaces). Rev A is not validated.
- Test players have distinct process names and UDP27026, owned-PID cleanup and private save/config/model roots. Harness isolation RED4 violations -> GREEN; vrcheck selftest OK. Separate Windows baseline linked1130 targets.
- Raw RAM/render-pass capture runs outside OpenXR; the simulator runs headset behavior. PrintWindow screenshots work in a visible interactive player. Inherited DebugShots produced no raw image files here; do not rely on it without further diagnosis.

## Camera

Evidence in private research runs flat-attract-original, flat-play-original and flat-route-original.

- This library stores pixel focal at0x0CBD6120, followed by width640, height480, center320/240. Setter0x8C075E40 copies the descriptor's focal/center; original guard words at075E40/42/44/46 are E00C,D504,F348,F53A.
- RAM focal:320 during setup,2408.04 in one attract camera,879.425903 in the train sequence. Last equals320/tan(7280*pi/65536), horizontal FOV39.9902deg. Scalar tracking is needed; normalized4x4 scanner does not find this camera.
- Render-state cached copies879.4259 at0x0CB7FB88/8C and0x0CBD6DF4/F8 are observations only, not approved hit-test or patch targets.
- Camera/context data around0x0CADA518 holds a view transform;0x0CADA5A0 position;0x0CADA5BC240.0 in later scenes. The16-bit7280 at0x05B788 is part of a numeric table, NOT an FOV literal: do not patch it.
- Profile tracks guarded live scalar focal. Original perspective setter at0x075E18 halves r4 in its tangent call delay slot0x075E1E (4421); guarded NOP0009 doubles the angle (train40->80deg, focal879.4->381.4). Causal train tests prove hit testing retains the original lens when drawing is widened: a rendered-head input hits the body (100 points), its stock-projected input kills the front head (500), and an input over visible sky kills the rear head (500). The guarded camera descriptor supplies the independent stock focal; see lupin-hit-marker-verification.md. Descriptor array0x9A58FC has4 entries, stride248, signed32 active index0x9A6994 and signed32 full-turn angle at+92. Range-check index0..3 and angle2..65535. Eleven code/data guards identify the accessor. Original focal=(width/2)/tan((angle>>1)*2*pi/65536), preserving the integer half-angle. Angle2756 gives2408.0402; angle7281 gives879.4260 while widened drawing remains381.4189. No descriptor is patched.
- Flat route Start60..84s reaches a playable sequence; score changes by95s. Full3D attract scene153..163s has~6k-9k vertices. CM's boot/press schedule cannot be reused.

## HUD, geometry and pistol

- UI inverse-W depths are multiples of1/300, W0.506757..0.681818 across title/train/ranking/failure. Snap band0.49..0.75, distinct plane0.6. All-flat screens retain inverse-depth spacing: linear W compression0.5..100 had collapsed neighbouring font layers and left holes. lp-hud150s passes the portrait0.534 snap and visual inspection of instructions/text.
- The library's near plane is W1.0. The19 far-out near-plane polygons in original route0001 are planar to relative residual<6e-7; preserve the real floor/walls via farOutBelowW0.999 rather than clipping them to the original screen. World textures c65800,c74800,c8a800 can be near; do not mistake them for HUD models.
- Dedicated handgun texture0xb80800 is held by train guards, not yet identified as Lupin's personal Walther. One complete rigid instance:24 strips,86triangles,52 welded vertices,12openedges, normalized extents1:.524413:.087961. Three pieces have gaps0.1306/0.1492units; other guns>10units apart. Opt-in instance join distance0.2 selects a whole single gun.
- Private Source/cache gun-lp-pistol.bin, physical barrel length0.22m, neutral preparation window<=180game seconds. lp-first-pistol fresh-cache simulator captured a complete25840byte HND1v2 at72game seconds (~15wall seconds); vrcheck confirms flagsgun-only+complete,24sourcepolys,86triangles,length0.22,muzzle and grip. Synthetic builder13/13 passes.

## Player and gun state

- Original code has float VM table0xA4C300. Guard getter0x0281EE words930A,D012,2439 plus mask0x02820607FF and table literal0x02823C8CA4C300. These are float32 variables, not CM byte/u16 fields.
- Controlled old-player state-rounds-original-v2: P1 ammo0xA4C808, magazine0xA4C818 (7), accepted shots0xA4C828; P2 corresponding+4, magazine6. Seven center shots consume7->0 and counter0->7; two empty pulls change neither. Each off-screen sensor reload replenishes without changing shots.
- Session float0xA4C728==1 distinguishes real sessions from attract; active floats0xA4C738/73C==1 distinguish each player. Lifecycle0xA4C748/74C uses1active,2continue,0out. P2 Start and life transitions causally verified in v2. Native snapshot reads finite integral counters and both session/player gates.
- Raw B128 does nothing. state-reload-original-v3: ADC off-screen with buttons0 auto-reloads; sensor2 at center and off-screen trigger also reload with no shot. Driver must hold the last valid ADC when merely pointing away in Shoot off-screen mode, and request sensor reload for B/Y or explicit point-mode pulse. Transport/calibration23/23 checks pass, including held-pull category and endpoint safety. Simulator lp-empty validates actual accepted rounds and independent reloads; exactly two empty clicks per player after a stable-scene delay. Starting VR ray pulls during the early 2D-to-3D transition can correctly suppress them; the test waits three seconds beyond ammo initialization. P1eight/P2seven real kicks match full magazines plus one after reload; no premature clicks remain with the measured200ms native input wait.
- Movie frames are inset448x288 or fullscreen flat YUV422 quads (format3), W0.54545456, observed textures0xb6d800/0xbad800. Active-player/session gates plus18/6-frame debounce select movie cutscenes; real gameplay shots veto lingering cutscene state as in CM. lp-feedback switches controllers on72.5s/off89.3s, without labels during the movie, then guns for combat. Later story coverage remains untested.

## Calibration and native markers

- Live per-player calibration table0xB67DA4, stride6: minX,maxX,minY,maxY,centerX,centerY. Original bytes P1[13,228,12,238,122,124], P2[20,240,8,232,130,118]. Changing EEPROM to a zero-min ideal table breaks off-screen reload; preserve it.
- Native JVS signed-low-byte assembly changes e.g0x7AFD to0x79FD before the integer calibration. The inverse includes this behavior and rebuilds bounded axis maps only when live calibration changes. Guard wrappers, table, mode1(640x480) and parser before applying it.
- Independent Python forward reference4521 checks; original measured28point predictions exact. Production lp-aim-wide and lp-aim-stock each16 game VM points pass: centers exact; sampled axes at most1px off, whole-screen inverse bound2px. All RAM snapshots retain original calibration bytes. Every render header has stock879.426; live381.419 wide or879.426 stock.
- Native output640/480 is inclusive and accepted for both players; strict outside643/482 reloads instead. Ten causal boundary cases preserve the original sentinel. A nearest inverse output of640 for target639 is valid.
- Red/blue duplicate crack: translucent4-vertex atlas0x92D800 quads at originalW0.5263158, drawn at unscaled gun coordinates. Three measured UV regions isolate all40 sampled marker rectangles from119 same-depth HUD rectangles. Classify before snapping and only near a recent3D shot. Keep markerWidened=false; world hit stars/effects remain.
- noShotFlash=true: no bright fullscreen UI flash in108 grid passes or9676 immediate shot-polygon entries. Broader fullscreen candidates decode as YUV movies. Preserve movie/fade layers; absence outside sampled scenes is not asserted.

## Reusable features and isolation evidence

- Native gun menu capability is independent of Source/model availability. Lupin gets in-game pistol, licensed Arcade/Zapper and user guns, retaining the CM fallback order. The Zapper is0.30m. No Lupin rifle rule is inferred from CM.
- Cache validation checks HND1v2 completeness,24source polygons, finite bounded payloads and16MiB limit. Atomic replacement keeps old valid cache until the new file is complete. Renderer and preparation use the same Lupin validator; CM's older partial-model policy is unchanged.
- Four cache runtime cases pass: Bskip with orphan.part, retry, valid-cache restart/Zapper, truncated final recovery. Captured cache25840bytes. Rejected-final renderer regression7/7 covers incomplete flags, NaN muzzle, truncation, wrong source count, missing, valid and CM policy.
- P2 prompt atlas0x92D800 uses two six-glyph messages. Suppression box437.5,440.5..617.5,464.5 and min8x8 (native640x480), only when the known VM player mask is exactly1 and finite P2lifecycle0 (out), preserving active/continue text. Do not hide trailing polygons. Offline352pass corpus validates both messages and shared-atlas feedback separation. All prior210prompt cases remain selected; two observed lifecycle2 captures have four ungated glyph matches each and zero matches with the lifecycle guard. Mixed P1-alive/P2-continue timing remains an explicit coverage gap.
- Actual game state produces P2 join and leave, not the visual prompt. lp-empty logs the real P2 start,7-round/6-round magazines and one-gun return on P2 loss.
- Lupin UI suite10/10: all12 rows/values, Touch labels/How to play, bindings, Index/Vive/WMR, both pointers with laser off, Quit timeout/confirmation, second-launch persistence and gameplay pause/focus/head redraw. Frozen image diffs0; head-yaw diff7.226; resumed images differ8.410/53.234. Startup BIOS blank frames are distinguished from gameplay rendering.
- Inherited private-ROM HOTD2/MOK and CM menu/pause regressions pass. CM pause23 cycles,10captures, frozen diffs0, head-yaw21.325; harmless owned sentinel survived player cleanup. Both harnesses use private ports, owned PIDs, checked resolved output/fixture paths. Original CM checkout is never mutated.

Current artifact and outstanding physical checks are in lupin-validation.md and lupin-vr-headset-checklist.md.

Point-mode integration: lp-reload-point145s passes P1three/P2two automatic reloads, expected relative exit times6.104/8.571/10.387 and3.049/7.883s,3real kicks each, full-magazine suppression, paused-menu suppression, manualP2Y and clean held-exit behavior. As in CM, Point mode intentionally reloads on an armed exit even while held, then waits for trigger release. Shoot mode preserves the initial held-pull category.
