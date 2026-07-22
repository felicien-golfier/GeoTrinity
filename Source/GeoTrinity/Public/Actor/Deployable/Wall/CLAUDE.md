# Actor/Deployable/Wall

Square's deployable wall.

## Files
| File | Role |
|---|---|
| `GeoWall.h/.cpp` | Damageable wall — draining health pool, consumed by Detonate to boost its ray |

## Key Points
- `bUseRegularDrain = true` — health drains over time and can be damaged by enemies; expires at zero. Never explodes (`bExplodeAtRecall = false`) — Recall is always silent.
- No overlap trigger; stepping on it does nothing.
- **Gameplay collision is on `MeshComponent`, not the capsule** — root `CapsuleComponent` is `NoCollision`; `MeshComponent` carries the `"GeoShape"` profile, so projectiles/AoE hit the mesh shape, not the capsule outline. Overlaps pawns (doesn't block/push). No `bPushActorsOnSpawn`.
- Deployed by the shared `UGeoDeployAbility` (BP-configured `DeployableActorClass = BP_Wall`); no Square-specific deploy ability.
- Consumed by `UGeoDetonateWallsAbility`: walls on the ray are recalled, multiplying the ray's output.
- `WallData` replicated `COND_InitialOnly`, like `AGeoPillar`/`AGeoTurret`. Mesh asset and capsule size are BP concerns; `MeshComponent` + its `GeoShape` profile are created in C++.
