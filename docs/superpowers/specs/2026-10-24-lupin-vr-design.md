# Lupin the Third VR: Confidential Mission 1.2 feature parity

Date: 2026-10-24 (America/Vancouver).
Status: approved by the user in chat, 2026-10-24: Proceed with full feature parity.

## Intent and source of truth

The user wants Lupin the Third: The Shooting converted into a standalone Quest VR game, with all the menus and features developed for Confidential Mission VR. This continues their series of Naomi lightgun conversions. They authorized reading their ROMs and creating another folder under E:\___flattovrdev. Confidential Mission is currently running final tests and must remain undisturbed.

Reference source: E:\___flattovrdev\hotd2-vr, branch naomi-gun-vr, inspected at e6bd81a97. Its working tree has additional changes to README.md, hotd2-vr/package-release.ps1 and shell/android-studio/flycast/build.gradle.kts, plus a new headset checklist and a locally modified gamesdk submodule. The reference is the finished 1.2 behavior, not the older upstream HOTD2 app. Record the exact source commit and relevant final working changes used before starting implementation. Do not alter, reset, clean, stop processes in, or run tests from that checkout.

## Product and technical choices

My implementation choices, open to correction:

- Separate source checkout E:\___flattovrdev\lupin-vr on branch codex/lupin-vr. Use an independent local clone/snapshot because the native worktree tool is attached to the unrelated VR4ALL repository. Do not share writable build directories, emulator config, saves, generated models, Gradle project caches, or test outputs with Confidential Mission.
- Same C++17 Flycast/hotd2-vr engine, OpenGL/OpenXR, native Android Java setup activity, Gradle Android build, Windows PC VR build, Python/PowerShell research tools. Preserve upstream attribution and licenses.
- App name Lupin the Third VR, Android application ID com.vrlightgunhub.lupin, initial version 1.0.0 (versionCode 1). Dist outputs lupin-the-third-vr-quest.apk and lupin-the-third-vr-pcvr.zip. Installs alongside Confidential Mission with separate settings and saves.
- Preserve the original Lupin game's rules, story and weapons. Features tied to a specific weapon in Confidential Mission translate to the equivalent weapon behavior actually present in Lupin. Do not insert Confidential Mission's assault rifle or its pickup rules into Lupin.
- Quest 3 is the primary target. Keep Quest 3S/2/Pro and PC VR compatibility from the reference; state untested headset combinations honestly.
- No installer/catalog work in VR4ALL, no multiplayer/networking expansion, no publication, GitHub release or headset installation in this scope.

## Feature parity contract

1. Actual stereo VR reconstruction of the game's 3D scenes, tracked head rotation and translation, tracked controller guns, and the same comfort settings. This is a port of the emulator rendering path with a Lupin profile, not a flat screen attached to the head.
2. Lupin's own camera/projection and viewport discovery, live focal tracking through attract, intros and play, world-unit scale, and wider-view option. Widen the game's drawing/culling while preserving its stock hit-test framing. Every RAM patch must have evidence and guards for its intended revision. Never reuse CM addresses, calibration or texture IDs.
3. Correct aiming at center, corners and edges; neutral calibration for both players; original hit/shot markers removed where appropriate; grey boundary ring for visible geometry beyond the original hittable view. Verify aiming at multiple head poses and zooms. Preserve original game effects and scene fades.
4. Stable 2D HUD, subtitles, notices, game menus and continue screens on the adjustable panel. Correct near-plane clipping, offscreen layers, sights, letterbox treatment, and floor coverage based on Lupin captures. Do not assume CM's depth bands fit Lupin.
5. The same 12-row paused VR menu, in the same order: Resume, Recenter, Gun, Gun angle, Reload, Laser, Haptics, World size, Panel distance, Wider view, How to play, Quit. Preserve row values, ranges, stick repeat, point-and-trigger selection with either hand, stick navigation, live gun preview, pointer visibility with laser off, and the 3-second second-selection Quit confirmation. Pause game simulation and audio while keeping headset rendering/head movement live. Focus loss, rapid open/close, recenter and resume must behave as in CM.
6. Same controls: right A P1 Start; right B P1 Reload; left X P2 Start/join; left Y P2 Reload; triggers fire for the assigned player; before P2 joins, either trigger fires P1 and the gun follows the firing hand. Left menu opens settings, left stick click recenters, right stick click toggles laser, grip+stick up/down adjusts world size, grip+stick left/right scales the gun, stick drives game D-pad. Preserve Index/Vive/WMR controller profiles and their labels/hold behavior.
7. Gun choices: Lupin's own in-game pistol, Arcade gun, Zapper at the CM 1.2 size (0.30 m), then user GLB guns with sidecar metadata. Preserve bundled/user override rules, case-insensitive persistent selection, handed mirroring, gun scale, muzzle alignment, custom controllers, and fallbacks (in-game pistol -> Arcade gun -> hidden built-in gun). Generalize the current CmGuns gate; never load a cached CM model for Lupin.
8. Generate Lupin's own gun models from the player's local files on first run, with progress, B skip, cancellation, atomic cache writes and next-start retry. Research suitable scenes and extraction criteria; do not label the gun feature complete using a borrowed CM model or an unverified fragment. Ship only the independently licensed models and their credits. Report first-run duration and any incomplete model explicitly. If Lupin has additional weapons, provide the reference's appropriate model/feedback behavior using evidence from Lupin.
9. Haptics, kick and muzzle flash follow actual fired rounds for each player; empty pulls and reloads do not produce a firing kick/flash. Preserve Off/Low/Normal/Strong including menu samples. Automatic weapons, if present, get per-round feedback while a chosen file gun stays selected. Discover Lupin's ammo, shot count, weapon and playing state instead of inferring success from trigger pulls.
10. Both reload modes: Shoot off-screen (default) and Point off-screen (one reload per exit, rearmed on reentry), independently for both hands. Preserve B/Y and any supported physical slide reload, full-magazine/menu suppression, the grey ring distinction and floor reload behavior. Adapt low-level button signaling to Lupin's actual I/O semantics.
11. Dual wield through the game's real P2 join/leave lifecycle. Right hand is P1, left is P2; separate aim colors, player state, feedback and reloads. Hide only P2's start prompt in single-player play. Reliable return to one gun when P2 leaves. Free play/no coin dependency, as in the reference.
12. Controllers before play and for How to play, with the same button label wording and placement; controllers without labels during noninteractive in-game cutscenes, guns on return to play. Find a stable Lupin scene signal, test skip boundaries and shots accepted by a scene, avoid heuristic flicker.
13. Same VR4ALL-style Android setup screen: dark rounded panels, cream text, lime/violet accents, Lupin branding, READY/MISSING FILES/ALLOW ACCESS, individual file badges, precise paths, access instructions, Look again, Play disabled until ready. Check actual Lupin files, bounded recursive discovery, case handling, BIOS inside the game ZIP or beside it, supported CHD/CUE/GDI behavior, and relaunch/quit to setup. Both app and engine enforce Lupin selection. Do not display CM's instructions or names as Lupin UI.
14. Persistent settings in the app's independent emu.cfg; portable PC launcher and correct defaults; credited models and license notices; distribution checks exclude game data, signing secrets, screenshots and generated gun caches; APK package/label/version/launcher and archive contents are validated.

## Verified input facts and unresolved game research

Local source ROM directory: E:\___flattovrdev\confidential mission\Roms (read originals in place).

- lupinsho.zip exists and contains 317-0325-jpn.pic (16,384 bytes).
- lupinsho/gds-0018.chd exists (209,405,716 bytes).
- naomi.zip exists beside the game ZIP.
- Flycast's database defines lupinsho (Rev A) with gds-0018a; lupinshoo (original) uses gds-0018 and the same PIC definition. The local layout therefore needs revision verification. A name alone does not prove disc contents. Do not modify the original names to hide a mismatch. Determine the correct set using CHD metadata and a successful boot. If necessary, create a separate local research layout using the matching set name; no game data goes in the tracked source.
- The engine already routes game ID LUPIN THE THIRD  -THE SHOOTING- through the Naomi analog lightgun board, like CM. Confirm exact runtime ID before adding the VR profile. Both revisions must be distinguished by content/code guards when their memory differs.
- The default Lupin EEPROM is annotated as calibrated; this does not prove VR aiming accuracy.
- No Lupin camera, FOV patch, HUD depth map, weapon model textures, shot/ammo fields, P2 prompt detection or cutscene signal exists in the inspected VR profiles. These are research work, not known constants.

The builder decides exact guarded patch strategy, research timings, generic interfaces and test fixtures based on measured evidence. Start with the supplied original disc revision; do not claim support for Rev A without checking it. Missing second-revision data does not block the supplied revision.

## Research player and coexistence

The existing Windows flycast.exe implements a full OpenXR simulator (vr.Xr=yes, vr.XrSim=yes): stereo 832x720 eyes, 64 mm IPD, Quest-like FOV, 72 Hz, simulated controllers and focus states, left eye in the desktop window. UDP commands drive XR inputs, head/hand poses and focus, alongside normal lightgun commands. Existing tools capture raw game frames, render-pass rips, textures and RAM, and inspect SH4 code/camera matrices. Use these to learn and test the port.

CRITICAL: vr-test.ps1 currently calls Get-Process flycast | Stop-Process -Force and hardcodes UDP 27016. accept.ps1 also temporarily moves model folders under VRLGH_GAMES. Do not invoke either unchanged while CM's session may be running. The Lupin harness must own/stop only its launched PID, use a separate port (my call: 27026), isolate configuration and save files, and avoid moving shared ROM-folder models. Never kill unrelated emulator processes or modify the other checkout. A copied, renamed research executable with independent writable paths can be used for reconnaissance without product-source changes. Keep launches hidden unless an interactive window is necessary to observe the game.

## Implementation stages and acceptance

A. Freeze/reference audit; independent checkout/source provenance; safe isolated runtime harness; verify supplied disc revision; obtain boot, attract, intro and gameplay timeline and captures.
B. Lupin camera profile, guarded view widening, correct stock aiming/calibration, HUD/depth/markers/floor; flat-vs-VR comparison and multi-pose shots.
C. Generalize only the CM-gated reusable features; Lupin state for actual shots, reloads, P2, weapons and cutscenes; first-run extraction/cache of Lupin models. Run synthetic fixture tests plus simulator scenarios proving actual game behavior.
D. Standalone Lupin setup activity/file validation, branding/identity/default launch, icon, version, PC launcher, packaging and documentation. Preserve inherited model credits.
E. Build Windows and Quest; run Lupin simulator tests (boot, camera, widened/off, aim grid, HUD/flat screens, floor, menu/pause/focus/quit, all controls/controller profiles, persistent settings, gun choices and custom model overrides, gun extraction/skip/cache, feedback/empty/reload/P2, cutscenes). Run inherited CM/HOTD2/MOK engine regressions from the isolated checkout against independently managed model fixtures/paths. Never borrow stale passing logs as verification of new code. Validate APK/zip identity and contents. Write a Quest headset checklist for physical feel, scale, tracking, sustained frame rate, load time and busiest scenes. Simulator success is not proof of physical headset success.

Deliver source with measured findings, the per-feature validation results and local build outputs when verified. List any outstanding parity item explicitly; a successful boot or an APK alone is not full feature parity. No feature may silently degrade to trigger-based feedback or a locked Gun row while being reported as matching CM.

## Data and repository constraints

Read the user's game files only for local testing and extraction. ROMs, CHDs, PICs, RAM dumps, screenshots, disassembly listings, generated game models and saves stay in ignored local research/build data, never committed or packaged. Findings record addresses, numeric values and evidence/run names only. Do not copy, move, delete or print the contents of C:\Users\ege19\.vrlightgunhub or its signing secrets. Existing signing configuration can consume it without exposure if release packaging later requires it. Do not change or stage core/deps/gamesdk. Do not push/publish or install onto the headset without a user request.

