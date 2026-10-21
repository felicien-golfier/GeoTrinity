# GemDropSounds — pending

Goal: a gem / shard drop makes a sound when it spawns and when a player picks it up, and the pickup sound plays out in full after the drop is destroyed.
Docs written: `Source/GeoTrinity/Public/Gem/CLAUDE.md` ("Drop sounds"), `Source/GeoTrinity/Public/AbilitySystem/Data/FX/CLAUDE.md` ("Sound"), `Source/GeoTrinity/Public/Actor/CLAUDE.md`, `AI/Python/Audio/CLAUDE.md`.

Done (unbuilt, editor was open): `PlaySoundEntryAtLocation`, `AGeoGemDrop` sounds + `PlayDropSound` seam, `Tests/GeoTestGemDrop.h`, test block in `GeoTrinity.Gems.Drop`, `gem_kit.py` (drafts rendered, preview played). Files clang-formatted.

## Code (to implement)
- [ ] Build (editor closed, `Tools\Build_Editor.bat`, `GEO_NO_PAUSE=1`), then `Tools\Run_Tests.bat GeoTrinity.Gems`.

## Scripts
- [x] `AI/Python/Audio/gem_kit.py ship` — after the user approves the drafts in `AI/Audio/Drafts/Gem/` (`SFX_Gem_Spawn`, `SFX_Gem_Pickup`: one asset each, pitch varied by the drop).
- [x] Add the `Gem` kit rows to `AI/SoundCreation.md` ("Reusable sounds") and a spirit line if the user wants one.
- [x] Import: add `SourceArt/Audio/Gem` → `/Game/Art/SFX/Gem` to the constant in `AI/Python/Asset/import_sound_waves.py`, run it.

## Editor / Blueprint (by hand or script)
- [ ] `/Game/Art/SFX/Gem/` — attenuation on the pickup chime so it carries across the arena; sound class / concurrency so a 50-gem fountain does not stack (max a few voices, resolve by quietest).
- [x] (sounds set, `BP_GeoHexArena.GemDropClass` set; volumes by ear) `/Game/Actors/BP_GemDrop` (child of `AGeoGemDrop`, also in `GemShapes3D.md`) — `SpawnSound.Sound` = `SW_Gem_Spawn`, `PickupSound.Sound` = `SW_Gem_Pickup`, volumes by ear (random pitch is the C++ default, `DropPitchRange`). Arenas' `GemDropClass` = it.

## Verify
- [ ] `GeoTrinity.Gems` tests pass (build first: big enough to need it, editor closed, `Tools\*.bat` `GEO_NO_PAUSE=1`).
- [ ] Listen once to the drafts (played by the agent); in-game check only if the user asks.
