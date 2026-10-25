# Death Crimson OX VR

Experimental standalone Quest and Windows PC VR conversion of the NAOMI arcade game, based on the Flycast/OpenXR work used for Confidential Mission VR and Lupin VR.

The game is reconstructed in stereo with tracked head movement and controller aiming. Includes the paused VR menu, recentering, gun angle, two reload modes, laser, haptic levels, world size, panel distance, wider view, controller labels, and custom GLB gun/controller support.

## Game files

Supply your own **Japan Rev A** cartridge set:

```text
Download/
  deathcoxj.zip
  naomi.zip
```

No CHD is used. The game ZIP must contain the verified `epr-23524a.ic22` program and ten `mpr-23514.ic1` through `mpr-23523.ic10` chips. A supported NAOMI BIOS can be in the game ZIP or `naomi.zip` beside it. Setup verifies the actual lengths and CRCs, including the BIOS. The native standalone loader also validates every game chip before emulation.

Older merged sets may be named `deathcox.zip` while containing Japan Rev A. Make a separate copy named `deathcoxj.zip`; renaming alone does not make incompatible contents valid. The current USA set uses a different program. The original Japan program is also unsupported in this build.

The downloads contain no game or BIOS data. Generated game models and research captures are excluded.

## Controls

**Swap P1/P2 is enabled by default.** In two-player mode P1 uses the left controller and P2 the right. Turn it off to reverse that assignment. In single-player, pressing either trigger moves the gun to that hand, independently of this option.

| Control | Default action |
| --- | --- |
| Either trigger, single-player | Move the gun to that hand and fire P1 |
| Left X | P1 Start |
| Right A | P2 Start / join |
| Y / B | Reload; either works in single-player, each player's own in two-player |
| Left Menu | Pause / VR settings |
| Left stick click | Recenter |
| Right stick click | Toggle laser |
| Grip + stick up/down | World size |
| Grip + stick left/right | Gun size |

With the swap option off, A/B belong to P1 and X/Y to P2. If only one controller is tracked or joining is unavailable, either Start button can start P1. Held triggers must be released before firing after joining, leaving, swapping, or restarting.

Shoot off-screen is the default reload mode; Point off-screen is selectable. Native damage can also refill the magazine as part of the original game's rules.

## Guns

The credited **Arcade gun** and **Zapper** are included. Custom GLB guns and sidecar settings use the inherited model-folder workflow. A built-in Arcade gun remains available when model files are missing.

No complete native player pistol was verified across the captured story/attract scenes. This build therefore has no generated native-pistol option or first-run extraction wait. It does not reuse another game's extracted gun. The investigation is recorded in `hotd2-vr/notes/death-gun-findings.md`.

## Builds and validation

Windows: `hotd2-vr\build-win.cmd Release ON`. Use `OFF` only for isolated research/regressions. Quest: `hotd2-vr\build-quest.cmd` with a configured JDK and Android SDK. `hotd2-vr\package-release.ps1` validates identities, native policy, source commit, licenses, model credits, and archive contents.

Outputs are `dist/death-crimson-ox-vr-quest.apk` and `dist/death-crimson-ox-vr-pcvr.zip`. Run the PC archive's `Play Death Crimson OX.cmd`; put the same owned ZIPs in its `roms` folder. On Quest, open Death Crimson OX VR under Unknown Sources and follow its file-access/setup screen.

See `hotd2-vr/notes/death-validation.md` for the current evidence and limitations. Simulator results do not establish physical headset tracking, comfort, frame rate, or later-stage coverage.

## Credits and license

Flycast contributors; mikermak's HOTD2 VR/OpenXR work; Confidential Mission VR and Lupin VR adaptations. Preserve `LICENSE`, `README.flycast.md`, and the distributed license/model-credit notices. Bundled models retain their own licenses. This unofficial project is not affiliated with the game publisher or Meta.
