# Death Crimson OX first-run gun and publication plan

The user authorized publishing Death Crimson OX on GitHub with Quest/PC builds like the previous ports, adding it to the existing VR4ALL catalog, and preparing its own native gun at first launch like Confidential Mission. Existing hand defaults and solo either-trigger transfer remain required.

## Native first-run gun

- Verify a complete native firearm from the owned Japan Rev A cartridge. Verified direct source: the ISO9660 cartridge filesystem contains E410A_ATOP.NJ and MSGUN01.PVR, a complete 52-triangle soldier pistol. It is not established as the mythical Crimson. The earlier attract-capture search was insufficient.
- Use an independently testable bounded parser for the verified object and matching texture. No generated game geometry or texture is tracked or distributed.
- Integrate device-local generation and per-game cache with the inherited gun selector/preparation UI. A valid cache skips preparation; missing/corrupt/incompatible caches rebuild. Skip/failure uses the Arcade gun and can retry later. Preserve the game's state and avoid an artificial wait if deterministic asset extraction is fast.
- Observe meaningful failing tests for model/container bounds, wrong inputs and cache/skip/retry behavior; verify complete mesh visually, scale/orientation, both hands and resumed play in simulator.

## GitHub project and release

- Publish `vr4all-standalone/death-crimson-ox-vr`, default `main`, matching prior port pages and attribution. Include the matching generated promotional banner and honest physical-test limits.
- Publish only signed Quest APK, PC VR ZIP and SHA256SUMS.txt. Build from the exact clean tagged source. Keep original game data, generated native models and private signing material out of source and release assets.
- Follow the previous ports' verified local release-build workflow; avoid running inherited unrelated upstream CI/signing/upload jobs on the new fork. Public release names, hashes and APK identity must be independently verified before catalog download checks.
- Create a draft, verify upload digests and tag provenance, then publish under the user's authorization and verify anonymous downloads.

## VR4ALL

- Add its own Arcade / Rail shooter card, exact Quest APK release pattern, separate package `com.vrlightgunhub.deathcrimsonox` and project banner.
- Transfer owned `deathcoxj.zip` and optional separate `naomi.zip` to canonical Download paths; no CHD. Explain Japan Rev A and unsupported original Japan/USA revisions.
- Test exact release selection, canonical install plan, rescan/package isolation and library counts; preserve the other chats' pending VR4ALL edits.
- Verify VR4ALL discovers and downloads the published APK and matches its SHA-256. Keep the single main VR4ALL publishing workflow; do not publish a separate branded installer.

## Delivery evidence

Re-run applicable native/Java/package checks and Windows/Quest builds after source changes; strict packaging validates identities, commit stamps, signatures, licenses and exclusions. Record final source, release URLs, hashes, first-run/cache/fallback runtime evidence and remaining physical headset/later-stage checks. All runtime tests use the owned Death Crimson process, private port 27116 and restored configuration/saves.
