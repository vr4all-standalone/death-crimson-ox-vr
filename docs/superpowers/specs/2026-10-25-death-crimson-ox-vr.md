# Death Crimson OX VR feature parity

User request: make Death Crimson OX like the completed Confidential Mission and Lupin ports. Source reference: Lupin f67026046, including CM final-sync 00706171d. Separate project at E:\___flattovrdev\death-crimson-ox-vr, C++17 Flycast/OpenXR, Windows PC VR and standalone Quest.

Carry over setup styling, menus, gun/custom model choices, comfort, world scale, panel distance, haptics, reload modes, pointer and controller labels, pause/focus/quit behavior, first-run gun preparation where a complete native model can be identified, and physical-headset checklist. All Death Crimson camera, input, state, HUD, cutscene and native-model constants require evidence from this game's local ROM. Preserve other projects and processes.

Latest user correction: the hand option controls the two-player gun assignment only. Default enabled is P1 left / P2 right; disabling reverses it. Single-player always allows either trigger to move the gun into that hand. The option must not lock single-player to a hand. Live swaps must preserve player state and physical-controller reload latches, and suppress held triggers until released.

Use independent saves/configs/ports/model caches. Private ROM copies are in build-win/game-data. Never package ROMs, BIOS, screenshots, RAM, generated native models or signing secrets. Preserve licenses. No publishing or device installation is requested. App identity com.vrlightgunhub.deathcrimsonox; label Death Crimson OX VR; outputs death-crimson-ox-vr-quest.apk and death-crimson-ox-vr-pcvr.zip. Support the supplied arcade set first, only claim revisions actually verified.
