# Abilities/Pattern/Hazard/SafeSpot

Safe spots of a hazard: places that shield from it. One system for every boss hazard; the pattern owns the *spots* (code, deterministic), the VFX only reads them.

| File | Role |
|---|---|
| `SafeSpot.h` | `FSafeSpot` (`Location`, `Radius`, `Shadow`, `Source` = the pillar, null otherwise) + `ESafeSpotShadow` + pure geometry `Shields(Target, Origin, Direction)` / `AnyShields(Spots, ...)` |
| `SafeSpotMask.h` | `FSafeSpotMask`: static upload of spots to the mask MPC (`MPC_MaskedArea`, 8 slots); `GetSlotValues` = the pure slot builder |
| `SafeSpotSource.h` | `ESafeSpotSource` + `GatherSafeSpots(...)`: where a pattern takes its spots from; a pattern keeps its own `TArray<FSafeSpot>` |

## Model
- A spot is a disc (`Location`, `Radius`) that casts a shadow; a target inside a shadow is not hit.
- `ESafeSpotShadow::Behind`: radial shadow away from the hazard origin, starts at the spot (pillars; devastating wave).
- `ESafeSpotShadow::Lane`: strip of the spot's width parallel to a `Direction`, both sides of the spot (lateral wave: the whole line through the spot is safe).
- `Shields` is the one geometry: hit test (server, per target, lag-compensated by the base) and VFX mask read the same spots. Devastating's former `IsBehindPillar` is the `Behind` case of it.

## Sources (`ESafeSpotSource`, a pattern picks one or several)
- `Pillars`: living `AGeoPillar` of the caster's hostile-or-neutral set; radius = collision radius. Devastating adds them as its front reaches them.
- `Adds`: living adds of the caster's arena (`AGeoArena::GetLiveAdds()`, server only: a client sees none, the tutorial is solo); radius = the pattern's `SpotRadius`.
- `Fixed`: world locations the pattern computes from offsets from the payload origin in its own frame (step 9 waves: a BP child per wave, spots move and shrink); radius = `SpotRadius`.
- Read every tick: a dead add drops its spot at once. A spot exists from the telegraph on so the player reads it before the front moves.

## Mask (VFX)
- `FSafeSpotMask::Set(World, Mpc, Spots)` writes `PillarPosWS_XX` / `Pillar_Radius` / `SafeSpot_ShadowLane` (`GeoMaterialParams`) for the first 8 spots (radius and shadow mode of the first), rest logged and skipped; `Clear` = `Set` with no spots, the unused sentinel in all 8.
- The MPC is global: `Clear` at init and end of every pattern that uses it (devastating also at start); two patterns using it at once is unsupported (one live hazard per boss).
- Material side: the shadow direction (`Behind` radial from the AOE origin, `Lane` parallel to the beam) is the material scalar `SafeSpot_ShadowLane` (1 = Lane), written with the MPC upload; a material that ignores the MPC shows no cut, damage stays right.
- Dedicated server never touches the MPC.
