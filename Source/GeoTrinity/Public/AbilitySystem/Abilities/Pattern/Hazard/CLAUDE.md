# Abilities/Pattern/Hazard

Ticking patterns whose damage is a server-judged volume. All lag-compensated (rules in `../CLAUDE.md`). VFX runs on every rendering machine, damage server-only.

| File | Role |
|---|---|
| `ZoneHazardPattern.h` | `UZoneHazardPattern`: parent of every hazard drawn on the zone layer: its one shape, colours, fills, telegraph → live switch, safe spot mask |
| `BeamPattern.h` | Static or sweeping beam; `SweepAngle` 0 = static ray, 90 = sweep |
| `TileCarveRayPattern.h` | Beam that destroys the furthest standing hex tile it reaches (keeps hex out of `UBeamPattern`) |
| `DevastatingWavePattern.h` | Expanding radial wave, pillars cut safe-zone shadows |
| `LateralWavePattern.h` | `UBeamPattern` child: wide band crossing the room sideways; safe spots given at runtime (`FLateralWavePatternData`) or from living adds |
| `SafeSpot/` | Safe spots shared by every hazard: model, sources, MPC mask → `SafeSpot/CLAUDE.md` |
| `Zone/` | Circle telegraphs, then a hit or a lingering zone: `UZonePattern` and its children (pillars, adds) → `Zone/CLAUDE.md` |

## ZoneHazard
- Parent of every hazard the zone layer draws (`World/ZoneLayer/CLAUDE.md`): `Beam` (→ `TileCarveRay`, `LateralWave`) and `DevastatingWave`. A new zone-drawn hazard derives from it; a later variant of one is a child of that one.
- One `UGeoZoneVisualComponent` (`HazardVisual`, protected) per pattern instance: created in `OnCreate` on rendering machines (null on a dedicated server), absolute transform, `ColorMask` from `HazardColor` + `SecondaryHazardColors`; reused per activation.
- A child only lays out its shape: `MoveHazardVisual(SpentTime)` (virtual; negative in the wind-up) sets shape, size and place. The parent calls it before showing, every wind-up tick (`TickDuringInit`) and every live tick (`TickPattern`).
- Phases, all in the parent:
  - `InitPattern`: layer `Telegraph`, `TelegraphFillMode`, `SetTimedFill(ServerSpawnTime, StartDelay)`, `MoveHazardVisual(-StartDelay)`, then shown.
  - `StartPattern`: layer `Ground`, `HazardFillMode`, `SetTimedFill(ServerSpawnTime + StartDelay, GetHazardDuration())`, `MoveHazardVisual(0)`, shown (the first show on the "too late" path).
  - `OnHazardEnd` and `EndPattern` (natural or forced): hidden, no fade.
  - "Too late" path (`UPattern::InitPattern` ran `StartPattern` at once, no wind-up timer): no telegraph; the live set-up stands.
- Placed before shown: `MoveHazardVisual` (and the child's spot upload) runs *before* `SetZoneVisible(true)`, so the first drawn frame already has its final place, size and cuts; never a frame at the component's default spot / size.
- Wind-up telegraph rule: the telegraph never grows. It is the hazard's whole footprint from its first frame, cuts for the safe spots included; nothing in it moves except the fill.
  - Fill = the wind-up spent (`StartDelay`), reaching its end exactly when the hazard starts. Its speed is `length / StartDelay`, independent of the hazard's own speed: equal speeds = equal times, set by the designer.
  - Fill direction = data, picked per Blueprint, telegraph and live hazard apart: `TelegraphFillMode` / `HazardFillMode` (`EGeoFillMode`: `FromCore`, `AlongBeam`, `None`; `World/ZoneLayer/CLAUDE.md`). Default `FromCore`; a child's constructor sets its own (lateral wave: both `AlongBeam`). Rules choosing them come later; until then, each case is decided with the user.
  - Hazard tied to its caster or moving under conditions (hex boss beam): the telegraph only shows where it starts.
  - Ambiguous hazard: the user decides; ask, never guess.
- Colour = the meaning of the hit (`Tool/Visual/CLAUDE.md`), telegraph and live alike: default `Damage`; both waves `LethalDamage` (their constructors).
- Safe spots, the one occlusion path: `SetSafeSpots(Spots, LaneDirection, Origin)` = `FSafeSpotMask::Set` on the parent's `MaskMaterialParameterCollection` (`MPC_MaskedArea`) + `bSafeSpotCut` on the shape (`SafeSpot/CLAUDE.md` "Mask"). The parent clears the mask in `InitPattern` before the first placement and at `EndPattern`; `ClearSafeSpots()` for a child that restarts its spots. A hazard with no spots never calls them (shape uncut, MPC untouched, no MPC needed).

## Beam
- Fired from `Payload.Origin` (the caster) along `GetTargetYaw()` for `BeamDuration`; hex boss ships two BP children (sweep laser, tile-carving ray).
- Hits on entering (swept angle per target, `AnglesFromBeam`); per-second effects tick on everyone inside.
- Shape `Beam`. `GetBeamOrigin()` + `MoveHazardVisual(SpentTime)` = the single aim path (wind-up and live); a child drawing something else with the same beam overrides `MoveHazardVisual` (lateral wave).
- `OverlapMode` (`ETargetOverlapMode`, default Automatic = centre-only for Enemy source).
- Debug: `Geo.DrawBeamBorder`.

## TileCarveRay
- `SweepAngle = 0` + `bDestroyLastTileHit`; direction locks at activation so the tank chooses the rim tile.
- The tile is destroyed in `EndPattern` before the base broadcasts `OnPatternEnd`: that ends the ability, which force-stops the pattern re-entrantly.

## DevastatingWave
- Radius grows at `ExpansionSpeed` to `MaxRadius`; hit band `[R - AnnulusWidth, R]`, swept `FrontOffsets`.
- Centre = the first of `GetTargetLocations()` (the ability's `TargetPoint`): hit test, visual, safe spots and cues (`FillCueParam`) all read it. `InitPattern` teleports the instigator there.
- `UZoneHazardPattern` child: zone layer only, no Niagara. `MoveHazardVisual`, on the centre:
  - wind-up: `Circle`, `Radius = MaxRadius`: the whole footprint at once; `TelegraphFillMode` `FromCore` (centre outward).
  - live: `Ring`, outer = front (`ExpansionSpeed * SpentTime`, ≤ `MaxRadius`), inner = `max(0, outer - AnnulusWidth)`: the drawn band is exactly the hit band; `HazardFillMode` `FromCore` (ring: inner radius → outer radius, over the wave's life).
- Colour `LethalDamage` (constructor), as the lateral wave.
- Safe spots = pillars, as `FSafeSpot` (`ESafeSpotShadow::Behind`, `SafeSpot/CLAUDE.md`) in `PillarSpots`, uploaded with `SetSafeSpots(PillarSpots, -, centre)` ("ZoneHazard"); hit test and mask read the same list.
  - Wind-up: every pillar (`AddPillarSpots(0)`) from the wind-up branch of `MoveHazardVisual`, so the telegraph shows all shadows from its first frame (and a pillar landing during the wind-up).
  - Live: `StartPattern` restarts the list (`ClearData` = `PillarSpots` emptied + `ClearSafeSpots`) before the base's live placement, each pillar is added as the front reaches it; the server kills those at `EndPattern`.
- Pillar detection runs on all machines (deterministic); damage server-only.
- Debug: `Geo.DrawDevastatingWave`.

## LateralWave
- `UBeamPattern` child, a *pattern with spots to keep safe*: no lane logic of its own beyond the visual. Beam rectangle = `FiringOrigin` along the travel yaw (the pattern's own, never the caster's facing), `TravelDistance` long, `2 * HalfWidth` wide; `FiringOrigin` = the first of `GetTargetLocations()` (the ability's `TargetPoint`), never `Payload.Origin`.
- Travel yaw: `TravelYaw` (default, 0 = +X) unless the firing's `FLateralWavePatternData::bRandomDirection` is set (set only by the tutorial miniboss for now; the Zones room keeps its fixed yaw). Then `InitPattern` rolls one of 0 / 90 / 180 / 270 from `Payload.Seed` (same on every machine) and uses it for the whole firing.
- A rolled wave starts at the arena edge opposite its travel and crosses the arena along it (start, `TravelDistance`, `HalfWidth` derived from the arena bounds along / across the roll, so a non-square room works); a fixed-yaw wave keeps the origin and values it is given.
- Front crosses `TravelDistance` at `Speed`; the band `[front - BandWidth, front]` hits everything inside the rectangle, minus the safe spots.
- One wave, one ability, one pattern Blueprint (`GA_LateralWave` / `BP_LateralWave`): what changes per firing is runtime data, never a new asset.
- Spots: `FSafeSpot` (`ESafeSpotShadow::Lane`, direction = the travel) gathered every tick from `SafeSpotSources` (default `Fixed` + `Adds`; `SafeSpot/CLAUDE.md`), radius `SpotRadius` for `Fixed`, `AddSpotRadius` (`FScalableFloat`, default 150, read at the payload's level: a difficulty column can narrow a dummy's lane) for `Adds`.
  - Lanes always run parallel to the travel: they turn with the wave, a rolled one included.
  - `Fixed` = `FLateralWavePatternData::SafeSpotOffsets` (X along / Y across the travel, from the origin; turned with the wave), given by whoever fires the wave (`GeoActivateAbilityAction::PatternData`, `Base/CLAUDE.md` "PatternAbility"); replicated with the pattern start, so every machine draws them. Fired with no data (state tree) = no fixed spot.
  - `Adds` = living adds of the caster's arena; server only (the tutorial is solo). None in a room without adds.
- Visual = the base beam, one component, the `Lane` spots cut by the zone layer: wind-up = the full rectangle (`TravelDistance` x `2 * HalfWidth`) at once with its lanes cut, fill `AlongBeam` (start edge → far edge over `StartDelay`, rule in "ZoneHazard"), live = the band, filling `AlongBeam` across the room. `MoveHazardVisual` lays the rectangle and calls `SetSafeSpots(GetSafeSpots(), travel direction, FiringOrigin)` every tick, so a dead add's lane closes at once (`SafeSpot/CLAUDE.md` "Mask"; `AI/Materials.md` "Safe spots").
- Mask: the parent's (`UZoneHazardPattern`), cleared by it before the first placement and at `EndPattern`.
- Base hooks: the child overrides `MoveHazardVisual` (rectangle + spots), `IsInHazard` (band + spots), `GetHazardDuration`, `InitPattern` (firing travel, set before `Super::InitPattern` reads the duration and places the beam).
- Hit test server only, lag-compensated by the base (spots read at judge time). Hit test and mask read the same `GetSafeSpots()`.
