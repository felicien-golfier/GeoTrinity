# CatLocomotionIdles — pending

Goal: the Cat walks with two takes blended by its real speed (Prowl slow, Fast fast), plays every idle variant at random while it stands, and the Tutorial's stretch is Twist; the other clips are cut or kept as decided.
Docs written: `AI/Tutorial/CatAnimation.md` ("Living, not mechanical", "Cat_Reassemble", "Clips", `ABP_Cat`), `AI/Python/Cat/CLAUDE.md`, `Source/GeoTrinity/Public/Actor/CLAUDE.md` ("GeoCat"). Settles `Pending/CatClipsPick.md` "Decide".
Assumption to confirm: "stretch in tuto" = `Cat_Reassemble`'s stretch beat (the only stretch the Tutorial plays), now the Twist pose. If you meant a separate Twist montage at some line, say so.

## Clips (scripts, via the bridge, `cat_idle.py` first)
- [x] `cat_posing.py`: `twist_spec(shake)` (taken from `cat_stretches.py`'s Twist), `paws`, `shake_between`, shared
- [x] `cat_stretches.py`: Twist reads `twist_spec`; Reach / Sprawl / Flat stay as they are
- [x] `cat_reassemble.py` (run, gap 0, `py_compile` clean; re-run in the editor): stretch beat = `twist_spec` (hold with tremble, release to idle's first pose, no pop); re-run, read `AI/Output/cat_reassemble.json` (`gap_to_idle_first_pose` ~0)
- [x] (written, run in the editor to get the report) `cat_walks.py`: keep Prowl + Fast, drop Glide; `PROWL_SPEED` / `FAST_SPEED` constants = stride length over duration of each clip (stride from the paw slide amplitude, report it in `AI/Output/cat_walk_*.json`)
- [x] Deleted: `cat_walk.py`, the `Cat_Walk` and `Cat_Walk_Glide` assets, their reports
- [x] (written; run in the editor) `cat_idles.py`: a montage per variant, `AM_Cat_Idle_<Name>` (`build_montage` as in `cat_portal_jump.py`, `DefaultSlot`, short blends since each variant starts and ends on idle's first pose); re-run, read the reports

## AnimBP
- [x] Editor shim written + clang-formatted (`BuildBlendedAnimGraph` takes the blend space + range, new generic `BuildBlendSpace1D`); built (Development editor beside the open DebugGame one, compiles); the open editor still runs the old DLL, so the scripts below wait for a restart on a DebugGame build
- [x] `cat_anim_blueprint.py` (run: ok, creates the blend space itself): create `BS_Cat_Walk` in `/Game/Characters/Anim/Cat`, rebuild `ABP_Cat`, read `AI/Output/cat_anim_bp.json`

## Code (`Source/GeoTrinity`)
- [x] (written, unbuilt) `Actor/GeoCat.h/.cpp`: `AnimSpeed` becomes cm/s (the real speed while walking, eased at `SpeedBlendRate`, default raised to match the unit); `IdleVariants` (`TArray<TObjectPtr<UAnimMontage>>`), `IdleVariantPause` (`FVector2D`, 6-14 s), a countdown run by `AdvanceMove` while the cat stands (armed at `BeginPlay`), a shuffled bag `PickIdleVariant()` (public for tests), `BeginOrder` stops a playing variant; no count while hidden (no tick work)
- [x] (run: five variants set) `AI/Python/Cat/Anim/cat_blueprint.py`: `IdleVariants` = the five `AM_Cat_Idle_*`; re-run
- [ ] `AI/Python/Tutorial/Level/tutorial_encounters.py`: `CAT_SPEED` 150 = pure Prowl, up to 300 blends to Fast; set the Landing walk to Fast-ish and the roam to Prowl-ish once the user has judged them (`encounters_diff.py` first)

## Tests (`GeoTrinity.Cat.*`, `AI/CodingStyle.md` "Tests")
- [x] (written, unrun) `IdleVariantBag`: over N draws every montage appears before any repeats, never the same twice in a row; empty list returns null
- [x] (written, unrun; checks the countdown only, a playing montage needs an anim instance) `IdleVariantsStandOnly`: no variant during a walk order; a walk order stops a playing variant (headless: the draw and the guard, no montage played)
- [x] Built (DebugGame editor, 2026-10-27): `GeoTrinity.Cat`, `GeoTrinity.Encounter`, `GeoTrinity.Characters` green; the whole `GeoTrinity.Tutorial.` set green (31/31, FullRun included)

## Verify
- [ ] User, in the editor: Prowl at slow speeds, Fast at fast, the blend between; each idle variant plays over time without a pop; the Twist entrance stretch

