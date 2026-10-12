# AbilitySystem/Data/FX

Authored presentation params: what a moment sounds like and looks like. Designers set all knobs in one struct.

| File | Role |
|---|---|
| `GeoSoundRow.h` | `FGeoSoundEntry` (the one sound struct), `FGeoSoundRow` (`DT_GenericSound` row), `UGeoSoundRowLibrary` (the one playback path) |
| `GeoCueParam.h` | `FGeoCueParam`: one authored gameplay cue = tag + palette colour slot |
| `GeoFXMoment.h` | `FGeoVFXParams`, `FGeoBurstVFXParams`, `FGeoBurstFXMoment`, `FGeoSustainedFXMoment`: system + User params |
| `GeoBuffFXDataAsset.h` | `UGeoBuffFXDataAsset`: per buff attribute, character FX and projectile FX while above base |

## Sound
- Play only through `UGeoSoundRowLibrary` (`ShouldPlay`, `GetVolume`, `GetPitch`, `PlaySoundEntry2D`, `FindSoundForTag`).
- Entry: `Sound`, `Volume`, `StartTime`, `Audience` (`InstigatorMachine`/`OtherMachines`), `OtherMachinesVolumeMultiplier` (0.5), attribute-driven pitch (`PitchAttribute` + `PitchCurve`), `RandomPitchMultiplierRange`.
- Reach an entry inline on the owning actor (`SoundMap` of projectile / deployable, shield-burst bounce) — preferred; or by tag through the generic sound cue (`GenericGameplayCueSoundTag`, `AggregatedSourceTags`, `DT_GenericSound` rows).

## Cues
- Every C++-configured cue (deployable moments, pattern init/start, ability fire, effect application) is typed `FGeoCueParam`.
- Cues are cosmetic; they carry data into the notify Blueprint.

## VFX moments
- A new User parameter every system should carry is added once in `FGeoVFXParams`; `UGeoFXComponent::ApplyFXParams` is the only push.
- Param names in `Tool/Visual/GeoNiagaraParams.h`.
