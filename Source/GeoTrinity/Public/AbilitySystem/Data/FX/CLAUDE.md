# AbilitySystem/Data/FX

Authored presentation params: what a moment sounds like and looks like. Designers set all knobs in one struct.

| File | Role |
|---|---|
| `GeoSoundRow.h` | `FGeoSoundEntry` (the one sound struct), `FGeoSoundRow` (`DT_GenericSound` row), `UGeoSoundRowLibrary` (the one playback path) |
| `GeoCueParam.h` | `FGeoCueParam`: one authored gameplay cue = tag + palette colour slot |
| `GeoFXMoment.h` | `FGeoVFXParams`, `FGeoBurstVFXParams`, `FGeoBurstFXMoment`, `FGeoSustainedFXMoment`: system + User params |
| `GeoBuffFXDataAsset.h` | `UGeoBuffFXDataAsset`: per buff attribute, character FX and projectile FX while above base |

## Sound
- Whose sound: the instigator's actor owner chain. A player's avatar, or anything a player ability spawns (projectile, deployable, a mine's burst: owner = the PlayerState, the player ASC owner), is that player's: `OtherMachines` on every other machine. A boss and everything it spawns (owner = the boss), a drop, a world actor, null: `InstigatorMachine` everywhere. Spawners keep `Owner` = the payload's `SourceOwner` so the chain holds.
- Play only through `UGeoSoundRowLibrary` (`ShouldPlay`, `GetVolume`, `GetPitch`, `PlaySoundEntry2D`, `PlaySoundEntryAtLocation`, `FindSoundForTag`).
- `PlaySoundEntryAtLocation(World, Entry, Location, Instigator, AbilityLevel)`: fire-and-forget positional one-shot, same gating / volume / pitch as the 2D one. Owned by the world, not by an actor or component: the caller may be destroyed the next frame and the sound still plays to its end. Use it for a moment whose emitter dies with it (pickup, break); a sound that follows an actor or loops uses `SpawnAudioComponent`.
- Entry: `Sound`, `Volume`, `StartTime`, `Audience` (`InstigatorMachine`/`OtherMachines`: see "Whose sound"), `OtherMachinesVolumeMultiplier` (0.5, other players' sounds only), attribute-driven pitch (`PitchAttribute` + `PitchCurve`), `RandomPitchMultiplierRange`, `Concurrency` (null = default, see "Stacking").
- Stacking: identical sounds played together must not sum louder. Every library play passes a `USoundConcurrency`: `Entry.Concurrency`, else for one-shots (`PlaySoundEntry2D`, `PlaySoundEntryAtLocation`, `UGeoFXComponent::PlaySound`) `UGameDataSettings::DefaultOneShotConcurrency`. Loops (`SpawnAudioComponent`, `ConfigureAudioComponent`) only use `Entry.Concurrency` (null = none). Local, per machine, per sound asset (`Limit To Owner` off): it covers many emitters dying on hit, which a per-actor cooldown (`UGeoGameFeelComponent`) cannot.
- Concurrency assets: `/Game/Art/SFX/Concurrency/`. `SCon_OneShot` (default): `RetriggerTime` 0.05 s, `MaxCount` 3, `ResolutionRule` StopQuietest, `VolumeScale` 0.7 (each copy already playing ducks the next). Stricter assets for dense sounds set on their entries: `SCon_GemDrop` (`MaxCount` 2, `RetriggerTime` 0.06 s, `VolumeScale` 0.6, StopQuietest) on `BP_GemDrop` spawn / pickup.
- Reach an entry inline on the owning actor (`SoundMap` of projectile / deployable, shield-burst bounce) — preferred; or by tag through the generic sound cue (`GenericGameplayCueSoundTag`, `AggregatedSourceTags`, `DT_GenericSound` rows).

## Cues
- Every C++-configured cue (deployable moments, pattern init/start, ability fire, effect application) is typed `FGeoCueParam`.
- Cues are cosmetic; they carry data into the notify Blueprint.

## VFX moments
- A new User parameter every system should carry is added once in `FGeoVFXParams`; `UGeoFXComponent::ApplyFXParams` is the only push.
- Param names in `Tool/Visual/GeoNiagaraParams.h`.
