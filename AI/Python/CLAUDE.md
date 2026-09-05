# AI/Python — Editor Automation Scripts

Every script here runs through `mcp-unreal` `execute_script` against the open editor. The rules that govern
them — silent failures, re-runnability, saving dirty packages, comment density — are in `AI/MCP/CLAUDE.md`;
read that before writing or running one.

Scripts are grouped by what they touch, one folder per topic. A script that another one imports is loaded by
its project-relative path, so moving a file means fixing those literals. Tunable values sit in a block at the
top of each file.

| Folder | Holds | Topic doc |
|---|---|---|
| `Ability/` | Ability Blueprints, the ability catalog, StateTree | `AI/MCP/MCP_NewEnemyAbility.md`, `MCP_StateTree.md` |
| `Anim/` | Skeletal animation and montage authoring | `AI/MCP/MCP_Animation.md` |
| `Asset/` | Generic asset editing, asset generation, saving | `AI/MCP/MCP_Blueprint.md` |
| `Level/` | Level content and per-level settings | `AI/MCP/MCP_Settings.md` |
| `Material/` | Materials, material functions, parameter collections | `AI/MCP/MCP_Material.md` |
| `Mesh/` | Procedural static meshes and rigging | — |
| `Niagara/` | Niagara systems and emitter stack editing | `AI/MCP/MCP_Niagara.md`, `AI/VFX.md` |
| `Runtime/` | Reading and driving a running session or the editor world | `AI/MCP/MCP_PIE.md`, `MCP_Preview.md` |
| `UI/` | Widget Blueprints and widget trees | `AI/MCP/MCP_UI.md` |

## Ability

| Script | Purpose |
|---|---|
| `ability_info_icons.py` | Normalise the catalog's player entries — deploy-count flag, icon fallback |
| `ability_tags.py` | Read or re-set the asset tag container on an ability Blueprint |
| `new_enemy_ability.py` | Create an enemy ability Blueprint and register it in the catalog |
| `state_tree_edit.py` | StateTree editing through the builder shim — states, tasks, transitions |
| `triangle_momentum_passive.py` | The Triangle stacking damage-boost passive, end to end |

## Anim

`anim_sequence_authoring.py` is the toolkit every other file here imports; the rest are one boss clip each.

| Script | Purpose |
|---|---|
| `anim_sequence_authoring.py` | Read a rig, report animated bones, write bone tracks, build montages |
| `death_montage_scale_out.py` | Circle and Triangle death montages, cut to the Square's beat |
| `hex_boss_abilities.py` | Hex boss sweep beam, tile-carving ray and cone spray |
| `hex_boss_death.py` | The three rings wander off axis, blow apart and settle |
| `hex_boss_idle.py` | Rings turning against each other, the outer one breathing |
| `hex_boss_intro.py` | Three dead pieces find each other, lock together and wake |
| `hex_boss_launch.py` | Tile bomb and tile turret launches |
| `star_death.py` | The star spins itself apart, swallows its points, collapses |
| `star_devastating_wave.py` | Winds into a thin spinning knot, then blows every spike out |
| `star_idle_breath.py` | Breathing on both axes with a swell travelling around the tips |
| `star_intro.py` | A seed with no points grows them one at a time, then goes nova |
| `star_intro_camera_shake.py` | Camera shake curve climbing with the intro montage |
| `star_pike_nova.py` | Tip bones fire around the star one at a time, then all together |
| `star_spike_nova.py` | Spikes clench away, throw out at once, settle back |

## Asset

| Script | Purpose |
|---|---|
| `curve_asset_authoring.py` | Write a curve asset from a table of keys |
| `generate_headshot_ding.py` | Synthesise the headshot ding and import it as a sound wave |
| `save_dirty_assets.py` | List and save dirty content packages, reporting to a file |
| `struct_container_edit.py` | Rewrite a struct array or struct map on an asset or a Blueprint |
| `wheel_zoom_input.py` | Create the zoom input action and bind it to the mouse wheel |

## Level

| Script | Purpose |
|---|---|
| `level_settings.py` | Edit a level's world settings, GameMode override and controller class |
| `tutorial_room.py` | Build the tutorial room in the draft map |

## Material

| Script | Purpose |
|---|---|
| `empowerment_ring_material.py` | Empowerment sprite materials — dashed ground ring, spark, shard |
| `geo_shape_material.py` | The one particle material every geometric effect draws with |
| `make_background_lattice_material.py` | Floor triangle line-art and the pulse collection driving it |
| `make_camera_mpc.py` | Camera parameter collection the parallax layers read |
| `make_deployable_outline_material.py` | Cel-shading outline post-process material |
| `make_duration_wipe_function.py` | Clock-wipe mask shared by every remaining-life readout |
| `make_parallax_stars_material.py` | Backdrop star layers, far to near |
| `make_pulse_beam_material.py` | Rectangular beam — outline frame, pulsing inside |
| `make_pulse_circle_material.py` | Unlit additive circle — constant outline ring, pulsing inside |
| `make_zone_indicator_ray.py` | Ray and bar variant of the zone indicator |
| `set_camera_backdrop.py` | Fill the camera's backdrop component with the plane and star layers |

## Mesh

| Script | Purpose |
|---|---|
| `generate_bomb_mesh.py` | Round bomb mesh sized to replace the pillar |
| `generate_hex_boss_mesh.py` | Hex boss body as three concentric hexagons, and the rig it emits |
| `rig_hex_boss.py` | Turn that body into a skeletal mesh on a new skeleton |

## Niagara

| Script | Purpose |
|---|---|
| `buff_vfx.py` | Every buff a character or its shots can wear, in one style |
| `charged_trail_vfx.py` | Electric trail left behind a damage-boosted shot |
| `cone_coil_vfx.py` | Electric helix wound around a cone, turning about its axis |
| `decode_niagara_parameters.py` | Decode a rapid-iteration store fetched from the property API |
| `empowered_arc_vfx.py` | Electric arc running over a character's own mesh |
| `empowered_aura_vfx.py` | Three empowerment auras — light flames wrapping a character |
| `empowerment_vfx.py` | The empowerment aura, authored for the orthographic view |
| `geo_aura_vfx.py` | The five character auras, rebuilt as geometry |
| `lightning_variants.py` | Three electric declinations, each on a mechanism the others lack |
| `niagara_stack_edit.py` | Stack editing through the builder shim — modules, switches, inputs |
| `set_outline_vfx_flags.py` | The two flags deciding how a translucent effect meets the outline |
| `static_electricity_vfx.py` | Blue bolts striking around a character |

## Runtime

| Script | Purpose |
|---|---|
| `couch_coop_debug.py` | Dump couch-coop input ownership from a running session |
| `pie_drive_menu_ui.py` | Drive live menu widgets without simulating input |
| `pie_inject_input.py` | Inject input actions into a session and measure the gameplay result |
| `vfx_editor_preview.py` | Place systems in the editor world for the level viewport to judge |

## UI

| Script | Purpose |
|---|---|
| `ability_bar.py` | Ability bar pipeline — cooldown sweep material, slot widget, bar widget |
| `charge_beam_gauge.py` | Create a widget Blueprint, build its tree, wire it to a component |
| `crosshair_cursor.py` | Crosshair software cursor, bound to the cursor slot |
| `group_widgets.py` | Wrap existing canvas children into one panel without moving them |
| `local_connect_menu.py` | Build a child panel inside an existing menu widget |
| `pause_menu_setup.py` | Centered vertical menu of labeled button rows |
| `second_player_gamepad_toggle.py` | The couch-coop gamepad row on the key-bindings widget |
