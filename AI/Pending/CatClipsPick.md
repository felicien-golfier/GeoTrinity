# CatClipsPick — pending

Goal: settle the Cat's new takes (walks, stretches, idles) and finish the one-scaled-part rewrite.
Docs written: `AI/Tutorial/CatAnimation.md`, `AI/MCP/MCP_Animation.md` ("Scale as motion"), `AI/Python/Cat/CLAUDE.md`.

## Decide (user, in the editor)
- [x] Walk: Prowl + Fast kept, blended by speed; `Cat_Walk`, `Cat_Walk_Glide`, `cat_walk.py` go (`Pending/CatLocomotionIdles.md`)
- [x] Stretches: all four kept; Twist is the Tutorial's (Reassemble's stretch beat), the others for later content
- [x] Idle variants: all five kept; the owner plays them at random while standing (`Pending/CatLocomotionIdles.md`)

## Rewrite
- [x] `cat_reassemble.py`: stretch beat scales `Body` and its children; gather hop scales every part. Body-only scale via `cat_posing.py`, shake through the hold, hop as travel.
- [ ] Tune overlaps: hand over head (`Reach`, `Sprawl`, `Flat`), foot over body (`Sprawl`, `Twist`), tucked paws (`Loaf`).
- [ ] Editor check: ear fold and head tilt in `Cat_Idle_Groom`, roll signs in `Reach` / `Sprawl`, `Twist` head turn.

## Wire
- [x] Picked clips into `ABP_Cat` (`cat_anim_blueprint.py`): see `Pending/CatLocomotionIdles.md`.
