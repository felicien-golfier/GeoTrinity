# PatternDescriptionTokens — pending

Goal: ability descriptions read their numbers from the ability's pattern (`SweepAngle`, cone angle…), so they never drift from balance.
Priority: 3
Docs written: none yet (step 3 still to do: `AI/AbilityDescriptions.md` "Tokens", `Source/GeoTrinity/Public/AbilitySystem/Data/CLAUDE.md`)

## Problem
- `{Token}` resolves only on the ability CDO, then `UGameDataSettings` (`AbilityInfo.cpp` ~l.64-84, l.196).
- Beam numbers live on the pattern Blueprint (`UBeamPattern::SweepAngle`, degrees either side of the aim), not on `GA_HexBoss_SweepBeam` (plain `UPatternAbility`): `{SweepAngle}` cannot resolve.
- So `Data/AbilityDescriptions.txt` hardcodes numbers: `Ability.Spell.SweepBeam` ("160 deg" = 2 × `SweepAngle` 80, set by hand), `Ability.Spell.ConeSpray` ("60 deg"). The `DevComment` of `SweepBeam` in `Config/Tags/GeoGameplayTags.ini` repeats it (plain string, never live).

## Read map
- Source/GeoTrinity/Private/AbilitySystem/Data/AbilityInfo.cpp — token resolver (property lookup, `{A*B}`, suffixes)
- Source/GeoTrinity/Public/AbilitySystem/Data/AbilityInfo.h — `GetResolvedDescription` / `GetResolvedStats`
- Source/GeoTrinity/Public/AbilitySystem/Abilities/Base/PatternAbility.h — how the ability references its pattern class
- Source/GeoTrinity/Public/AbilitySystem/Abilities/Pattern/Hazard/BeamPattern.h — `SweepAngle` (half-angle)
- Source/GeoTrinity/Public/AbilitySystem/Abilities/Pattern/Projectile/ — cone spray angle property
- AI/AbilityDescriptions.md — token table to extend

## Code (to implement)
- [ ] Decide with the user: resolver falls back to the ability's pattern class CDO (after the ability, before `UGameDataSettings`), vs. a per-pattern token prefix (`{Pattern.SweepAngle}`).
- [ ] `AbilityInfo.cpp` — implement the chosen lookup; unresolved still stays visible + warning.
- [ ] Doubled value: `SweepAngle` is a half-angle, so the text needs 2×. Decide: reword ("{SweepAngle} deg either side") or allow `{A*2}` / a constant factor in `{A*B}`.
- [ ] `Data/AbilityDescriptions.txt` — `SweepBeam` and `ConeSpray` use tokens, no typed numbers.
- [ ] `Config/Tags/GeoGameplayTags.ini` — `SweepBeam` `DevComment` drops the number.
- [ ] Tests per `AI/CodingStyle.md` ("Tests"): token on a pattern property resolves; a name on neither stays `{Typo}`.

## Verify
- [ ] Automated test above; Tutorial gate (`GeoTrinity.Tutorial.FullRun`) since `Source/` changes.
