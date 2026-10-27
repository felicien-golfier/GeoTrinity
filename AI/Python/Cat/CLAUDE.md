# AI/Python/Cat

The Tutorial Cat, from mesh to Blueprint. Design: `AI/Tutorial/CatAnimation.md`. Anim rules: `AI/MCP/MCP_Animation.md`, toolkit `Anim/anim_sequence_authoring.py` (loaded by project-relative path).

Order: `Mesh/generate_cat_mesh.py` → `Mesh/rig_cat.py` → `make_cat_materials.py` → clips → `Anim/cat_anim_blueprint.py` → `Anim/cat_blueprint.py`.

## Mesh/
| Script | Does |
|---|---|
| `generate_cat_mesh.py` | `SM_Cat` (`/Game/Art/Meshes/Cat`): the Tutorial Cat, face toward +X, built for the top-down camera, nothing touches: HUD portrait face extruded (head, ears, tilted eyes, nose, mouth), three-sided pyramid body floating behind the head (a triangle from above, apex back), four disc paws (two hands at the head's sides, two feet beside the body), tail of 9 small squares curling sideways from the apex, two small `FangLeft` / `FangRight` triangles (`Light`) under the grin's corners; one part each for a rig; slots = colour roles `Base` / `Shade` / `Ink` / `Light`; part bounds in `AI/Output/cat_gen.json` |
| `rig_cat.py` | `SM_Cat` → `SKM_Cat` on `SK_Cat` (same folder): one rigid bone per part, a vertex owned by the smallest part box holding it, tail as a chain, `PortalPoint` socket on `Root` at `PORTAL_X` (bones: `AI/Tutorial/CatAnimation.md`); helpers loaded from `Badge/Mesh/rig_class_badges.py` |

## Material
| Script | Does |
|---|---|
| `make_cat_materials.py` | `M_Cat` (flat unlit, masked below the actor origin height) + `MI_Cat_<role>`, put on `SKM_Cat` |

## Anim/
| Script | Does |
|---|---|
| `cat_clip.py` | Shared by the Cat clips (ear gap `EAR_DETACH` and the default hidden fang track too): clip writer from a `pose(frame)` of deltas on rest, eased-curve helpers (attack / release curves, summed whole-cycle drivers, lagged reads, planted / swing foot), top-down check (new part overlaps minus the clip's allowed pairs, highest vertex, loop closure, least head pitch `head_pitch_min`) reported to `AI/Output/cat_<clip>.json` |
| `cat_idle.py` | Sit, head up to the camera, blink / ear / tail flick loop, the tail crossing to the other side now and then, one evil-face beat |
| `cat_face.py` | The evil face (`AI/Tutorial/CatAnimation.md`, "Evil face"): `evil_weight(frame, start, hold)` curve and `apply_evil(deltas, frame, start, hold)` (eyes, mouths, fangs, ears added on a clip's finished pose); used by idle, walks and run |
| `cat_posing.py` | World-space spec (body, head, paws, tail base, eyes) → deltas on rest, children counter-scaled; eases from and to the idle's first pose. Used by stretches, idle variants, portal jump, reassemble. Holds `FACE_PEEK_DEGREES` (20, the least head pitch of any clip), `twist_spec(shake)`, the Twist pose, shared by the clip and `Cat_Reassemble`'s stretch beat; `paws`, `shake_between` |
| `cat_stretches.py` | `Cat_Stretch_Reach` / `_Sprawl` / `_Flat` / `_Twist` (Twist = the Tutorial's; the others kept for later content) |
| `cat_idles.py` | `Cat_Idle_Toilet` / `_Groom` / `_Loaf` / `_LookAround` / `_Knead` + a montage each (`AM_Cat_Idle_<Name>`, `DefaultSlot`, short blends, the owner plays them at random) |
| `cat_walks.py` | `Cat_Walk_Prowl` / `_Fast`, the only walks: same frame count and pattern, amplitude differs; owns `PROWL_SPEED` 150 / `FAST_SPEED` 300 (cm/s) |
| `cat_run.py` | `Cat_Run`, the gallop (own cycle, `Body` flex, tail streaming); owns `RUN_SPEED` 600 (cm/s) |
| `cat_portal_jump.py` | Open portal, hop, sink through the plane: clip, montage `AM_Cat_PortalJump`, portal notify state |
| `cat_reassemble.py` | Splashed apart on the floor, gather, stretch, release onto idle: clip + montage `AM_Cat_Reassemble` |
| `cat_anim_blueprint.py` | `BS_Cat_Walk` (1D blend space on `Speed`: Prowl, Fast) and `ABP_Cat`: Idle ↔ `BS_Cat_Walk` blended on `Speed` (cm/s), montage slot (`BuildBlendedAnimGraph`) |
| `cat_blueprint.py` | `BP_Cat` (child of `AGeoCat`, `/Game/Actors/Cat`): mesh `SKM_Cat`, anim class `ABP_Cat` (montages are given by the encounter) |
