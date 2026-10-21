# AI/Python/Cat

The Tutorial Cat, from mesh to Blueprint. Design: `AI/Tutorial/CatAnimation.md`. Anim rules: `AI/MCP/MCP_Animation.md`, toolkit `Anim/anim_sequence_authoring.py` (loaded by project-relative path).

Order: `Mesh/generate_cat_mesh.py` → `Mesh/rig_cat.py` → `make_cat_materials.py` → clips → `Anim/cat_anim_blueprint.py` → `Anim/cat_blueprint.py`.

## Mesh/
| Script | Does |
|---|---|
| `generate_cat_mesh.py` | `SM_Cat` (`/Game/Art/Meshes/Cat`): the Tutorial Cat, face toward +X, built for the top-down camera, nothing touches: HUD portrait face extruded (head, ears, tilted eyes, nose, mouth), three-sided pyramid body floating behind the head (a triangle from above, apex back), four disc paws (two hands at the head's sides, two feet beside the body), tail of 9 small squares curling sideways from the apex; one part each for a rig; slots = colour roles `Base` / `Shade` / `Ink` / `Light`; part bounds in `AI/Output/cat_gen.json` |
| `rig_cat.py` | `SM_Cat` → `SKM_Cat` on `SK_Cat` (same folder): one rigid bone per part, a vertex owned by the smallest part box holding it, tail as a chain, `PortalPoint` socket on `Root` at `PORTAL_X` (bones: `AI/Tutorial/CatAnimation.md`); helpers loaded from `Badge/Mesh/rig_class_badges.py` |

## Material
| Script | Does |
|---|---|
| `make_cat_materials.py` | `M_Cat` (flat unlit, masked below the actor origin height) + `MI_Cat_<role>`, put on `SKM_Cat` |

## Anim/
| Script | Does |
|---|---|
| `cat_clip.py` | Shared by the Cat clips: clip writer from a `pose(frame)` of deltas on rest, eased-curve helpers (attack / release curves, summed whole-cycle drivers, lagged reads, planted / swing foot), top-down check (new part overlaps, highest vertex, loop closure) reported to `AI/Output/cat_<clip>.json` |
| `cat_idle.py` | Sit, head up to the camera, blink / ear / tail flick loop |
| `cat_walk.py` | Diagonal-pair walk loop |
| `cat_portal_jump.py` | Open portal, hop, sink through the plane: clip, montage `AM_Cat_PortalJump`, portal notify state |
| `cat_reassemble.py` | Splashed apart on the floor, gather, stretch, release onto idle: clip + montage `AM_Cat_Reassemble` |
| `cat_anim_blueprint.py` | `ABP_Cat`: Idle / Walk blended on `Speed`, montage slot (`BuildBlendedAnimGraph`) |
| `cat_blueprint.py` | `BP_Cat` (child of `AGeoCat`, `/Game/Actors/Cat`): mesh `SKM_Cat`, anim class `ABP_Cat` (montages are given by the encounter) |
