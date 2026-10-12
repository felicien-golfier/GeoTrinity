# Animation

Anim notifies authored on montages so the beat is placed against the animation; montages already replicate through the ASC, so no RPC.

| File | Role |
|---|---|
| `GeoCameraShakeNotify.h` | Starts `ShakeClass` on the local player camera; no-op on dedicated server |
| `GeoDeathDebrisNotify.h` | Niagara notify; hands the spawned debris to the owning character |
| `GeoLoopSoundNotify.h` | Starts / stops the owner's `UGeoLoopSoundComponent` loops unfaded |
