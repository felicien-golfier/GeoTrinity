# ConeSprayTell — pending

Goal: ConeSpray's wind-up must show the ability playing and its facing from the first frames, not only at the loop.
Docs written: `AI/Python/Anim/CLAUDE.md` (Boss/ note on the wind-up tell).

Done: `hex_boss_abilities.py` has `tell` / `tell_frames` / `tell_focus` on `Clip`, used in `spike_reach`; ConeSpray 1.0 / 4 / 4.0 (first run at 0.7 / 4 / 8.0 showed nothing: outer blades rest ±30° off front, so cos^8 left 0.1; the middle ring's front blade hides inside the outer shell), the other two clips unchanged (tell 0, `tell_focus` = their `focus`). Syntax-checked only — nothing runs outside the editor.

## Scripts
- [ ] `AI/Python/Anim/Boss/hex_boss_abilities.py` — re-run via mcp-unreal `execute_script` (editor open; the server failed to connect this session). Rewrites `SK_HexBoss_Sequence_ConeSpray` and `SK_HexBoss_Montage_ConeSpray` (and re-saves the other two, unchanged).
- [ ] `AI/Python/Anim/Boss/dump_bone_motion.py` — only if the sound score of ConeSpray is re-timed from bone motion.

## Verify
- [ ] Report: frame 0 and last frame max blade negative (folded); "closest the rings come" > 0 through the wind-up; FireLoop closure gaps unchanged.
- [ ] Report rows frames 0–30: "blades past face" > 0, from the front blade only, changing as the crank turns. Tune `tell` / `tell_focus` off the room columns if a shell is punched.
