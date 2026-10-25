# Death Crimson OX native gun investigation

2026-10-25. Initial inspection used existing captures offline. A subsequent authorized neutral attract run (`dc-attract-gun-corpus`, private UDP port 27116) lasted 220 seconds, sent no game inputs, saved ten geometry captures, and exited cleanly with settings/saves restored. All geometry analysis was offline; no simulator was used for this investigation.

No complete protagonist pistol is established by the current captures. The robot in `dc-route-port` carries a scythe. The apparent multi-hole barrel texture is paired knee armour, not a gun. The character in `dc-reva-baseline` pass 2 is an unarmed woman; her silver texture candidates are cuffs/collar and chest clothing. These meshes must not be advertised or cached as the player's native pistol. Use the Arcade gun/custom models until an actual complete in-game pistol can be verified from another scene.

## Captures and method

Private inputs remain in `build-win/vrtest/<run>/rip`; no ROM, RAM, VRAM, screenshots or generated geometry belongs in source or release packages. RIP1 data was read using the repository's `hotd2-vr/assets/riplib.py`. Texture decoding used each pass's associated VRAM and palette snapshot. All unique textures were inspected in contact sheets, and selected geometry was unprojected and rendered from six directions.

The raw RIP1 headers in these captures report focal `(640,640)`. For model dimensions and previews the offline scripts override both axes with the measured Death Crimson focal `554.297`. Dimensions below are game units, not metres. Connected components use the existing `rip_hands.clusters` shared-vertex rounding to 0.001 game units. PCA extents are descriptive bounding dimensions, not semantic weapon axes.

Private analysis scripts, texture sheets, six-view renders and machine-readable measurements are under `build-win/gun-research/`: `inspect_captures.py`, `inspect_models.py`, `cluster_summary.py`, `summary.json`, and `clusters.json`. They do not touch a running game.

## Evidence

| Capture | Content | Strips | Texture observations |
| --- | --- | ---: | --- |
| dc-route-port pass 0 | Robot enemy holding a scythe, street scenery and HUD | 879 | 59 decoded textured variants; 60 texture-address groups including untextured geometry |
| dc-route-port passes 1 / 2 | Same robot/street assets with animation/HUD changes | 888 / 842 | No additional protagonist gun texture identified in the pass inventories |
| dc-route-port passes 3 / 4 | Flat UI screens | 13 / 7 | One strip per used texture |
| dc-reva-baseline pass 0 | Empty pass | 0 | No geometry |
| dc-reva-baseline pass 1 | Intro street scenery | 597 | 35 decoded textured variants; no character gun |
| dc-reva-baseline pass 2 | Unarmed magenta-clothed woman, robot and interior scenery | 1670 | 50 decoded textured variants; the woman's hands are both visibly empty in the isolated mesh |

The route robot uses 189 strips / 928 triangles across `0xACB000..0xAE1000`, with a combined eye-space box of `175.204 x 247.076 x 179.544`. It is not a complete robot firearm. In particular:

| Texture | Strips / triangles | Verified geometry and measurements |
| --- | --- | --- |
| `0xACB000` | 8 / 40 | Two separate bronze knee/leg plates, each 4 strips / 20 nondegenerate triangles. PCA boxes `17.951 x 20.128 x 10.965` and `18.416 x 20.612 x 10.890`. The holes are painted armour details. |
| `0xADA800` | 33 / 180 | Scythe plus both boots. Four components: scythe blade/upper shaft 10 strips / 56 triangles, two boots each 9 / 56, lower shaft 5 / 12. PCA boxes respectively `154.439 x 55.827 x 8.646`, `43.631 x 28.727 x 15.797`, `43.586 x 28.219 x 15.926`, `130.553 x 6.254 x 6.254`. A texture-only extraction would incorrectly include the boots. |
| `0xAD3000` | 3 / 12 | Broad box-like robot body/back part, no pistol grip or barrel. |
| `0xAE0000` | 10 / 48 | Black robot arm/shoulder parts, no pistol. |

The magenta character's selected body textures (`0x9FA400..0xAB9000`, excluding HUD strings `0xA90800` and `0xA99800`) contain 1188 strips / 6099 triangles and a combined eye-space box of `71.603 x 65.704 x 131.385`. Six-view rendering shows the whole unarmed character. Silver candidates:

| Texture | Strips / triangles | Verified geometry |
| --- | --- | --- |
| `0xA9BC00` | 13 / 66 | Two cuffs and two thin collar/clothing pieces, four disconnected components. |
| `0xA9C800` | 26 / 145 | One chest-clothing component, eye-space box `23.331 x 22.042 x 18.469`; no gun parts. |
| `0xAB7400` | 31 / 154 | Dark blue skirt/shorts fabric pieces. |

## Limits and next evidence

This rules out the candidates in these captures; it does not prove the game never renders a pistol. The current scene samples do not include a verified protagonist holding one. Direct ROM extraction is not established: the RIP1 pipeline provides rendered vertices and textures, and no validated Death Crimson native model-container parser has been identified here. A new native-gun source needs another character/cutscene capture with an actual pistol visible, a complete dedicated mesh (all sides, barrel, grip), clean component selection when textures are shared, measured orientation and origin, and tests that reject partial/corrupt caches. No borrowed Confidential Mission/Lupin model or native-source constant was added.
## Wider neutral attract corpus

The follow-up `death-test.ps1 -Game deathcoxj -Name dc-attract-gun-corpus -Port 27116 -Seconds 220 -Config config:vr.GameHands=no` scheduled `rip 1 1 0` at each time below. No Start/trigger/poke or other game input was scheduled. The harness reported 220 seconds, 10 actions, exit 0; its owned process ended and the port was released before other runtime testing resumed. This used the already-built binary and live RIP1 geometry, with no engine/source edits or speed override.

| Wall seconds | Pass | Strips | Observed content |
| ---: | ---: | ---: | --- |
| 75 | 0 | 349 | Street establishing view and story text |
| 90 | 1 | 590 | Street story view |
| 105 | 2 | 3573 | Magenta-clothed woman plus interior/monster geometry under a transition |
| 120 | 3 | 946 | Lab/interior scene |
| 135 | 4 | 3988 | Three human meshes and a large mechanical enemy in the street |
| 150 | 5 | 615 | Street/scenery and effects |
| 165 | 6 | 3 | Baked reload tutorial screen, not reconstructible character/weapon geometry |
| 180 | 7 | 13 | Ranking screen |
| 195 | 8 | 1 | One full-screen YUV textured quad |
| 210 | 9 | 7 | Title screen |

Private outputs are `build-win/gun-research/dc-attract-gun-corpus-<pass>-{textures,scene}.png`, `attract_summary.json`, and the `inspect_attract*.py` scripts. Three- and six-view model files were not exported as runtime caches. The six-view inspection images are `attract4-human-all.png`, `attract4-male.png`, `attract4-other-human.png`, `attract4-woman.png`, `attract4-mech.png`, and `attract4-mech-<texture>.png`.

At 135 seconds the raw pass includes the green-jacket man, the magenta-clothed woman and an older woman in a pale/turquoise top. Six-view inspection of all human-textured components shows their hands empty. Selected ranges/counts (these are texture selections, not certified standalone models):

| Selection | Strips / triangles | Measurement / result |
| --- | --- | --- |
| Human textures `0x9FA400..0xAFF000`, excluding the known HUD strings | 2903 / 15188 | Three human figures; no dedicated pistol geometry found |
| Green-jacket man `0xABA800..0xAC8FFF` | 1254 / 6627 | Eye-space box `183.523 x 428.684 x 122.481`; both hands empty |
| Older woman `0xAC9000..0xAFF000` | 424 / 2298 | Eye-space box `119.134 x 369.781 x 101.826`; no gun |
| Mechanical enemy `0xB00000..0xB54800` | 667 / 3356 | Selected eye-space box `298.911 x 475.689 x 132.273`; large body/forearm assembly, not a protagonist pistol |
| `0xB54000` | 32 / 166 | Large rectangular mechanical forearm assembly; eye-space box `163.336 x 223.944 x 54.077`. Completeness as a separate weapon is not established. |
| `0xB08000` | 36 / 72 | Paired cables/tubes connecting that assembly to the mechanical enemy; box `72.994 x 126.081 x 51.316` |
| `0xB2B000` | 7 / 42 | Long silver mechanical head/upper-body part, not a gun; box `48.241 x 118.585 x 22.178` |

The existing texture decoder does not handle five format-4 bump-map variants in pass 4 (`0xB12000`, `0xB1E800`, `0xB2C800`, `0xB3A000`, `0xB46800`), or the format-3 full-screen YUV quad in pass 8 (`0x99D800`). Those unsupported texture layers are omitted from contact sheets; their geometry remains in numeric inventories and in the mechanical-enemy preview without those texture layers. This does not conceal a pistol mesh: all human texture ranges and the complete selected mechanical geometry were inspected separately.

The larger sample reinforces the limited conclusion: **no verified complete native protagonist pistol is available from these captures**. There is a large enemy mechanical forearm assembly, but no established standalone robot pistol that should substitute for the player's gun. Retain the Arcade/custom model fallback. Future native preparation needs a distinct scene that actually exposes the protagonist's pistol and verifies its complete grip/barrel/all-sided geometry; do not invent a Death Crimson native source from the observed clothing, armour, scythe, forearm assembly, or baked tutorial image.