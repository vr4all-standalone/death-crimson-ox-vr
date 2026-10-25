# Lupin the Third VR validation

Original GDS-0018 only, set lupinshoo. Independent source E:\___flattovrdev\lupin-vr, branch codex/lupin-vr. Source provenance and numeric research are in lupin-findings.md. Physical headset checks remain unchecked in lupin-vr-headset-checklist.md. No game data is included.

## Feature-to-evidence matrix

| Design sections | Implemented behavior | Local evidence |
|---|---|---|
|1,2|Stereo reconstruction, tracked view, guarded live camera and independent original hit lens; widening|XR simulator gameplay/pause; causal front/rear head hits; original lens2756/7281; production lp-aim-wide and lp-aim-stock render headers|
|3|Both-player calibration, corners/edges/center and native reload sentinel; narrow original-marker removal|23 input/calibration checks;32 actual game VM points across widening modes (sampled max1px/axis);10 causal boundary cases;40 marker UV positives/119 HUD negatives|
|4|Inverse-depth HUD, flat screens, preserved near floor/walls, P2 prompt separation|lp-hud150s and instruction/text inspection;318pass P2 audit; actual-pistol downward/head-translation captures; extreme poses expose black outside the source game's drawn geometry|
|5,14|Twelve-row paused menu, values, pointers, Quit confirmation, persisted settings|All10 lp-ui scenarios pass; real second-launch persistence; frozen image diffs0 and live head redraw|
|6,12|Touch/Index/Vive/WMR bindings and labels, hand switching, story controllers without labels|lp-ui controls/profiles/labels; lp-feedback story on/off and controller/gun handoff|
|7,8|Original-game pistol, licensed Arcade/Zapper, custom-model infrastructure, atomic isolated cache|Complete24-strip/86-triangle25840byte pistol;16 model/cache unit tests;4 cache runtime cases;7 renderer-consumption regression checks plus3 rejected/valid cache runtime cases; actual licensed Arcade/Zapper/pistol frames|
|9,10|Accepted-round recoil/flash/haptics, both magazines and reload modes, empty feedback|lp-feedback; lp-empty native watches: P1eight/P2seven kicks, exactly two empty clicks each, independent sensor reloads and no empty kicks; lp-reload-point: P1three/P2two automatic reloads at expected exits, full-magazine/menu suppression, held Point-mode exit and manual Y|
|11|Real P2 join/leave, separate magazines/feedback, single-gun return|lp-feedback and lp-empty native active flags, P2Start and one-gun return; measured prompt filter|
|13,14|Separate Android setup/app, exact original files, standalone native selection, portable launcher/licenses|67 Java11 JVM tests;27 native-policy checks;40 distribution tests; preliminary Quest build; final artifact checks pending|

## Engine regressions

Private copied ROMs/config/saves only. HOTD2 pass100s (camera, projection/widening), MOK pass112s (four actual shots max1px), CM menu pass72s, CM pause pass125s/23cycles/10captures. Frozen CM diffs0; yaw21.325; resumes2.783/3.988. The owned sentinel survived cleanup. No process-wide kill remains in the invoked harnesses.

## Current build and release gates

Windows development build links successfully with OpenXR. Preliminary optimized arm64 Quest APK builds successfully; identity com.vrlightgunhub.lupin, Lupin the Third VR, versionCode1/base1.0.0, sole Lupin setup launcher. Final build verification uses a separate clean committed checkout at E:\___flattovrdev\lupin-vr-release. It avoids changing dependency/index metadata in the development checkout. Actual native policy allow/reject, package identity/content/signer/license results and output hashes are recorded in dist/build-verification.json after successful packaging; absence of that report means those final gates are pending. Expected outputs dist/lupin-the-third-vr-quest.apk and dist/lupin-the-third-vr-pcvr.zip.

## Limits and next requested synchronization

Simulator checks do not prove physical tracking, feel, comfort, sustained frame rate, Android relaunch behavior or hardware/controller compatibility. No physical headset was installed or tested. Raw CUE/GDI boot, later stages and P2-continue visual transitions remain untested. Extreme head movement can expose black beyond geometry drawn by the original game camera. Custom model loading/angle/scaling behavior uses shared CM engine paths; distinguish inherited implementation from physical verification.

The user subsequently requested the final minor CM updates after both sessions finish. That work is queued separately in lupin-cm-followup.md; the initial12-row UI evidence is retained accurately until the synchronized feature set is validated.
