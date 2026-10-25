# Final Confidential Mission Feature Sync Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans for inline implementation and a fresh whole-change review.

**Goal:** Bring the user's finished CM control/menu updates into the completed Lupin port and refresh local Quest/PC builds.

**Architecture:** Adapt the committed generic CM changes to Lupin's existing isolated branch. Keep all measured Lupin camera, calibration, state, model/cache and standalone identity code. The clean release clone remains the build/package source.

**Tech Stack:** C++17/OpenXR, PowerShell simulator harness, Gradle/Android.

**Spec:** User-approved full feature parity and the subsequent explicit authorization to synchronize finished CM changes; hotd2-vr/notes/lupin-cm-followup.md.

## Global Constraints

- Work only in E:\___flattovrdev\lupin-vr on codex/lupin-vr and its clean release clone. CM is read-only.
- Do not stage/change cgltf dependency paths or gamesdk. Do not read signing-secret contents.
- No publication, headset installation, game data or game-derived images/models in source or distributions.
- Preserve com.vrlightgunhub.lupin and original GDS-0018 support. Physical headset checks remain untested.

## Review Focus

- P1 left maps X/Y to P1, A/B to P2; a live dual-player hand swap waits for trigger release.
- How to play and focus/menu transitions must not produce shots or restore controllers in story scenes.
- Legacy numeric/word haptics load deterministically; Off retains visual recoil and Strong is longer than Medium.
- All four controller profiles update player labels; Trigger stacks above Stick when present.
- Distribution checks attest the final committed native/app source, retaining Lupin-only setup and licensed models.

### Task 1: Adapt and verify the generic features

**Files:** core/cfg/option.{h,cpp}; core/rend/vr/xr_host.{h,cpp}, xr_labels.{h,cpp}, xr_menu.{h,cpp}; hotd2-vr/lupin-ui-accept.ps1; new hotd2-vr/lupin-cm-sync.ps1; hotd2-vr/lupin-accept.ps1.

- [ ] Update menu/persistence/pointer tests to 13 rows, Hands, word-based haptics and 0.072s Strong sample. Add P1-left labels/input, dual-player routing/swap, legacy haptics and real movie-empty-hands scenarios.
- [ ] Run the new menu and P1-left scenarios against the initial build and observe missing-feature failures.
- [ ] Apply only generic finished CM core deltas, resolving Lupin-specific source context explicitly. Preserve all Lupin memory and model changes.
- [ ] Build Windows and run the affected UI, control-profile, haptics, actual game state/cutscene/hand-swap checks and appropriate existing calibration/cache checks. Inspect representative images.

### Task 2: Review, document and refresh artifacts

**Files:** README.md; hotd2-vr/package-release.ps1 portable instructions; hotd2-vr/notes/lupin-validation.md, lupin-cm-followup.md, lupin-vr-headset-checklist.md; ignored final report/ledger.

- [ ] Record the complete CM commit comparison and feature adaptations. CM publishing/card art and its internal folder rename are project-specific and must not add false Lupin publication/install claims.
- [ ] Fresh review, resolve important findings, commit only explicit source/test/doc paths.
- [ ] Fast-forward the clean release clone, build Windows/Quest, strict package validation and native allow/reject; copy verified files while preserving playable-folder user config and game data.
- [ ] Save final hashes/source/results, mark synchronization complete, stop the now-obsolete heartbeat and deliver artifact links with physical-testing limits.

Ruling: The user explicitly said the CM changes are done and to continue now. Existing approval covers this feature synchronization and inline execution; no repeated design/permission prompt is needed. Reuse the already isolated Lupin checkout and clean release clone.