# System/Pooling

| File | Role |
|---|---|
| `GeoActorPoolingSubsystem.h` | World subsystem: `RequestActor<T>`, `ReleaseActor`, `PreSpawn<T>`, `Get(World)` |
| `GeoPoolableInterface.h` | `Init()` on acquisition, `End()` on return |

- `Pool` holds only idle actors: `PreSpawn` inserts what it spawns; `RequestActor` on an empty pool hands the fresh actor straight to the caller.
- `ReleaseActor` bails when the actor is already pooled; callers keep their own end-of-life guard (`bIsEnding`).
- `Pool` is not a `UPROPERTY` (nested array in a map); `ULevel::Actors` keeps pooled actors alive, so never `Destroy()` a pooled actor.
