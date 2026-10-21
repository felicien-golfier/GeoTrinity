# Abilities/Pattern/Hazard

Ticking patterns whose damage is a server-judged volume. All lag-compensated (rules in `../CLAUDE.md`). VFX runs on every rendering machine, damage server-only.

| File | Role |
|---|---|
| `BeamPattern.h` | Static or sweeping beam; `SweepAngle` 0 = static ray, 90 = sweep |
| `TileCarveRayPattern.h` | Beam that destroys the furthest standing hex tile it reaches (keeps hex out of `UBeamPattern`) |
| `DevastatingWavePattern.h` | Expanding radial wave, pillars cut safe-zone shadows |
| `LateralWavePattern.h` | `UBeamPattern` child: wide band crossing the room sideways; safe spots from a fixed list or from living actors |
| `SafeSpot/` | Safe spots shared by every hazard: model, sources, MPC mask → `SafeSpot/CLAUDE.md` |
| `SpawnPillarPattern.h` | Telegraphed zones → effects, then pillars |
| `ZonePattern.h` | Circle telegraph at payload origin, then effects or a lingering zone (`ZoneParams.LifeDrainMaxDuration`) |

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
- `UBeamPattern` child, a *pattern with spots to keep safe*: no lane or strip logic of its own. Beam rectangle = `Payload.Origin` along `TravelYaw` (the pattern's own, 0 = +X, never the caster's facing), `TravelDistance` long, `2 * HalfWidth` wide; the origin is the ability's `TargetPointTag` point. The beam VFX (single component, the base's; `BeamVfxSystem`, `BeamColor`) shows that rectangle in the wind-up (Ray Zone Indicator) and the moving band after, both laid by the `MoveBeamVfx` override.
- Front crosses `TravelDistance` at `Speed`; the band `[front - BandWidth, front]` hits everything inside the rectangle (`BandWidth` = `TravelDistance` leaves the floor lethal behind the front), minus the safe spots.
- Spots: `FSafeSpot` (`ESafeSpotShadow::Lane`, direction = the travel) from `SafeSpotSources` (`Fixed` via `FixedSpotOffsets`, X along / Y across the travel, or `Adds`; radius `SpotRadius`; `SafeSpot/CLAUDE.md`); the pattern refreshes its `FSafeSpotMask` (`MaskMaterialParameterCollection`) every tick, the beam material cuts the lanes out of the band and the telegraph. Boss-specific waves = a BP child with its own spots, never a new class.
- Base hook: `UBeamPattern::MoveBeamVfx` is virtual; the child overrides it (rectangle / band), `IsInHazard` (band + spots), `GetHazardDuration`.
- Hit test server only, lag-compensated by the base (spots read at judge time). Adds are server-only (arena): the tutorial is solo.

## SpawnPillar
- Zone locations come from `FSpawnPillarPatternData` (ability's `CreatePatternData`).
- Effects per hostile when `ZoneExpiry` (`FGeoLagCompensatedEvent`) judges it; pillars spawn and pattern ends once it `IsOver`.
- `DelayGameplayCueTag` countdown cue per spawn point; clients end in `StartPattern`.
