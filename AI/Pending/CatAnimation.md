# CatAnimation — pending

Goal: the Tutorial Cat (`SM_Cat`) idles looking up at the camera with an animated face, walks, and opens a portal, jumps in and sinks through the ground plane.
Docs written: `AI/Tutorial/CatAnimation.md`, `AI/Tutorial/Cat.md` ("In game"), `AI/Python/Mesh/CLAUDE.md`, `AI/Python/Anim/CLAUDE.md` ("Cat/"), `AI/Python/Material/CLAUDE.md`, `Source/GeoTrinityEditor/Public/Tool/CLAUDE.md`.

## Done (run via the bridge; reports in `AI/Output/cat_*.json`)
- [x] `rig_cat.py` → `SKM_Cat` / `SK_Cat` in `/Game/Art/Meshes/Cat`, 22 bones, `PortalPoint` at X 110
- [x] `cat_idle.py`, `cat_walk.py`, `cat_portal_jump.py` (+ `AM_Cat_PortalJump`, stock `Timed Niagara Effect` notify state 0.33 s → 3.07 s on `PortalPoint`): closure gap 0 on both loops, no new top-down part overlaps, jump ends fully below the plane from frame 83
- [x] Notify pair: the stock `UAnimNotifyState_TimedNiagaraEffect` does it (spawn at begin, graceful `Deactivate` at end): no C++ notify. `NS_TeleportPortal` plays standalone (no owner dependency, default `User.Color` = portal blue)
- [x] `make_cat_materials.py`: `M_Cat` + `MI_Cat_Base/Shade/Ink/Light`, set on `SKM_Cat`'s slots (one master and four instances rather than four materials)
- [x] Shim `UGeoAnimBuilderUtil::BuildBlendedAnimGraph` (+ `ResetAnimGraph` shared with `BuildLayeredAnimGraph`), clang-formatted; `GeoTrinityEditor` Development compiles. No headless test: it needs an AnimBlueprint asset; the script's compile result is the check

## Alive pass + Cat_Reassemble
Docs written: `AI/MCP/MCP_Animation.md` ("Making a creature read as alive"), `AI/Tutorial/CatAnimation.md` ("Living, not mechanical", "Cat_Reassemble").
Goal: the Cat reads as a living cat, not a robot; a new clip has it splashed apart on the floor, gathering back, then stretching.
- [x] `cat_clip.py`: `attack_release`, `wander` (whole-cycle sums; a lagged read is the same call at `frame - lag`), `paw_slide`, quaternion rotations, `families` for the check
- [x] `cat_idle.py` (300 frames), `cat_walk.py` (32-frame four-beat stride), `cat_portal_jump.py` (swinging wiggle, eyes, ears, tail tip), `cat_reassemble.py` (+ `AM_Cat_Reassemble`): written, `py_compile` clean, the quaternion maths checked offline; NOT run yet: the editor is closed
- [x] Open the editor, then run the four scripts in that order (`cat_idle` before `cat_reassemble`) via `python AI/Python/Runtime/run_via_bridge.py <script> 8090` and read `AI/Output/cat_<clip>.json`: loop gap 0 on idle and walk, `gap_to_idle_first_pose` ~0 on reassemble, `new_overlaps` only at the crossings while gathering, jump `below_plane_from` still set

## Face peek (every clip, head slightly up)
Goal: no Cat clip shows the head level; the face always reads from above, like `Cat_Stretch_Flat` frame 61. Rule written in `CatAnimation.md` ("Reading from the top camera").
Today: Flat head pitch 20 (the model); Reach 0, Portal jump 0, walks 0 (`cat_walk.py`, `cat_walks.py` set yaw only), Reassemble stretch / gather to check; Sprawl 65, Twist 80, idle 80 are already up.
- [x] `cat_posing.py`: add `FACE_PEEK_DEGREES = 20.0` (shared constant); Flat uses it
- [x] `cat_stretches.py` Reach: head pitch 0 → peek
- [x] `cat_portal_jump.py`: head euler `(0, 0, 0)` → `(0, peek, 0)`; check head/ear overlap and the dive (head leads down, pitch must not pop the head above the plane late)
- [x] `cat_walks.py` (Prowl, Fast): head pitch 20 (= peek, done with `Pending/CatEvilFace.md`), yaw counter as now; `Body|Head` allowed, no other new overlap in Prowl (Fast's tail pairs were already there)
- [x] `cat_reassemble.py`: gather ends and stretch head ≥ peek, finish onto idle's pose with no pop
- [x] `cat_idles.py`: each variant's head pitch ≥ peek (Toilet, Groom, Loaf, LookAround, Knead)
- [x] Add to `cat_clip.py` check: report min head pitch per clip (flag < peek outside the allowed beats)
- [x] Rerun every clip script (idle first), read `AI/Output/cat_<clip>.json`

## Left
- [x] `Tools\Build_Editor.bat` is up to date (the shim is in the DebugGame DLLs)
- [x] With the editor open, run `AI/Python/Cat/Anim/cat_anim_blueprint.py` → `ABP_Cat`, read `AI/Output/cat_anim_bp.json`
- [ ] User judges the feel in the editor (no PIE unless asked): idle head pitch / slid head / ears, walk stride, jump timing, portal size against the Cat, the cut at the plane (a throwaway skeletal-mesh actor in a scratch level only if a visual check is asked; no actor class for now)
