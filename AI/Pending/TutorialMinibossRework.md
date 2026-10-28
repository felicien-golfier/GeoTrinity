# TutorialMinibossRework — pending

Goal: the tutorial Miniboss fight is reworked and balanced: readable, fair for a first fight, short.
Priority: 1
Docs written: none yet (workflow steps 2-3 when picked up). Owner docs: `AI/Tutorial/Steps.md` (step 10), `AI/Python/Tutorial/CLAUDE.md` (`tutorial_miniboss.py`, `tutorial_assets.py` `CT_Tutorial`), `Source/GeoTrinity/Public/Actor/Arena/CLAUDE.md`.

## Discuss first (step 2)
- [ ] What feels wrong in play: health (`MinibossHealth` 500), salve damage / speed (`MinibossProjectileDamage` 8, `MinibossProjectileSpeed` 600), summon rhythm and growing zone, wave every N summons (`CyclesRequired`), the three move points.
- [ ] Rework vs tune: new phases or only numbers in `CT_Tutorial`.

## Then
- [ ] Docs (Steps.md step 10), `tutorial_miniboss.py` / `tutorial_assets.py`, bot `Miniboss` stages (`Tests/Tutorial/CLAUDE.md` coverage)
