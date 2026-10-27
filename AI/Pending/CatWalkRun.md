# CatWalkRun — pending

Goal: the two Cat walks share length and pattern so they blend perfectly (slow Prowl, long-step Fast), and a real-cat Run (gallop) clip exists, to be integrated in `ABP_Cat` later.
Docs written: `AI/Tutorial/CatAnimation.md` ("Living, not mechanical" Walk / Run, "Clips", `ABP_Cat`), `AI/Python/Cat/CLAUDE.md`, `Source/GeoTrinity/Public/Actor/CLAUDE.md`.

## Scripts
- [x] `AI/Python/Cat/Anim/cat_walks.py` — rename `Strut` → `Fast`, `STRUT_SPEED` → `FAST_SPEED`, clip `Cat_Walk_Fast`; Fast `frames` 36 → 56 (both 56); all cycle counts / lags as fractions of `frames` (`head_lag`, `TAIL_LAG` scaled) so the pattern is identical; only amplitudes differ; docstring updated
- [x] `AI/Python/Cat/Anim/cat_run.py` (evil flash left to `Pending/CatEvilFace.md`: `cat_face.py` does not exist yet) — new, `Cat_Run` ≈ 24 frames, gallop per `CatAnimation.md` ("Run"); `RUN_SPEED = 600.0`; loads `cat_clip.py` / `cat_face.py` like `cat_walks.py`; evil flash; head ≥ peek; allowed overlaps as the walks
- [x] `AI/Python/Cat/Anim/cat_anim_blueprint.py` — rename only: `STRUT_SPEED` → `FAST_SPEED`, `Cat_Walk_Strut` → `Cat_Walk_Fast` (no Run sample)
- [x] Rerun `cat_walks.py`, `cat_run.py`, `cat_anim_blueprint.py` via bridge (editor open); read `AI/Output/cat_Cat_Walk_*.json`, `cat_Cat_Run.json`: loop gap 0, no new overlaps

## Editor / Blueprint
- [x] Delete stale asset `/Game/Characters/Anim/Cat/Cat_Walk_Strut` after the rerun (redirectors none; `BS_Cat_Walk` rebuilt by the script)
- [ ] User: ABP integration of `Cat_Run` (later)

## Docs follow-up
- [x] Rename Strut → Fast in `AI/Pending/CatAnimation.md`, `CatEvilFace.md`, `CatLocomotionIdles.md`, `CatClipsPick.md`, and `AI/Python/Tutorial/Level/tutorial_encounters.py` comments if any

## Verify
- [x] Reports above; blend check: Prowl ↔ Fast at 0.5 weight keeps foot phases (no cross-over) — compare paw X per frame in the reports
