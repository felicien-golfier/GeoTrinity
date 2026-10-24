# Abilities/Pattern/Hazard/SafeSpot

Safe spots of a hazard: places that shield from it. One system for every boss hazard; the pattern owns the *spots* (code, deterministic), the VFX only reads them through the mask MPC below (devastating: its AOE material; lateral wave: the beam materials, `../CLAUDE.md` "LateralWave").

| File | Role |
|---|---|
| `SafeSpot.h` | `FSafeSpot` (`Location`, `Radius`, `Shadow`, `Source` = the pillar, null otherwise) + `ESafeSpotShadow` + pure geometry `Shields(Target, Origin, Direction)` / `AnyShields(Spots, ...)` |
| `SafeSpotMask.h` | `FSafeSpotMask`: static upload of spots to the mask MPC (`MPC_MaskedArea`, 8 slots); `GetSlotValues` = the pure slot builder |
| `SafeSpotSource.h` | `ESafeSpotSource` + `GatherSafeSpots(...)`: where a pattern takes its spots from; a pattern keeps its own `TArray<FSafeSpot>` |

## Model
- A spot is a disc (`Location`, `Radius`) that casts a shadow; a target inside a shadow is not hit.
- `ESafeSpotShadow::Disc` (default): the disc itself, `Radius` around `Location`; nothing else is shadowed (a hole in a beam).
- `ESafeSpotShadow::Behind`: radial shadow away from the hazard origin, starts at the spot (pillars; devastating wave).
- `ESafeSpotShadow::Lane`: strip of the spot's width parallel to a `Direction`, both sides of the spot (lateral wave: the whole line through the spot is safe). On a beam: the beam is cut across its width wherever a spot projects perpendicularly, so it stays visible only between the lanes.
- `Shields` is the one geometry: hit test (server, per target, lag-compensated by the base) and VFX mask read the same spots. Devastating's former `IsBehindPillar` is the `Behind` case of it.

## Sources (`ESafeSpotSource`, a pattern picks one or several)
- `Pillars`: living `AGeoPillar` of the caster's hostile-or-neutral set; radius = collision radius. Devastating adds them as its front reaches them.
- `Adds`: living adds of the caster's arena (`AGeoArena::GetLiveAdds()`, server only: a client sees none, the tutorial is solo); radius = the pattern's `SpotRadius`.
- `Fixed`: world locations the pattern computes from offsets from the payload origin in its own frame, taken from its runtime pattern data (lateral wave: `FLateralWavePatternData`, given by the encounter per firing); radius = `SpotRadius`.
- Read every tick: a dead add drops its spot at once. A spot exists from the telegraph on so the player reads it before the front moves.

## Mask (VFX)
- `FSafeSpotMask::Set(World, Mpc, Spots, LaneDirection)` writes `PillarPosWS_XX` (alpha 1 = slot used, 0 = free) / `Pillar_Radius` / `SafeSpot_ShadowMode` / `SafeSpot_LaneDirection` (`GeoMaterialParams`) for the first 8 spots (radius and shadow mode of the first, direction = the travel's unit XY, read by `Lane` only), rest logged and skipped; `Clear` = `Set` with no spots, the unused sentinel in all 8.
- The MPC is global: `Clear` at init and end of every pattern that uses it (devastating also at start); two patterns using it at once is unsupported (one live hazard per boss).
- Material side: the shadow mode is the material scalar `SafeSpot_ShadowMode` (the `ESafeSpotShadow` ordinal: 0 Behind, 1 Lane, 2 Disc), written with the MPC upload. For a used slot, a pixel P is cut when `Disc`: `distance(P, Spot) <= Pillar_Radius`; `Lane`: `|dot(P - Spot, Right(LaneDirection))| <= Pillar_Radius`. `MF_SafeSpotCut` (`/Game/Art/VFX/AOE/Functions`) outputs that `CutMask`; it ignores `Behind` (devastating's own material handles it).
- Opt-in: the MPC is global, so a beam material applies `CutMask` only where its `SafeSpotCut` is 1 (`User.SafeSpotCut`, `GeoNiagaraParams::SafeSpotCut`, default 0). The pattern that uploads the spots sets it on its own VFX; every other beam ignores the mask.
- Readers: devastating's AOE material (`Behind`), the beam materials `M_PulseBeam` / `M_ZoneIndicatorRay` (`Disc` or `Lane`, cut out of fill and outline). No used slot = nothing cut, so every other beam is untouched; a material that ignores the MPC shows no cut, damage stays right.
- A beam pattern picks the mode by the shadow it gives its spots: the lateral wave `Lane` (+ direction), any other beam `Disc`.
- Dedicated server never touches the MPC.
