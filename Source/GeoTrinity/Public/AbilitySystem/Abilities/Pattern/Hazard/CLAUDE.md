# Abilities/Pattern/Hazard

Ticking patterns whose damage is a server-judged volume. All lag-compensated (rules in `../CLAUDE.md`). VFX runs on every rendering machine, damage server-only.

| File | Role |
|---|---|
| `BeamPattern.h` | Static or sweeping beam; `SweepAngle` 0 = static ray, 90 = sweep |
| `TileCarveRayPattern.h` | Beam that destroys the furthest standing hex tile it reaches (keeps hex out of `UBeamPattern`) |
| `DevastatingWavePattern.h` | Expanding radial wave, pillars cut safe-zone shadows |
| `LateralWavePattern.h` | `UBeamPattern` child: wide band crossing the room sideways; safe spots given at runtime (`FLateralWavePatternData`) or from living adds |
| `SafeSpot/` | Safe spots shared by every hazard: model, sources, MPC mask → `SafeSpot/CLAUDE.md` |
| `SpawnPillarPattern.h` | Telegraphed zones → effects, then pillars |
| `ZonePattern.h` | Circle telegraph at payload origin, then effects or a lingering zone (`ZoneParams.LifeDrainMaxDuration`); the zone is `DefaultZoneClass` tinted by `ZoneParams.Color`, never a zone Blueprint per tint |

## Beam
- Fired from `Payload.Origin` along `Yaw` for `BeamDuration`; hex boss ships two BP children (sweep laser, tile-carving ray).
- Hits on entering (swept angle per target, `AnglesFromBeam`); per-second effects tick on everyone inside.
- `GetBeamOrigin()` + `MoveBeamVfx(SpentTime)` = the single aim path (wind-up and live); `MoveBeamVfx` is virtual, the VFX component protected, for children that draw something else with the same beam (lateral wave).
- `PreviewSystem` (`GameDataSettings::BeamPreviewSystem`) → `BeamVfxSystem` via `GeoNiagaraParams::ApplySwappableAsset`.
- VFX component spawned deactivated in `OnCreate`, reused; graceful `Deactivate` on natural end, `DeactivateImmediate` on force-stop.
- `OverlapMode` (`ETargetOverlapMode`, default Automatic = centre-only for Enemy source).
- Debug: `Geo.DrawBeamBorder`.

## TileCarveRay
- `SweepAngle = 0` + `bDestroyLastTileHit`; direction locks at activation so the tank chooses the rim tile.
- The tile is destroyed in `EndPattern` before the base broadcasts `OnPatternEnd`: that ends the ability, which force-stops the pattern re-entrantly.

## DevastatingWave
- Radius grows at `ExpansionSpeed` to `MaxRadius`; hit band `[R - AnnulusWidth, R]`, swept `FrontOffsets`.
- `InitPattern` teleports the instigator to `Payload.Origin` (target point from the ability).
- Masked AOE VFX (`AOEVfxSystem`, `MaskMaterialParameterCollection`): telegraph in `InitPattern`, expanding wave from `StartPattern`.
- Safe spots = pillars, as `FSafeSpot` (`ESafeSpotShadow::Behind`) in an `FSafeSpotMask` (`SafeSpot/CLAUDE.md`); hit test and mask read the same list.
- Pillar detection runs on all machines (deterministic); damage server-only.
- Debug: `Geo.DrawDevastatingWave`.

## LateralWave
- `UBeamPattern` child, a *pattern with spots to keep safe*: no lane logic of its own beyond the visual. Beam rectangle = `Payload.Origin` along `TravelYaw` (the pattern's own, 0 = +X, never the caster's facing), `TravelDistance` long, `2 * HalfWidth` wide; the origin is the ability's `TargetPointTag` point.
- Front crosses `TravelDistance` at `Speed`; the band `[front - BandWidth, front]` hits everything inside the rectangle, minus the safe spots.
- One wave, one ability, one pattern Blueprint (`GA_LateralWave` / `BP_LateralWave`): what changes per firing is runtime data, never a new asset.
- Spots: `FSafeSpot` (`ESafeSpotShadow::Lane`, direction = the travel) gathered every tick from `SafeSpotSources` (default `Fixed` + `Adds`; `SafeSpot/CLAUDE.md`), radius `SpotRadius`.
  - `Fixed` = `FLateralWavePatternData::SafeSpotOffsets` (X along / Y across the travel, from the origin), given by whoever fires the wave (`GeoActivateAbilityAction::PatternData`, `Base/CLAUDE.md` "PatternAbility"); replicated with the pattern start, so every machine draws them. Fired with no data (state tree) = no fixed spot.
  - `Adds` = living adds of the caster's arena; server only (the tutorial is solo). None in a room without adds.
- Visual = the base beam, one component, the `Lane` spots cut by the beam materials: wind-up = the whole rectangle (Ray Zone Indicator, `M_ZoneIndicatorRay`), live = the band (`BeamVfxSystem`, `M_PulseBeam`). `MoveBeamVfx` lays the rectangle, uploads `GetSafeSpots()` (+ `TravelYaw` direction) to the mask MPC and sets `User.SafeSpotCut = 1` on its component every tick, so a dead add's lane closes at once. The materials read the mask (`SafeSpot/CLAUDE.md` "Mask"; `AI/Materials.md` "Safe spots on a beam").
- Mask MPC: own `MaskMaterialParameterCollection` (`MPC_MaskedArea`); cleared at `InitPattern` and `EndPattern`.
- Base hooks: `MoveBeamVfx` virtual; the child overrides `MoveBeamVfx` (rectangle + mask), `IsInHazard` (band + spots), `GetHazardDuration`, `InitPattern` / `EndPattern` (mask clear).
- Hit test server only, lag-compensated by the base (spots read at judge time). Hit test and mask read the same `GetSafeSpots()`.

## SpawnPillar
- Zone locations come from `FSpawnPillarPatternData` (ability's `CreatePatternData`).
- Effects per hostile when `ZoneExpiry` (`FGeoLagCompensatedEvent`) judges it; pillars spawn and pattern ends once it `IsOver`.
- `DelayGameplayCueTag` countdown cue per spawn point; clients end in `StartPattern`.
