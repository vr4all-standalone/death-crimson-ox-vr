# Lupin prompt and model verification evidence

Numeric observations only. Game RAM, render passes, textures and the extracted pistol remain private and ignored.

## P2 prompt selector

The original local disc revision uses shared atlas address `0x92d800`. At native 640x480, the two observed messages occupy these six-quad boxes:

| Message | Full screen bounds | Quad widths | Quad heights |
| --- | --- | --- | --- |
| Join midway | 449,442 to 612,464 | 13 or 30 | 20 to 22 |
| Insert a coin | 438,441 to 617,462 | 29 or 30 | 19 to 21 |

Profile bounds are `{437.5,440.5,617.5,464.5}`; minimum quad size is 8x8, both scaled by framebuffer dimensions. Native W is 0.51724136. The renderer snaps this near HUD to the profile's 0.6 plane before suppression. The atlas packs glyphs at different UV coordinates in different scenes, so a fixed UV whitelist is unsuitable.

Use only known player mask 1 (session/P1 active/P2 inactive), and disable trailing-rectangle suppression. This preserves waiting text and joined-P2 feedback on the shared atlas. Five P1-only captures include an additional 3x1 red polygon at 513,457 to 516,458, native W 0.5263158; minimum size rejects it.

The 2026-10-25 audit snapshot contains 318 passes: 211 P1-only, 210 with exactly six qualifying glyph quads and one with none. These split into 132 join and 78 coin messages. Texture+box alone overlaps feedback in 43 joined-P2 passes; all observed feedback overlaps fail the 8x8 minimum. Both-inactive waiting screens need the player-state gate. There are no P1-active/P2-continue captures, so that transition is not proven by this corpus. Counts grow as new private captures arrive.

Private reproducible captures (paths relative to their respective `build-win/vrtest` roots):

| Root | Run/pass | Log time from process start | State | Observation |
| --- | --- | --- | --- | --- |
| Lupin clone | `lp-hud/rip/pass0000.bin` | 84.012 s | session 1, players 1/0, lifecycle 1/0 | Prompt absent |
| Lupin clone | `lp-hud/rip/pass0001.bin` | 105.167 s | session 1, players 1/0, lifecycle 1/0 | Join message, six quads |
| Lupin clone | `lp-hud/rip/pass0002.bin` | 125.016 s | players 0/0 | Waiting text; retain |
| Research clone | `state-rounds-original-v2/rip/pass0001.bin` | 89.737 s | players 1/0, lifecycle 1/0 | Join message, six quads |
| Research clone | `state-rounds-original-v2/rip/pass0002.bin` | 94.758 s | players 1/0, lifecycle 1/0 | Join message, six quads |
| Research clone | `state-rounds-original-v2/rip/pass0004.bin` | 106.036 s | players 1/1, lifecycle 1/1 | P2 feedback; retain |
| Research clone | `aim-grid-original-ideal-rip-v3/rip/pass0008.bin` | 91.948 s | players 1/0, lifecycle 1/0 | Coin message extends beyond initial join-only bounds |
| Research clone | `aim-grid-original-stock-rip-v2/rip/pass0026.bin` | see run log | players 1/0, lifecycle 1/0 | Join message plus 3x1 effect |

Audit with `re/lupin_p2_audit.py --capture-root <research-vrtest> --capture-root <clone-vrtest> --output <ignored-json>`. It reads the latest RAM dump at/before each pass. Float state offsets are session `0xa4c728`, players `0xa4c738`/`0xa4c73c`, lifecycle `0xa4c748`/`0xa4c74c`. Output contains state, quad bounds/UV/depth and counts only.

## Model fixture and runtime recovery

`re/lupin-model-fixture.ps1` defaults to Plan. Install backs up executable-directory guns/controllers and writable-data bundled models, then copies the licensed `hotd2-vr/models` files into `build-win/data/models`. It verifies copy hashes and rejects reparse-point targets. Restore retains the fixture for diagnosis and restores all original directories. No process is started or stopped by this fixture.

`re/lupin-cache-runtime.ps1` also defaults to Plan. After obtaining the exclusive clone runtime slot, use `-Run -Games <private-original-set-folder>`. It installs its own fixture, preserves the original final cache and any `.part`, exercises skip/retry/valid restart/truncated replacement, and restores state in `finally`. Never overlap another harness: executable, config and cache paths are shared even when UDP ports differ.

The 2026-10-25 preliminary runtime used the private equivalent scripts with the compiled binary before P2 suppression was added:

| Run in clone `build-win/vrtest` | Result |
| --- | --- |
| `lp-cache-skip-real` (86 s) | Valid orphan `.part` ignored; B at 2 s skipped after 7 game seconds; licensed Arcade fallback; no final generated during the run |
| `lp-cache-retry-real` (35 s) | Next process prepared again; 25840-byte complete pistol saved at 72 game seconds/~14.6 wall seconds; `.part` consumed; in-game pistol selected |
| `lp-cache-valid-zapper` (7 s) | Valid cache logged made-before, no preparation; actual bundled Zapper loaded and visually rendered (`t004-zapper-real.png`) |
| `lp-cache-corrupt-real` (35 s) | Four-byte `HND1` final rejected; 25840-byte final replaced it at 72 game seconds/~14.8 wall seconds; no `.part` left |

The first skip run had an incorrect expected-log regex on the source-root line; manual readback and the corrected follow-up verified the actual bundled model lines. Its hidden screenshots were unavailable. Retry/corrupt screenshots were during the completion panel, so their pistol selection is log evidence; capture at least 5 s after `VR hands: done after` for the actual pistol visual. The checked-in runner uses visible windows and that later capture, and explicitly requests the automatic gun selection with `vr.GameHands=yes,vr.Gun=0`.

Licensed model readback: Namco 17108 triangles/2 textures, Zapper 3328/5, each Quest controller 5609 triangles with sidecar label positions. After the four checks, original cache SHA-256 matched, original directories were restored, and no clone player remained. A subsequent fixture was deliberately left installed for root's remaining UI tests; its private manifest/restore command were handed to root.

Fresh standalone command: `re/hands_build_test.cmd build-win/vrtest/lp-model-unit-fresh`; 16 passed, 0 failed. It covers 13 mesh/selection/isolation cases and 3 bounded-cache/truncation/replacement groups, using wholly synthetic meshes and pixels.

The Lupin cache validator and HND1 consumer share a payload layout: both use the same 23-float header, texture layout, four mesh slots and 36-byte vertices. Lupin additionally requires complete gun-only v2 data, the expected polygon count, bounded counts, finite data and exact EOF before treating a final cache as reusable. Other games retain their existing cache policy. The native P2 guard matches the measured texture, widened bounds, scaled minimum size and known-mask policy; the unobserved continue transition remains a coverage limit.

## Renderer rejection regression

A subsequent review found that preparation validation alone did not protect the renderer: the generic HND1 loader could still accept a structurally valid incomplete final or a NaN muzzle header after B skipped preparation. Lupin's `gunModel` entry now checks the same `loadGunCache` policy before entering that loader; CM keeps its existing partial-model policy.

`re/lupin_render_cache_test.ps1` extracts the actual `gunModel` definition verbatim and compiles it with production Source/GunKind/cache validation and a small platform/renderer dependency seam. All payloads are generated synthetic HND1 data. Before the guard, 2 cases passed and 5 failed; afterward, all 7 passed: complete Lupin accepted; incomplete, NaN muzzle, truncated, wrong-count and missing Lupin final blocked before the generic parser; incomplete CM still admitted. The existing 16 builder cases also passed again.

For the runtime regression after obtaining a rebuilt exclusive slot, use `re/lupin-cache-runtime.ps1 -Run -Games <private-original-set-folder> -RendererRejectedOnly`. It mutates independent copies of a previously complete final to flags1 and NaN-muzzle variants, presses B in each, and requires the licensed Arcade fallback without an in-game-model load. A final valid-cache restart captures the in-game pistol. The three cases take about 21 seconds plus process setup/cleanup; the script restores cache and model folders in `finally`.
