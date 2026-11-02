# Tests/Gameplay/Deployable

Deployable tests; spawn through the production funnel (`AI/Testing.md`, "Spawning a deployable in a test").

| File | Covers |
|---|---|
| `GeoDeployableInitTests.cpp` | Init phase: inert while arriving, live once it ends |
| `GeoDeployableStartHealthTests.cpp` | Start health, start health from the ability, self-owned spawn |
| `GeoTurretTests.cpp` | Player turret waits for the owner's basic-hit target, idles when it is gone |
| `GeoGrowingZoneTests.cpp` | Growing zone (leaves with `AI/Pending/RemoveZoneGrowth.md`) |
