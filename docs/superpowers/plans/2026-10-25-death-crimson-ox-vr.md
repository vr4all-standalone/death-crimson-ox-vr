# Death Crimson OX VR Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans for research/runtime integration; independent setup and hand-policy tasks may use the dispatching-parallel-agents skill. Steps use checkbox syntax.

**Goal:** Deliver a separate Death Crimson OX Quest and PC VR game with existing feature parity and the revised dual-hand option.
**Architecture:** Independent clone of Lupin f67026046. A game profile supplies measured camera and native-state behavior; existing OpenXR rendering/menu infrastructure is reused. Android setup and standalone policy independently validate the supported arcade set.
**Tech Stack:** C++17, OpenGL/OpenXR, Java, Gradle, MSVC/CMake, PowerShell/Python.
**Spec:** docs/superpowers/specs/2026-10-25-death-crimson-ox-vr.md

## Global Constraints
- Do not mutate other ports or stop their processes.
- Default two-player assignment P1 left / P2 right. Single-player either-trigger hand switching stays independent.
- No game data or generated native models in source/distribution. No publish/device install.
- All per-game addresses and texture classifications must be measured and guarded.

## Review Focus
- P2 joins/leaves while either trigger is held: no accidental firing or extra reload.
- Unrecognized ROM revision fails safely rather than patching unrelated code.
- Partial/missing game sets cannot display READY.
- Gun preparation skip/corrupt cache recovers without loading a borrowed gun.
- The VR menu pauses simulation while headset motion remains live; focus loss preserves inherited input release behavior.

### Task 1: isolated research and native profile
**Files:** core/rend/vr_reproject.cpp; core/rend/vr/hands_rip.*; hotd2-vr/death-test.ps1; hotd2-vr/notes/death-findings.md.
**Interfaces:** existing GameProfile, GunState and hands::Source feed the inherited OpenXR host. No game constants copied from Lupin.
- [x] Baseline safe harness and boot: private UDP27116, distinct death-crimson-player, owned-PID cleanup, raw RAM/render captures.
- [x] Identify supplied ROM revision, camera/projection and HUD; write fixture tests before profile changes; verify against live game.
- [x] Find native shot/ammo/session/P2/reload/cutscene signals; add guarded state reader and tests, then controlled real-shot and empty-pull checks.
- [x] Identify complete native gun geometry; test preparation/cache path or explicitly record an outstanding native-model limitation.
- [x] Verify camera/aiming, HUD and gameplay in simulator; record measured evidence.

### Task 2: dual-hand option
**Files:** core/rend/vr/xr_host.*, xr_menu.cpp, xr_labels.cpp, core/cfg/option.*, hotd2-vr/re/death_hand_test.cpp and runner.
**Interfaces:** p1Hand() remains dual-player assignment; single gun follows trigger independent of setting. GunState player indices unchanged.
- [x] Write production-function tests for default P1-left assignment, both toggle states, either single-player trigger, join/leave, held-trigger and downward-latch swaps; observe RED.
- [x] Implement dedicated persisted two-player option, accurate labels and menu copy; observe GREEN.
- [x] Run native build and simulator single/dual tests once Task1 provides profile/state.

### Task 3: app identity, setup and distribution
**Files:** shell/android-studio/flycast/src/main/java/com/flycast/emulator/DeathCrimson*.java, Android resources/manifest/build.gradle.kts, core/standalone_game.h, CMakeLists.txt, hotd2-vr/build-*.cmd and package-release*.ps1.
**Interfaces:** standalone policy rejects other loaded game IDs; exact Death Crimson ID from Task1 supplied on discovery. Windows research builds permit policy OFF.
- [x] Write failing setup/policy/distribution tests for game set, BIOS, validation bounds, wrong/truncated input and own identity.
- [x] Adapt setup and release tooling to own app and file requirements, preserve independently licensed models and signing configuration without exposing secrets.
- [x] Build Windows and Quest; verify identity, no game data, content, licenses and native selection guards.

### Task 4: integration and delivery
**Files:** README.md, hotd2-vr/notes/death-validation.md and headset checklist, dist/ verification reports.
- [x] Verify final-binary menus, persistence, labels, focus, Quit, hand behavior, actual rounds/reload/P2, cache fallback and model choices.
- [x] Review source and run relevant inherited engine regression checks from private fixtures.
- [ ] Package local APK/PC ZIP and prepared local PC launcher; report remaining physical checks honestly.

Final Release ON and signed Quest rebuild/package verification follow the source commit. Physical checks remain explicitly pending in hotd2-vr/notes/death-headset-checklist.md.
