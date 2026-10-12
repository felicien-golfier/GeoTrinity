# Abilities/Pattern/Hazard

Ticking patterns whose damage is a server-judged volume. All lag-compensated (rules in `../CLAUDE.md`). VFX runs on every rendering machine, damage server-only.

| File | Role |
|---|---|
| `BeamPattern.h` | Static or sweeping beam; `SweepAngle` 0 = static ray, 90 = sweep |
| `TileCarveRayPattern.h` | Beam that destroys the furthest standing hex tile it reaches (keeps hex out of `UBeamPattern`) |
| `DevastatingWavePattern.h` | Expanding radial wave, pillars cut safe-zone shadows |
| `SpawnPillarPattern.h` | Telegraphed zones → effects, then pillars |
| `ZonePattern.h` | Circle telegraph at payload origin, then effects or a lingering zone (`ZoneParams.LifeDrainMaxDuration`) |

## Beam
- Fired from `Payload.Origin` along `Yaw` for `BeamDuration`; hex boss ships two BP children (sweep laser, tile-carving ray).
- Hits on entering (swept angle per target, `AnglesFromBeam`); per-second effects tick on everyone inside.
- `GetBeamOrigin()` + `MoveBeamVfx(SpentTime)` = the single aim path (wind-up and live).
- `PreviewSystem` (`GameDataSettings::BeamPreviewSystem`) → `BeamVfxSystem` via `GeoNiagaraParams::ApplySwappableAsset`.
- VFX component spawned deactivated in `OnCreate`, reused; graceful `Deactivate` on natural end, `DeactivateImmediate` on force-stop.
- `OverlapMode` (`ETargetOverlapMode`, default Automatic = centre-only for Enemy source).
- Debug: `Geo.DrawBeamBorder`.

## TileCarveRay
- `SweepAngle = 0` + `bDestroyLastTileHit`; direction locks at activation so the tank chooses the rim tile.

## DevastatingWave
- Radius grows at `ExpansionSpeed` to `MaxRadius`; hit band `[R - AnnulusWidth, R]`, swept `FrontOffsets`.
- `InitPattern` teleports the instigator to `Payload.Origin` (target point from the ability).
- Masked AOE VFX (`AOEVfxSystem`, `MaskMaterialParameterCollection`): telegraph in `InitPattern`, expanding wave from `StartPattern`; 8 MPC pillar slots, `ClearData()` at init/start/end.
- Pillar detection runs on all machines (deterministic); damage server-only.
- Debug: `Geo.DrawDevastatingWave`.

## SpawnPillar
- Zone locations come from `FSpawnPillarPatternData` (ability's `CreatePatternData`).
- Effects per hostile when `ZoneExpiry` (`FGeoLagCompensatedEvent`) judges it; pillars spawn and pattern ends once it `IsOver`.
- `DelayGameplayCueTag` countdown cue per spawn point; clients end in `StartPattern`.
