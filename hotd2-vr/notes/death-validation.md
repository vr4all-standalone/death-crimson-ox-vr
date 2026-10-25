# Death Crimson OX VR validation

2026-10-25. Experimental Japan Rev A port, based on Lupin `f67026046` (including the final Confidential Mission sync). Local research and simulator checks do not establish physical headset performance or full-game completion.

## Supported data and identity

- Japan Rev A, resolved as `deathcoxj`, program `epr-23524a.ic22`, CRC `b60e113e`, plus ten 8 MiB MPR chips and a supported NAOMI BIOS. No CHD.
- Quest application `com.vrlightgunhub.deathcrimsonox`, label **Death Crimson OX VR**, version 1.0.0 with source commit suffix.
- Setup streams and checks actual chip/BIOS lengths and CRCs. The standalone native loader additionally checks the resolved set, cartridge layout and actual bytes of every game chip. The original Japan program shares the boot title and is deliberately rejected.
- Distribution checks exclude ROMs, BIOS, saves, research data, generated native models and signing material. Both packages retain licenses and the same credited model assets.

## Measured integration

The native perspective builder at `0x0B8454` uses focal `554.29718`, stored at `0x86A588`. Eight code words guard the optional wider-view change at `0x0B846C` (`4421 -> 0009`), removing the half-angle operation. The live focal becomes about `184.8`. Native hit polygons are projected through the same live focal (`0x059FD8 -> 0x0AE45C`), so aiming must not apply an extra inverse lens correction.

HUD layers have W=1,2,3 and inverse depth 27/18; the world near clip is 20 game units. Separate prompt textures `A90800` and `A99800` were decoded as PRESS START BUTTON and FREE PLAY and bounded in the P2 corner. A captured human mesh is about 131 units tall; unitScale .12 at world size .11 gives about 1.73 m. These are desktop reconstruction measurements; physical scale and comfort need headset checks.

Native player blocks start at `0x18BFC8`, stride 60. Ammo, active state, health, accepted-shot counts and unsigned ADC calibration have guarded readers. The game decreases ammo one native update before incrementing the accepted-shot counter. Recoil therefore follows accepted-shot deltas directly for this game; empty pulls do not increment that counter. See `death-native-state.md` for signatures, calibration, cutscene table traversal and evidence.

Cutscenes use the guarded native scene resolver and active-player mask with inherited 18-on/6-off frame debounce. The controlled route enters movie mode about 35.7 s and leaves about 59.5 s. Guns disappear during the movie and return in interactive play.

## Automated checks

| Check | Result |
| --- | --- |
| Actual hand/menu/INI/label/input functions | 298 checks, 0 failures |
| Native gun, ADC mapping and scene bounds | 0 failures; every 640x480 axis target checked within one pixel |
| Delayed native accepted-shot feedback | 11 checks, 0 failures; delayed-counter regression observed failing before fix |
| Actual gunChoices, licensed/custom models, missing-file fallback | 21 checks, 0 failures; empty-choice regression observed failing before fix |
| Standalone policy, ON/OFF/explicit zero, private real-chip mutations | 109 checks per mode, 0 failures |
| Java setup/status including private owned ROM fixture | 28 tests, 0 failures |
| Synthetic distribution validation | 43 checks, 0 failures |
| Inherited model builder/cache | 16 cases, 0 failures |
| Inherited gun input/ADC transport | 23 cases, 0 failures |
| Renderer-analysis fixtures | selftest OK |

`death-ui-accept.ps1` covers the 13-row menu, both licensed gun choices, angle/reload/hand/laser/haptic/world/panel/wider-view changes, persistence across a fresh restart, controller labels, both pointer hands, How to play, Resume, and timed Quit confirmation. Touch, Index, Vive and WMR simulator profiles passed. The first menu fixture expected Lupin's third native-gun choice; corrected to Death Crimson's two licensed models after checking the actual menu logs and saved model stem. The product menu itself required no correction.

Gameplay acceptance: `dc-magazines-accept` ran 134 seconds: P1 accepted 14 rounds with 14 recoil kicks; P2 accepted 13 with 13 kicks. Both reached zero ammo, produced an empty click, reloaded with their assigned button, and fired again. P2 joined right-handed while P1 remained left-handed. P2 left while its right trigger was held: no P1 shot leaked before release; fresh right then left solo pulls transferred and fired correctly. Native movies entered/exited. Menu screenshots had mean RGB difference 0.0 while stationary, 33.28 after head rotation, and resumed gameplay differed by 18.95. System-overlay focus loss preserves the inherited behavior: controller inputs release and guns hide while emulation continues; focus return restores controls and recenters after a long absence. It does not automatically open the VR pause menu.

## Review and limits

Independent source review found a held P2 trigger could transfer to solo P1 on the following frame, and the common boot title alone did not reject the unsupported original Japan program. Both were reproduced in tests and corrected. Release latches now track physical controllers through join, leave, live swap and restart; the loader verifies the exact native chip contents. Parent inspection also covered the later scene adapter and missing-model fallback.

The ten-capture, 220-second neutral attract investigation did not establish a complete native protagonist pistol. No generated native-pistol choice or preparation wait is offered. The credited Arcade gun and Zapper are included, custom GLB models remain supported, and a built-in Arcade gun covers missing asset folders. See `death-gun-findings.md`.

Private RAM/VRAM/geometry and screenshots remain under ignored build directories. Diagnostic runs that missed Start, reset play repeatedly, or missed short simulator pulses are not passing acceptance evidence. The controlled magazine experiment disables only the original game's damage-induced ammo refill at `0x03CAF6` and raises native health, solely through harness POKE actions. That patch is absent from the production profile, shipped settings and binaries; ordinary damage/refill behavior remains intact in the product.

Physical Quest setup/permission flow, tracking, haptic feel, frame rate, comfort, longer sessions and all later routes/bosses remain untested. Use `death-headset-checklist.md` for these remaining checks.
