# Actor/Deployable/Wall

Square's deployable wall.

## Files
| File | Role |
|---|---|
| `GeoWall.h/.cpp` | Damageable wall — draining health pool, consumed by Detonate to boost its ray |

## Key Points
- `bUseRegularDrain = true` — health pool drains over time like a turret (rate from `Params.LifeDrainMaxDuration`); can also be damaged by enemies. Expires when health hits zero.
- `bExplodeAtRecall = false` — the wall **never explodes**. `Recall()` (Detonate, class switch, natural death) is silent.
- **No overlap trigger** — stepping on it does nothing.
- **Gameplay collision is on `MeshComponent`, not the capsule** — the root `CapsuleComponent` collision is disabled in the constructor (`NoCollision`); `MeshComponent` (a `UStaticMeshComponent`, mesh asset assigned on `BP_Wall`) carries the `"GeoShape"` profile. So projectiles and AoE queries hit the wall's mesh shape instead of the capsule outline. Overlaps pawns, does not block or push characters; still detectable by enemy projectiles so it can take damage. Does **not** set `MovementBlocker` or `bPushActorsOnSpawn`.
- Deployed by the shared `UGeoDeployAbility` (configured in BP with `DeployableActorClass = BP_Wall`). No Square-specific deploy ability.
- Consumed by `UGeoDetonateWallsAbility`: each wall on the detonate ray is recalled and multiplies the ray's damage/shield output.
- Minimal class like `AGeoPillar`/`AGeoTurret`: the `.cpp` holds the concrete `GetData()` override, the replicated `WallData` (`COND_InitialOnly`), and the `MeshComponent` collision wiring. The mesh **asset** and capsule size are Blueprint concerns (set on `BP_Wall`); the `MeshComponent` itself and its `GeoShape` profile are created in the C++ constructor.
