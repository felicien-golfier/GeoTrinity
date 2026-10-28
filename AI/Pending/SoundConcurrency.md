# SoundConcurrency — pending

Goal: several copies of the same sound playing together (bullet impacts, gem fountain) must not get louder than one.
Priority: 2
Docs written: `Source/GeoTrinity/Public/AbilitySystem/Data/FX/CLAUDE.md` ("Sound": "Stacking", "Concurrency assets", Entry list).

## Code (to implement)
- [x] `AbilitySystem/Data/FX/GeoSoundRow.h` — `FGeoSoundEntry::Concurrency` (`TObjectPtr<USoundConcurrency>`, EditAnywhere, null = default); library helper `GetConcurrency(Entry, bOneShot)` → entry's, else `DefaultOneShotConcurrency` for one-shots, else null.
- [x] `AbilitySystem/Data/FX/GeoSoundRow.cpp` — pass it as `ConcurrencySettings` to `PlaySound2D`, `PlaySoundAtLocation` (one-shot) and `SpawnSoundAttached` (loop); `ConfigureAudioComponent` sets `AudioComponent->ConcurrencySet` from the entry only.
- [x] `Characters/Component/FX/GeoFXComponent.cpp` `PlaySound` — same one-shot concurrency (or route through `PlaySoundEntryAtLocation`).
- [x] `Settings/GameDataSettings.h` — `TSoftObjectPtr<USoundConcurrency> DefaultOneShotConcurrency` (Config, category GeoSound); read through `GetLoadedDataAsset` (rooted after the first load).
- [x] `Config/DefaultGame.ini` — `DefaultOneShotConcurrency=/Game/Art/SFX/Concurrency/SCon_OneShot.SCon_OneShot`.
- [x] Grep other direct `UGameplayStatics::PlaySound*` / `SpawnSound*` in `Source/` (rule: library only) and route them through.

## Tests (per `AI/CodingStyle.md` "Tests")
- [x] `GeoTrinity.Sound.Concurrency` — `GetConcurrency`: entry's wins; null entry + one-shot → default; null entry + loop → null.

## Editor / Blueprint (by hand or script)
- [x] Create `/Game/Art/SFX/Concurrency/SCon_OneShot` — `bLimitToOwner` false, `MaxCount` 3, `ResolutionRule` StopQuietest, `RetriggerTime` 0.05, `VolumeScale` 0.7.
- [ ] If bullet impacts still too loud: `SCon_BulletImpact` (`MaxCount` 2, `RetriggerTime` 0.06, `VolumeScale` 0.6), set on the projectile `SoundMap` impact entries.
- [x] Gem fountain: `SCon_GemDrop` on `BP_GemDrop` `SpawnSound` / `PickupSound` (closes `GemDropSounds.md` concurrency row).

## Verify
- [x] Built (DebugGame editor, 2026-10-27): `GeoTrinity.Sound.` green; the whole `GeoTrinity.Tutorial.` set green (31/31, FullRun included)
- [ ] By ear in game only if the user asks.
