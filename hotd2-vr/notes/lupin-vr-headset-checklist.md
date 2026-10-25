# Lupin the Third VR: physical headset checklist

All items below remain untested on a physical headset. Simulator checks and an APK build do not establish tracking quality, comfort, sustained frame rate or the feel of the guns.

Use the final packaged Lupin build recorded in the validation report. App ID: `com.vrlightgunhub.lupin`; settings: `/sdcard/Android/data/com.vrlightgunhub.lupin/files/emu.cfg`; generated pistol: `files/data/gun-lp-pistol.bin`. Back up settings and the generated pistol before testing cache recovery. Keep game files and logs private.

## Setup and lifecycle

- [ ] Lupin has its own library entry and opens beside Confidential Mission without changing CM's files or settings.
- [ ] Original `lupinshoo.zip`, `lupinshoo/gds-0018.chd`, and `naomi.zip` produce READY and individual file badges. Missing files and access restrictions name the required fix, disable Play and recover after Look again.
- [ ] Text, marks and the Lupin icon render correctly. Instructions expand and fit the panel; Play enters VR without a second permission prompt.
- [ ] Quit returns to setup; Play starts a fresh game. Home, focus loss, headset sleep and repeated launches leave no duplicate game process or stuck trigger.

## First start and pistol cache

- [ ] With no pistol cache, the violet preparation panel watches the attract demo, shows progress, then loads one complete handgun. Record real headset elapsed time: ______.
- [ ] The pistol has one barrel, grip and muzzle, with no enemy body or additional floating gun. The current captured gun is the train guards' in-game handgun; no claim is made that it is Lupin's personal Walther.
- [ ] B skips preparation, leaving the Arcade gun for this session. A later launch retries. A complete cache skips preparation.
- [ ] Interrupted preparation and an orphan `.part` do not count as a completed cache. A truncated final cache regenerates; a good final cache survives an interrupted replacement.

## Controllers and labels

- [ ] Before play, both controllers and the labels are readable at arm's length. Right: A P1 Start, B P1 Reload, trigger P1 Shoot, stick Laser. Left: X P2 Start, Y P2 Reload, trigger P2 Shoot, stick Recenter, Menu.
- [ ] Hands set to P1 left assigns X/Y to P1 and A/B to P2. Labels change for both hands; Trigger is above Stick where present. The choice survives relaunch.
- [ ] Label endpoints meet the physical controls. Custom controller GLBs and metadata override the bundled models correctly; a missing side falls back independently.
- [ ] Before P2 joins, firing either trigger puts P1's gun in that hand. After joining, the Hands setting assigns P1/P2. Switching it preserves each player and magazine, and a held trigger waits for release.

## Paused menu

- [ ] The thirteen rows appear in order: Resume, Recenter, Gun, Gun angle, Reload, Hands, Laser, Haptics, World size, Panel distance, Wider view, How to play, Quit.
- [ ] Either pointer/trigger and stick navigation work, including repeat. Opening the menu pauses both game simulation and audio while head movement remains live.
- [ ] Resume, B, rapid menu toggling, focus changes and recentering produce no time jump, fired round, unintended reload or stuck input.
- [ ] With Laser off, menu pointers remain visible. How to play shows controllers/labels and restores the guns on closing.
- [ ] Quit needs a second selection within three seconds; an expired confirmation requires arming again.

## Guns and scale

- [ ] In-game pistol, Arcade gun, Zapper and user GLBs cycle and preview while paused. Zapper is 0.30 m long. Missing or invalid custom choices fall back safely.
- [ ] Saved choices are case independent and survive relaunch. A user file overrides its matching bundled gun; unrelated files remain available.
- [ ] The gun is held by its grip; laser, dot, flash and shots follow its muzzle. Both hands mirror correctly.
- [ ] Gun angle changes by one degree and stops at -30/+30. Shots still follow the barrel at both limits and after changing the selected gun.
- [ ] Grip plus stick changes world and gun scale as labelled. World size, gun scale, panel distance and recenter feel comfortable. Record preferred settings: ______.

## Aiming, camera and image

- [ ] Test center, edges and corners with both players, several head poses and changing game zoom. The shot lands on the dot; the grey margin produces neither a shot nor reload.
- [ ] Turning and moving the head reveals reconstructed stereo 3D. Wider view on/off preserves accurate hits and shows the intended extra scenery without missing enemies.
- [ ] Floor and walls remain intact looking down and sideways. No near-plane streaks, floating floor wedges or clipped ground appear in train scenes and later stages.
- [ ] HUD, magazines, subtitles, instructions, score/ranking and continue notices stay readable on the panel. No missing glyph blocks, bent text or floating off-screen notices.
- [ ] Native shot markers do not duplicate VR impacts in the world. Real scene fades, damage effects and story transitions remain intact.

## Fired rounds and reloads

- [ ] P1 fires seven rounds and P2 six. Every accepted round kicks/flashes once in the correct hand. Empty pulls give a light click and no kick/flash; reloading gives no firing feedback.
- [ ] B and Y reload only their assigned player. Haptics Off/Light/Medium/Strong behave as labelled; Off retains visual recoil/flash.
- [ ] Shoot off-screen mode reloads only on an off-screen pull. Merely pointing away does not reload.
- [ ] Point off-screen mode reloads once per exit after returning on-screen; a full magazine, menu/focus transition and the grey ring do not reload.
- [ ] In Shoot off-screen mode, start a pull on-screen, keep it held, sweep into the grey ring and fully away: no extra shot or reload. Start a reload away, keep it held and sweep back: no shot. Point off-screen mode intentionally reloads on an armed exit even while held, and waits for release before another shot.
- [ ] Supported floor/physical-slide reload behavior matches the selected mode; single and dual wield remain independent.

## P2 and story scenes

- [ ] X joins the actual game with a left gun and separate aim color. Y, trigger and feedback affect P2 only. The single-player start prompt is hidden without hiding P2 HUD once joined.
- [ ] On P2 loss/continue expiry, one gun returns; P1 can choose either hand again. Joining and leaving during scene transitions is stable.
- [ ] Noninteractive movie scenes leave both hands empty; guns return as gameplay resumes. Skipping and accepted shots end that state promptly, with no flicker during combat.
- [ ] Complete later stages and note any additional weapon or scene behavior requiring a game-specific adaptation: ______.

## Persistence, performance and PC VR

- [ ] Relaunch preserves all menu settings in Lupin's own configuration. CM settings and generated models remain unchanged.
- [ ] Play at least 30 minutes, including busy scenes, rapid shooting, both players and head motion. Record headset model, refresh rate, sustained frame rate, thermal behavior and loading stalls: ______.
- [ ] The PC portable launcher opens this folder's original Lupin set, uses its own settings, enters the active OpenXR runtime and quits cleanly.
- [ ] Repeat on other claimed headset/controller combinations before recording those combinations as verified.

For any failure, record the build version/commit, scene, hand, steps and private log path. Mark only checks actually performed; leave all others unchecked.
