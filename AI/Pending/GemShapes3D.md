# GemShapes3D — pending

Goal: gems exist in 3D as the perfect solid of their 2D tier polygon (regular glowing prisms); a boss kill throws the granted gems out in a fountain that lies on the ground until a character steps on it. Lit, tumbling in 3D, bouncing, an eruption of a fountain.
Priority: 2
Docs written: `Source/GeoTrinity/Public/Gem/CLAUDE.md` ("3D gems", "Drop feel", "Fountain feel"), `Source/GeoTrinity/Public/Actor/CLAUDE.md`, `Source/GeoTrinity/Public/Actor/Arena/CLAUDE.md`, `AI/Python/Mesh/CLAUDE.md`.

Done: meshes, `AGeoGemDrop` (tumble, pop-in, bounces + `Flash`, rocking rest), arena fountain (front-loaded, seeded, up-speed spread), `M_Gem` self-lit key light + glint + `Flash` (script run, compiles without errors), tests (`GeoTrinity.Gems` 21/21, built).

## Left
- [ ] Look at it once in the editor (shader compiled by eye? never seen lit): kill a boss, tune `Glow` / `RimBoost` / `FlashBoost` on `M_Gem` instances, `HaloGlow` / `HaloFalloff` on `M_GemHalo`, `KEY_LIGHT_DIRECTION` / `GLINT_*` in `make_gem_materials.py`, and the feel numbers on the drop (`TumbleSpeed`, `TumbleFriction`, `GroundFriction`, `WallRestitution`, `BounceRestitution`; walls are found as world-static objects: check the barrier's moving walls and the pillars hold a gem in too) and arena (`FountainFrontLoad`, `FountainUpSpread`).
- [ ] Tier solids (octahedron / dodecahedron / icosahedron / diamond, size by tier) regenerated and materials re-slotted: check in that same look that `RestHeight` (20, the centre's Z) suits the sizes (Core half-height ~26, Chip ~25; the halo sphere, 1.7× radius, is additive and may meet the floor) and tune `RADIUS` in `generate_gem_meshes.py` if the size steps read too weak / strong; shards halved 2026-10-28 (`SHARD_SCALE` 0.5, ~15–36 tall, one drop per shard).
- [ ] Optional: `/Game/Actors/BP_GemDrop` (child of `AGeoGemDrop`), then set it as `GemDropClass` on the arenas.
