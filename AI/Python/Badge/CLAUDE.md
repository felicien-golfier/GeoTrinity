# AI/Python/Badge

Class badges (Square, Circle, Triangle): meshes, rig, material, every clip, the turret. Rules: `AI/MCP/MCP_Animation.md`; toolkit `Anim/anim_sequence_authoring.py` (loaded by project-relative path). Run `Anim/verify_badge_scripts.py` before re-running any clip script.

Order: `Mesh/generate_class_badge_meshes.py` → `Mesh/rig_class_badges.py` → clips → `Anim/class_badge_anim_blueprints.py` → `Anim/class_badge_wiring.py`.

## Mesh/
| Script | Does |
|---|---|
| `generate_class_badge_meshes.py` | Three class logo silhouettes extruded per class; Triangle body thins to its point |
| `rig_class_badges.py` | Rig badges: body bottom layer, parts on own bones under a top layer, fire sockets, Square keyhole socket |
| `generate_triangle_turret_mesh.py` | Triangle class turret: flat triangular body (tip +X) with a layered round hub, long triangular-section cannon out of the tip (dark bands, muzzle block with two stripes), rear block, two grey side cannons at the back corners; slots `Color` / `Cannon` / `CannonBand` / `Legs` |

## Material
| Script | Does |
|---|---|
| `generate_class_badge_masks.py` | Draws each class's line mask from the badge's part outlines and its mark tables; composites the hand layer on top |
| `make_class_badge_materials.py` | Badge body material: imports the masks, instance per class, wired to class data and meshes |

## Anim/
| Script | Does |
|---|---|
| `rerig_class_badges.py` | Re-rig badges from `Mesh/rig_class_badges.py`, rewriting every clip to play as before |
| `class_badge_anim_blueprints.py` | One AnimBlueprint per badge: top / bottom slot clips over idle, full-body and additive slots |
| `class_badge_wiring.py` | Put the playable characters on the badges: class data, mesh, montages, materials |
| `class_badge_idle.py` | Breathing, bored part play |
| `class_badge_death.py` | Swell, vanish, shed class debris; sound on first frame |
| `class_badge_rez.py` | 3 s rez: grow back, parts orbit while the body rolls |
| `class_badge_deploy.py` | Throw deployable, full-body slot |
| `class_badge_body_turns.py` | Body-only rolls (Circle through Moira beam, Triangle reeling turrets) |
| `class_badge_circle_charge.py` / `_orbit.py` | Circle charge beam, two variants |
| `class_badge_square_fire.py` / `_piston.py` | Square auto-fire, two variants |
| `class_badge_square_sacrifice.py` | Martyr Beam and Martyr's Wrath, bottom slot |
| `class_badge_triangle_fire.py` | Triangle heavy shot: FireRatchet (auto-fire), FireFormation (turret launch) |
| `class_badge_triangle_reload.py` | Triangle reload, full-body slot |

## Line masks
Lines are not geometry: one mask per class (`T_<Class>Badge_Mask`), projected from the pre-skinned position onto both caps, so every part carries its lines on its own bone, top and bottom.
- Source of truth = `generate_class_badge_masks.py`, never the texture. Change a line: edit its tunable or mark table, rerun the script, then `make_class_badge_materials.py`.
- Each movable part gets two marks: a stroke just inside its outline, and a filled copy of its shape scaled down about its centre (the small Circle disc: ring + dot). A body adds its logo lines as polylines in the mark table.
- One `LINE_WIDTH` (17 px) for every contour, the Triangle's. `THIN_WIDTH` only where two parallel lines run side by side (Square outline pair, Circle middle line). Both in mask pixels, tunables at the top of the script.
- Layers per class in `GamePictures/BadgeMasks/`: `<Class>_Generated.png` (script output, overwritten), `<Class>_Hand.png` (optional, drawn by hand in the same frame and size, never overwritten, composited over the generated one) and `<Class>_Mask.png` (the composite the material script imports). Side drawings go in the hand layer.
- Mask frame and projection: the mesh generator's pixel frame (`frame()`), unchanged; the material is untouched.
- Body marks are drawn first, then each movable part's footprint is cleared (`PART_CLEAR`), so no body line lands on a part. A part too small for `LINE_WIDTH` plus its inner shape takes a narrower `RING_WIDTH` entry (needle, mandibles).
- Runs on host Python (Pillow), no editor; `unreal` is stubbed to load `badge_parts()` / `frame()`.
