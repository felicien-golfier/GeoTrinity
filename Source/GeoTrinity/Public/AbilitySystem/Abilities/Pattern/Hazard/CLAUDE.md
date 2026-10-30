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
- One `UGeoZoneVisualComponent` (`Beam`) created in `OnCreate`, reused: layer `Telegraph` during the wind-up (fill = wind-up spent), `Ground` live; hidden at the end, natural or forced (no fade).
- Placed before shown: `InitPattern` calls `MoveBeamVfx` (and a child uploads its mask) *before* `SetZoneVisible(true)`, so the first drawn frame already has its final place, size and cuts; never a frame at the component's default spot / `BeamRange` size.
- Wind-up telegraph rule: the telegraph never grows. It is the hazard's whole footprint from its first frame, cuts for the safe spots included; nothing in it moves except the fill.
  - Fill = the wind-up spent (`StartDelay`), reaching its end exactly when the hazard starts. Its speed is `length / StartDelay`, independent of the hazard's own speed: equal speeds = equal times, set by the designer.
  - Fill direction = data, picked per Blueprint, telegraph and live hazard apart: `TelegraphFillMode` / `HazardFillMode` (`EGeoFillMode`: `FromCore`, `AlongBeam`, `None`; `World/ZoneLayer/CLAUDE.md`). Defaults `FromCore` (hex beam: across the width from its middle line); the lateral wave's constructor sets both `AlongBeam`. Rules choosing them come later; until then, each case is decided with the user.
  - Beam tied to its caster or moving under conditions (hex boss beam): the telegraph only shows where it starts.
  - Ambiguous beam or zone: the user decides; ask, never guess.
- `OverlapMode` (`ETargetOverlapMode`, default Automatic = centre-only for Enemy source).
- Debug: `Geo.DrawBeamBorder`.

## TileCarveRay
- `SweepAngle = 0` + `bDestroyLastTileHit`; direction locks at activation so the tank chooses the rim tile.
- The tile is destroyed in `EndPattern` before the base broadcasts `OnPatternEnd`: that ends the ability, which force-stops the pattern re-entrantly.

## DevastatingWave
- Radius grows at `ExpansionSpeed` to `MaxRadius`; hit band `[R - AnnulusWidth, R]`, swept `FrontOffsets`.
- `InitPattern` teleports the instigator to `Payload.Origin` (target point from the ability).
- Telegraph = one zone-layer `Circle` (`Telegraph` layer, `Radius = MaxRadius`, `bSafeSpotCut`, fill `TelegraphFillMode` (default `FromCore`), timed off `Payload.ServerSpawnTime` over `StartDelay` like the beam): full size at once, pillar shadows (`Behind`) cut out from `InitPattern` on, only the fill grows from the centre. Cosmetic, every rendering machine, own component created in `OnCreate`, hidden at `StartPattern`.
- Live wave: masked AOE VFX (`AOEVfxSystem`, `MaskMaterialParameterCollection`) from `StartPattern`; no Niagara telegraph.
- Safe spots = pillars, as `FSafeSpot` (`ESafeSpotShadow::Behind`) in an `FSafeSpotMask` (`SafeSpot/CLAUDE.md`); hit test and mask read the same list.
- Pillar detection runs on all machines (deterministic); damage server-only.
- Debug: `Geo.DrawDevastatingWave`.

## LateralWave
- `UBeamPattern` child, a *pattern with spots to keep safe*: no lane logic of its own beyond the visual. Beam rectangle = `Payload.Origin` along the travel yaw (the pattern's own, never the caster's facing), `TravelDistance` long, `2 * HalfWidth` wide; the origin is the ability's `TargetPointTag` point.
- Travel yaw: `TravelYaw` (default, 0 = +X) unless the firing's `FLateralWavePatternData::bRandomDirection` is set (set only by the tutorial miniboss for now; the Zones room keeps its fixed yaw). Then `InitPattern` rolls one of 0 / 90 / 180 / 270 from `Payload.Seed` (same on every machine) and uses it for the whole firing.
- A rolled wave starts at the arena edge opposite its travel and crosses the arena along it (start, `TravelDistance`, `HalfWidth` derived from the arena bounds along / across the roll, so a non-square room works); a fixed-yaw wave keeps the origin and values it is given.
- Front crosses `TravelDistance` at `Speed`; the band `[front - BandWidth, front]` hits everything inside the rectangle, minus the safe spots.
- One wave, one ability, one pattern Blueprint (`GA_LateralWave` / `BP_LateralWave`): what changes per firing is runtime data, never a new asset.
- Spots: `FSafeSpot` (`ESafeSpotShadow::Lane`, direction = the travel) gathered every tick from `SafeSpotSources` (default `Fixed` + `Adds`; `SafeSpot/CLAUDE.md`), radius `SpotRadius`.
  - Lanes always run parallel to the travel: they turn with the wave, a rolled one included.
  - `Fixed` = `FLateralWavePatternData::SafeSpotOffsets` (X along / Y across the travel, from the origin; turned with the wave), given by whoever fires the wave (`GeoActivateAbilityAction::PatternData`, `Base/CLAUDE.md` "PatternAbility"); replicated with the pattern start, so every machine draws them. Fired with no data (state tree) = no fixed spot.
  - `Adds` = living adds of the caster's arena; server only (the tutorial is solo). None in a room without adds.
- Visual = the base beam, one component, the `Lane` spots cut by the zone layer: wind-up = the full rectangle (`TravelDistance` x `2 * HalfWidth`) at once with its lanes cut, fill `AlongBeam` (start edge → far edge over `StartDelay`, rule in "Beam"), live = the band, filling `AlongBeam` across the room. `MoveBeamVfx` lays the rectangle, uploads `GetSafeSpots()` (+ `TravelYaw` direction) to the mask MPC and sets `bSafeSpotCut` on its component every tick, so a dead add's lane closes at once (`SafeSpot/CLAUDE.md` "Mask"; `AI/Materials.md` "Safe spots").
- Mask MPC: own `MaskMaterialParameterCollection` (`MPC_MaskedArea`); cleared at `EndPattern` and at `InitPattern` *before* `Super::InitPattern` (the base's first placement uploads the spots; clearing after would show one uncut frame).
- Base hooks: `MoveBeamVfx` virtual; the child overrides `MoveBeamVfx` (rectangle + mask), `IsInHazard` (band + spots), `GetHazardDuration`, `InitPattern` / `EndPattern` (mask clear).
- Hit test server only, lag-compensated by the base (spots read at judge time). Hit test and mask read the same `GetSafeSpots()`.

## SpawnPillar
- Zone locations come from `FSpawnPillarPatternData` (ability's `CreatePatternData`).
- Server: pillars spawn in `InitPattern` with `PillarParams.InitDuration` = wind-up left (`StartDelay - TravelTime`), so they drop in during the telegraph, `bAutoFinishInit` false: inert (not a hazard target) until the natural `EndPattern` (server: after `HazardJudge.IsOver`, the lag-compensated judge done) calls `FinishInit()`. A force-stop expires them (`Expire(true)`).
- Effects per hostile when the base hazard judge reaches it; the pattern ends once it `IsOver`.
- Base `InitCue` = countdown telegraph, `DirectionCue` too: one per spawn point (`ExecuteGameplayCue` override), radius `SpawningZoneSize`; clients end in `StartPattern`.
