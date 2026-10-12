# Actor/Deployable/Wall

`GeoWall.h` — Square wall; deployed by the shared `UGeoDeployAbility` (`DeployableActorClass = BP_Wall`), consumed by `UGeoDetonateWallsAbility`.

- `bUseRegularDrain`; expires at zero; never explodes (`bExplodeAtRecall = false`).
- Gameplay collision on `MeshComponent` (`GeoShape` profile, created in C++); root capsule `NoCollision`; overlaps pawns, no push.
- Mesh asset and capsule size are BP concerns; `WallData` `COND_InitialOnly`.
