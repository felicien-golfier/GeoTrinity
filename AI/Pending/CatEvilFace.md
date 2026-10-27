# CatEvilFace — pending

Goal: the 3D Tutorial Cat sometimes flashes an "evil" face (arched squinting eyes, wide grin, small fangs, ears pinned back) in its idle and while walking.
Docs written: `AI/Tutorial/CatAnimation.md` ("Evil face", rig table, clips), `AI/Python/Cat/CLAUDE.md`.

Done (2026-10-26, headless DebugGame commandlet): mesh + rig + every clip re-run, loop gap 0 on idle / walks / run, fangs visible only in idle (frames 16-59), walks (24-35), run (8-18); `GeoTrinity.Tutorial.FullRun` green.

## Editor
- [x] Open the editor fresh (a session open before this run holds the old `SK_Cat` in memory; saving from it would overwrite the new rig). `BS_Cat_Walk` / `ABP_Cat` could not be saved by the commandlet (file move error) and did not need it: re-run `AI/Python/Cat/Anim/cat_anim_blueprint.py` if the AnimBP shows a skeleton error

## Verify
- [ ] User judges in the editor: grin / fangs read from above, idle beat timing, walk flash (16 frames per 56-frame stride; shorten `EVIL_FACE` in `cat_walks.py` if still too much), walks now peek (head pitch 20)
