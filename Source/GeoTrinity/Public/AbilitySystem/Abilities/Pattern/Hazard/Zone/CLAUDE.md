# Abilities/Pattern/Hazard/Zone

Patterns that telegraph circles on the ground, then land there. Hazard and lag-compensation rules: `../CLAUDE.md`, `../../CLAUDE.md`.

| File | Role |
|---|---|
| `ZonePattern.h` | `UZonePattern`: parent of the family. One circle per location: telegraph, then a burst of effects or a lingering zone |
| `SpawnPillarPattern.h` | Child: pillars drop in during the telegraph, released after the hit |

A child only adds what lands with the hit. Telegraph, hit and cues belong to the parent, so the whole family looks the same.

## ZonePattern
- One circle per `GetTargetLocations()`, cues included (`ExecuteGameplayCue`) (`../../CLAUDE.md`, "Pattern.h"); how many and where = the ability's `Target` (`Base/CLAUDE.md`, "Pattern target").
- Telegraph: one `UGeoIndicatorComponent` per location for the whole wind-up, radius `ZoneParams.Size`, `ColorMask` from `ZoneParams.Color` + `SecondaryColors`. The only wind-up visual of the family. All removed in `StartPattern` and `EndPattern`.
- `ZoneParams.LifeDrainMaxDuration` picks the landing:
  - 0 = burst: `bHasHazard`, the base judge applies `GetHazardEffects()` to whoever `TeamAttitude` matches in any circle; ends once the judge `IsOver`. `StartCue` = the burst.
  - above 0 = lingering: one zone per location (`ZoneClass`, else `DefaultZoneClass`, tinted by `ZoneParams.Color`, never a zone Blueprint per tint) carrying the effects; ends at once.
- Cues fire once per location, `RawMagnitude` = `ZoneParams.Size`.

## SpawnPillar
- Boss use: `GA_SpawnPillar` = plain `UPatternAbility`, `Target` = `RandomPlayers` (`Count` 1, `ExtraBelowHealth` [0.5, 0.2]), `PreLaunchCue` on the marked players.
- Server: one pillar per target location; pillars spawn in `InitPattern` with `PillarParams.InitDuration` = wind-up left (`StartDelay - TravelTime`), so they drop in during the telegraph, `bAutoFinishInit` false: inert (not a hazard target) until the natural `EndPattern` (server: after `HazardJudge.IsOver`, the lag-compensated judge done) calls `FinishInit()`. A force-stop expires them (`Expire(true)`).
- Two effect lists: `PillarSpawnEffects` = the hit (`GetHazardEffects` override); the ability's effects = the pillar's own.
- `ZoneParams.Size` = the hit's radius; `PillarParams.Size` = the pillar's own.
