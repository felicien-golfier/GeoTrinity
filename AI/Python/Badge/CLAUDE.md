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
| `make_class_badge_materials.py` | Badge body material: logo masks, instance per class, wired to class data and meshes |

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
