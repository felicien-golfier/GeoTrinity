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
| `Audio/` | Fetching references, taking sounds apart, synthesising and fitting them — run with `uv`, outside the editor | `AI/MCP/Audacity/ListeningToSound.md` |
| `Level/` | Level content and per-level settings | `AI/MCP/MCP_Settings.md` |
| `Material/` | Materials, material functions, parameter collections | `AI/MCP/MCP_Material.md` |
| `Mesh/` | Procedural static meshes and rigging | — |
| `Niagara/` | Niagara systems and emitter stack editing | `AI/MCP/MCP_Niagara.md`, `AI/VFX.md` |
| `Runtime/` | Reading and driving a running session or the editor world | `AI/MCP/MCP_PIE.md`, `MCP_Preview.md` |
| `UI/` | Widget Blueprints and widget trees | `AI/MCP/MCP_UI.md` |
| `Video/` | Scanning footage and rendering edit lists — run with `uv`, outside the editor | `AI/MCP/Video/CLAUDE.md` |

## Ability

| Script | Purpose |
|---|---|
| `ability_info_icons.py` | Normalise the catalog's player entries — deploy-count flag, icon fallback |
| `ability_tags.py` | Read or re-set the asset tag container on an ability Blueprint |
| `boss_behaviour_trees.py` | Split the boss StateTrees into the shared base and one behaviour tree per boss |
| `deploy_target_cue.py` | The deploy landing marker cue — ring plane on an `AGeoDeployTargetCue` Blueprint |
| `new_enemy_ability.py` | Create an enemy ability Blueprint and register it in the catalog |
| `state_tree_edit.py` | StateTree editing through the builder shim — states, tasks, transitions |
| `triangle_momentum_passive.py` | The Triangle stacking damage-boost passive, end to end |

## Anim

`anim_sequence_authoring.py` is the toolkit every other file here imports; the rest each build one set of clips
or wire them in.

| Script | Purpose |
|---|---|
| `anim_sequence_authoring.py` | Read a rig, report animated bones, write or offset bone tracks, build montages, turn a bone about a point, measure overlaps between outlines and in 3D |
| `bake_clips.py` | Freeze clips no script can rebuild — Guillaume's Circle clips, those built before the badge depth was halved — into `SourceArt/Anim/Baked`, keys and montage layout, so their scripts write them back unchanged |
| `boss_montage_sounds.py` | Put every notify and fight-loop curve of each boss montage's sound score on it — the hex boss intro's (`SourceArt/Audio/HexBossIntro/cues.json`, `loop.json`), its other montages' (`SourceArt/Audio/HexBossMontages/<montage>/`) and the star boss's (`SourceArt/Audio/StarBoss/<montage>/`); `BOSSES` picks which |
| `class_badge_anim_blueprints.py` | One AnimBlueprint per class badge: a top- or bottom-slot clip plays the whole badge over the idle, and with both playing the bottom one holds the body and the top one the parts; plus a full-body slot and an additive slot over all |
| `class_badge_body_turns.py` | Body-only rolls about the aim in the bottom slot, round the parts: the Circle rolls through the Moira beam, the Triangle rolls once reeling in its turrets; checked in 3D against the parts |
| `class_badge_circle_charge.py` | Circle badge charge beam: the hourglass whips round the badge, swells shaking in front, flattens wide on release |
| `class_badge_circle_charge_orbit.py` | Circle badge charge beam, alternative: the hourglass circles the swelling, trembling badge accelerating, then holds in front turned on release while the disc kicks back; top slot |
| `class_badge_deploy.py` | The three badges throwing their deployable, full-body slot over the auto-fire: the Square's mandibles roll faster and faster then are crushed by the recoil against their pinned backs, the Triangle rears and stakes its turret, the Circle stands its hourglass up and pours it over |
| `class_badge_death.py` | The three badges swell and vanish, shedding their class debris, and stay gone until the rez brings them back; each montage starts its sound on its first frame |
| `class_badge_rez.py` | The three badges' 3 s rez: grow back spinning to a stop, then show off while the body rolls over — one part orbits level on a rosette, a second sinks and orbits the other way underneath, swinging under the body to the far side; the Circle's hourglass whips round, snaps into reverse and tumbles; orbit radii and depths read off the rig so nothing meets the rolling body |
| `class_badge_idle.py` | The three badges breathe and, bored, play with their parts: twirling, drumming, looking round, flipping, rolling round the rim |
| `class_badge_square_fire.py` | Square badge auto-fire: the mandibles take turns thrusting out, flaring and slamming back |
| `class_badge_square_fire_piston.py` | Square badge auto-fire, alternative: the mandibles cock outward and jab out long, top slot only |
| `class_badge_square_sacrifice.py` | Square badge Martyr Beam and Martyr's Wrath, bottom slot on the block only: pumps the channel into its keyhole winding itself over about the aim, is slammed back and rolled over by the ray; checked against the auto-fire |
| `class_badge_triangle_fire.py` | Triangle badge heavy shot, two cuts: FireRatchet cranks the plug back in three clicks and flings the needle out (the auto-fire), FireFormation spreads plug and needle into a wider arrowhead (the turret launch) |
| `class_badge_triangle_fire_crossbow.py` | Triangle badge heavy shot, alternative: the needle draws back into the arrowhead like a crossbow bolt, the arrowhead flexing like its bow, and launches as a spear; top slot |
| `class_badge_triangle_reload.py` | Triangle badge reload, heavy: braces with the needle drawn in, heaves the whole badge round a yaw turn, clunks past and rocks back; full-body slot |
| `class_badge_wiring.py` | Swap the playable characters onto the badges: class data, character mesh, ability montages, materials |
| `dump_bone_motion.py` | Sample each hex and star boss montage's bones in parent space to JSON, yaw unwrapped, with the rest pose, its sections and blends — the motion a sound score is timed from; a montage samples as its reference pose, so each of its sequences is sampled across the span it plays |
| `export_anim_fbx.py` | Export every animation sequence under the class badge folder to FBX in `AI/Output/ClassBadgeAnimFbx`, one subfolder per class |
| `verify_badge_scripts.py` | Run every class badge clip script into a fresh sandbox and compare each clip and montage with the live one, frame by frame — run before re-running any of them |
| `rerig_class_badges.py` | Re-rig the three class badges from `rig_class_badges.py`, rewriting every clip on their skeletons to play exactly as before on the new rest pose; reports which bones still translate per clip |
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
| `boss_fight_loop.py` | Put each boss's fight loop layers (its score's `loop.json`) — sound, volume, curves, rest level — on its loop sound component, keeping the drift tuned on it |
| `curve_asset_authoring.py` | Write a curve asset from a table of keys |
| `generate_headshot_ding.py` | Synthesise the headshot ding and import it as a sound wave |
| `import_sound_waves.py` | Import every `SFX_` WAV of each `SourceArt/Audio` folder into its package as `SW_`, replacing same-named assets, marking `_Loop` ones looping and to play when silent; deletes unreferenced `SW_` assets whose source is gone |
| `import_textures.py` | Import image files as textures, replacing same-named assets, with the sRGB and compression they are sampled with |
| `save_dirty_assets.py` | List and save dirty content packages, reporting to a file |
| `trim_sound_start_silence.py` | Cut the silence before the first sound off every Sound Wave at the root of the SFX folder, reimported in place with its settings |
| `volume_sound_classes.py` | The volume sound classes (General over Effects, Music, Interface) and their mix; menu sounds onto Interface |
| `struct_container_edit.py` | Rewrite a struct array or struct map on an asset or a Blueprint |
| `wheel_zoom_input.py` | Create the zoom input action and bind it to the mouse wheel |

## Audio

These run from a shell with `uv run <script>`, not through the editor; each declares its own dependencies.

| Script | Purpose |
|---|---|
| `compare_sections.py` | Judge a synthesised section against a reference section: event anatomy with the backing subtracted, octave balance, stacked spectrograms |
| `dissect.py` | Take a sound apart off its bed — envelope, events, dominant lines on a 5 ms window, tracked partials and harmonic families, noise per octave, modulation, echo, stereo — with a picture; `survey` a pack in one line per sound; `sound_span` finds a sound's own span and how far under its peak it reaches, or cuts a render to a given depth |
| `electric_layers.py` | Synthesise a seamless electric loop — high-voltage hum and corona swelling on top, crackle, rumble, static snaps and fizz — one WAV per layer for Audacity |
| `fetch_reference.py` | Turn a sound-pack video into references: audio and 720p video, cuts on silence with contact sheets, on-screen names read by OCR into labelled spans, cuts named from `labels.txt` |
| `fit_patch.py` | Fit a patch's free values to its reference by CMA-ES on a multi-resolution log-mel gap plus a brightness-contour term; compare, picture and reference-then-synth A/B into `AI/Audio/Reproductions` |
| `heavy_machine.py` | Heavy futuristic machine building blocks and one-shots — electric motors under load following turn speed, gear teeth, stick-slip strain, holding hum, clamp slam, seal hiss, release blast; every part sized |
| `hex_intro_score.py` | Score the hex boss intro off its dumped motion as a heavy machine: strain and hum while the pieces shiver, crack, clamp and seal on each landing, a motor and a gear train per ring, the body straining through the gather, the release; writes the one-shots, stems, `cues.json` and a preview mix to the draft, or to `SourceArt` with `ship` |
| `hex_boss_montage_score.py` | Score the hex boss's ability and death montages: kit one-shots on beats anchored to each montage's sections, and curves bending the fight loop — the engine spooling with the wind-up, out while the boss holds still, back on the hit; the whir following the rings' turn, the rattle the shake; the death cutting the loop on its blast; writes each montage's `cues.json` and `curves.json` to the draft, or to `SourceArt` with `ship`, and a preview of each, looped live phase included |
| `hex_intro_mech_score.py` | Score the hex boss intro off its dumped motion with the mechanical kit: a sub bed dipping before each landing, the engine as the fight loop bent by montage curves (or a stem: engine or gather-only hum), the fight whir and rattle loops, a riser, shiver clicks, a clattering impact per landing, a gear train per ring, rattle and air through the gather, a dead freeze, boom and surge on the release; writes the kit, stems, `cues.json`, `loop.json` and a preview mix per hum version — the loop one as Unreal plays it, measured against the old engine stem — to the draft, or to `SourceArt` with `ship` |
| `hex_lock_layers.py` | Synthesise the hex boss intro's lock — the trailer clack: strikes, rattle, gliding knock, room, swell — one WAV per layer for Audacity; a `match_reference.py` model, its fitted variants in `hex_lock_params.json` |
| `listen.py` | Print a WAV's perceptual measures and render its waveform, spectrogram and spectrum to a PNG |
| `match_reference.py` | Match a synth to a hit in a reference recording: find hits and repeats, measure one with its backing cancelled, fit a model's parameters, compare |
| `mech_kit.py` | The mechanical kit, synthesised to the mechanical intro reference: ticks, chain clicks, clattering metal hits, sized impacts, riser, grind, air stutter and blast, boom, tone tail, surge, the break (an outro), an engine woofing with its speed, and drone, hum, rattle, gear-train and engine loops; writes each into `SourceArt/Audio/Mech` |
| `metal_one_shots.py` | Write the reusable metal one-shots into `SourceArt/Audio/Metal`: the trailer clack's cracks |
| `ninja_kit.py` | The ninja kit, air and cloth with no metal: air cuts and a flurry of them, blade swishes, sized whooshes, cloth snaps, a soft landing, a gust, inhales into a hard cut, a smoke-bomb vanish, and wind, whirl and cloth-flutter loops; writes each into `SourceArt/Audio/Ninja` with a kit preview |
| `sfx_patch.py` | Render a JSON patch: band-limited oscillators, FM, inharmonic partials, shaped noise, random blips, under step envelopes, drive, echo, room, bit reduction; free values `{"fit": [value, low, high]}`, shared ones `"@name"` |
| `zap_kit.py` | Small electric hits for a boosted shot's every hit, measured against a spell pack's electric shocks — a hash of discharges clumping in shrinking bursts, lurching noise lows, a tilted sizzle, faint gliding arcs, soft-clipped, no tone anywhere — in two styles (Zap, Crackle) of four variants, with a preview run at auto-fire rate; writes each into `SourceArt/Audio/Elec` with `ship` |
| `split_on_silence.py` | Cut a reference montage into one WAV per sound wherever it falls silent against its own floor; `sound_spans` is imported by `fetch_reference.py` |
| `star_boss_score.py` | Score the star boss with the ninja kit off its dumped motion: its fight loop (wind, whirl, flutter) and every montage's one-shots on beats found in the motion — an air cut per point stabbing out, an inhale into the still frames, a gust on the nova — with curves from the spin and the shake, looping sections tilted to close; writes `loop.json` and each montage's `cues.json` and `curves.json` to the draft, or to `SourceArt` with `ship`, and a preview of each |

`Patches/<Pack>/` holds one patch per reproduced sound, its fitted values written back by `fit_patch.py`.

## Level

| Script | Purpose |
|---|---|
| `arena_walls.py` | Place every arena wall and barrier wall state on its floor outline (`AI/ArenaWalls.md`) |
| `level_settings.py` | Edit a level's world settings, GameMode override and controller class |
| `tutorial_room.py` | Build the tutorial room in the draft map |

## Material

`material_graph_authoring.py` is the toolkit the function-based scripts import.

| Script | Purpose |
|---|---|
| `compare_material_layers.py` | Compile candidate layers in one slot of a material's layer stack and report each one's instruction counts |
| `dry_run_material_build.py` | Run material build scripts outside the editor against a stand-in module, checking every function-call pin |
| `make_arena_rail_material.py` | Arena border rail — core line, halo and travelling quads on each wall's top face, one instance per arena, wired onto the placed walls |
| `make_beam_and_telegraph_materials.py` | The beam and both zone telegraphs from shared functions — `M_PulseBeam`, `M_ZoneIndicatorRay`, `M_ZoneIndicator`: frame or ring, the beam's fill in the meaning pattern, the telegraphs' in world stripes of their colours (a lone colour paired with a shade of itself), the beam's pulse and life wipe |
| `make_background_lattice_material.py` | Floor triangle line art — the layer stack, its pattern and ring layers, the glow blend, the pulse collection |
| `make_background_look_instances.py` | One material instance per floor look, the form arenas and the settings' pool cycle through |
| `make_background_looks.py` | Floor glow looks, one layer each — two-tone, shock and polygon rings, halos, spiral, whirl, Sierpinski, radar, twinkle, fireflies |
| `make_color_pattern_functions.py` | Colour patterns splitting a surface into one region per gameplay meaning — zigzag bands, overlapping quads, stripes — index wrapping, colour picking, and `MF_MeaningColors`, the one call every multi-colour effect makes, laying the pattern over the world at `MPC_MeaningColors`' size and speed; its `PATTERN` picks the pattern for all of them |
| `make_pulse_circle_material.py` | `M_PulseCircle`, every zone's disc, rebuilt from functions: outline, inward pulse, up to four fill colours through `MF_MeaningColors`, life wipe |
| `make_class_badge_materials.py` | Class badge body material: logo line masks projected from the pre-skinned position onto the top face, one instance per class, wired into the class data and the badge meshes |
| `make_generic_material_functions.py` | Standard functions — distances to shapes, polar coordinates, triangle cells, strokes, two-tone glows, the Sierpinski mask, random per cell, Lissajous paths, the clock wipe |
| `material_graph_authoring.py` | In-place graph rebuilds, asserted wiring, function and layer pins, calls, parameters, layer stacks |
| `read_material_text.py` | Export a material as text and read the lines naming given properties, such as a constant-driven input |

## Mesh

A triangle's front face is its clockwise side, seen from outside. Generators outline counter-clockwise and reverse
the order in their one triangle helper. A reversed mesh looks right from the top-down camera, which sees its inner
bottom faces through the culled top, but anything passing through it at mid-height draws on top of it. Check a
new mesh against the engine cube: the cross product of a top-face triangle's two edges points down.

| Script | Purpose |
|---|---|
| `generate_bomb_mesh.py` | Round bomb mesh sized to replace the pillar |
| `generate_class_badge_meshes.py` | The three class logo silhouettes, extruded to a height per class; each centred on its box; the Triangle body thins toward its point |
| `generate_hex_boss_mesh.py` | Hex boss body as three concentric hexagons, and the rig it emits |
| `generate_star_mesh.py` | Star boss body: eight long points with counter-points between, a star-shaped hole through the heart |
| `rig_hex_boss.py` | Turn that body into a skeletal mesh on a new skeleton |
| `rig_star.py` | Rebuild SKM_Star from that body on SK_Star's own hierarchy, weighted so every existing star clip plays unchanged |
| `rig_class_badges.py` | Rig the three class badges: body on a bottom layer, each floating part on its own bone under a top layer, fire sockets fixed on the root ahead of the badge, and the Square keyhole socket |

## Niagara

| Script | Purpose |
|---|---|
| `meaning_color_bindings.py` | The beam and telegraph systems' meaning colour User parameters, bound to their material's colour parameters |
| `buff_vfx.py` | Every buff a character or its shots can wear, in one style |
| `charged_trail_vfx.py` | Electric trail left behind a damage-boosted shot |
| `cone_coil_vfx.py` | Electric helix wound around a cone, turning about its axis |
| `death_debris_persist.py` | The class death debris stay on the floor until revive: particles never die, no fade or shrink |
| `decode_niagara_parameters.py` | Decode a rapid-iteration store fetched from the property API |
| `empowered_arc_vfx.py` | Electric arc running over a character's own mesh |
| `empowered_aura_vfx.py` | Three empowerment auras — light flames wrapping a character |
| `empowerment_vfx.py` | The empowerment aura, authored for the orthographic view |
| `geo_aura_vfx.py` | The five character auras, rebuilt as geometry |
| `lightning_variants.py` | Three electric declinations, each on a mechanism the others lack |
| `niagara_stack_edit.py` | Stack editing through the builder shim — modules, switches, inputs |
| `sacrifice_release_vfx.py` | Square sacrifice detonation: the sacrifice spat out of the keyhole as a ray of square links, slugs shot down it |
| `sacrifice_hole_vfx.py` | Square badge keyhole during the sacrifice channel: squares churning in the hole, frames and a ring of squares sucked into it |
| `set_outline_vfx_flags.py` | The two flags deciding how a translucent effect meets the outline |
| `static_electricity_vfx.py` | Blue bolts striking around a character |

## Runtime

| Script | Purpose |
|---|---|
| `couch_coop_debug.py` | Dump couch-coop input ownership from a running session |
| `pie_drive_menu_ui.py` | Drive live menu widgets without simulating input |
| `pie_inject_input.py` | Inject input actions into a session and measure the gameplay result |
| `run_via_bridge.py` | Host-side: run an editor script file over the bridge's HTTP route and print OK or its traceback |
| `vfx_editor_preview.py` | Set up the editor world for the level viewport to judge — preview systems, collection values, the camera |

## UI

| Script | Purpose |
|---|---|
| `ability_bar.py` | Ability bar pipeline — cooldown sweep material, slot widget, bar widget |
| `charge_beam_gauge.py` | Create a widget Blueprint, build its tree, wire it to a component |
| `crosshair_cursor.py` | Crosshair software cursor, bound to the cursor slot |
| `group_widgets.py` | Wrap existing canvas children into one panel without moving them |
| `leaderboard_menu.py` | Leaderboard widget on the shared list frame, its main-menu entry and the fight timer beside the boss bar — runs after `list_panel.py` |
| `list_panel.py` | The frame every full-screen list wears, with the server browser re-rooted onto it — runs after `list_row.py` |
| `list_row.py` | The row every list is built from (RowButton + ColumnsBox), shared by the server browser and the leaderboard |
| `local_connect_menu.py` | Build a child panel inside an existing menu widget |
| `main_menu_layout.py` | Centre the main menu button column and space its buttons evenly |
| `pause_menu_setup.py` | Centered vertical menu of labeled button rows |
| `second_player_gamepad_toggle.py` | The couch-coop gamepad row on the key-bindings widget |
| `volume_sliders.py` | The four labelled volume sliders of the sound settings widget |

## Video

These run from a shell with `uv run <script>`, not through the editor, on the FFmpeg on PATH.

| Script | Purpose |
|---|---|
| `video_scan.py` | See a video without watching it: timestamped contact sheets, scene cuts, loudness every 100 ms, a GPU transcript with word timings, and a timeline joining them per window with the loudest ranked; a span zooms in, keeping source times |
| `video_bar.py` | When an on-screen bar empties and refills over a whole video — a recording player's deaths from their health bar |
| `edit_to_fcpxml.py` | Turn an edit list into an FCPXML timeline and a captions SRT that DaVinci Resolve imports with every part editable — cuts, follow zoom as keyframes, titles, sounds, cards |
| `locate_frame.py` | Find where a reframed frame sits in its source frame — the crop centre a zoom actually produced, to check against the one intended |
| `video_render.py` | Render an edit list in one FFmpeg pass: cuts from any sources, image and colour cards, source crop, a zoom window following keyed points, one-shot sounds placed on the timeline, framing to any size (fill with focus, fit, blurred fill), speed, text, transitions with per-clip hard cuts, word captions, a ducked music bed, loudness normalisation under a limiter |
