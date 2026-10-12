# Tool/Visual

| File | Role |
|---|---|
| `GeoColor.h` | `EGeoColor` semantic slots + `FGeoColorParam` |
| `GeoIcon.h` | `UGeoIcon`: vector icon as polylines (stroked/filled), drawn by `GeoIconImage` / `SGeoIconImage` |
| `GeoNiagaraParams.h` | Every Niagara `User.*` parameter name the C++ side writes + swap helper |

## GeoColor
- Slots name what the effect does, not when: `Damage` (incoming, enemy-owned), `AllyDamage`, `LethalDamage`, `Heal`, `Shield`, `BothHealAndDamage`, `DamageReduction`, `DamageBoost`, `HealBoost`, `MoveSpeed`, `Override` (last).
- No telegraph slot: a wind-up zone wears the colour of the effect it announces.
- Every authored colour property is `FGeoColorParam` (slot + `OverrideColor` shown only on `Override`); read with `GetColor()`.
- Resolved through `GameDataSettings::ColorPalette`; a missing slot `ensureMsgf`s and falls back to white.
- Side split is authored at the spawn site: ally abilities set head/trail `AllyDamage` with the override toggles on `OverrideValue`; only valid on non-replicated pooled projectiles — replicated ones carry colour on the Blueprint.
- Materials read the palette as a 1 × `SlotCount` texture (`GeoColor::CreatePaletteTexture`, set on `AGeoGameCamera::OutlineMaterial` with `PaletteSize`), so a new slot needs no asset work. `Override` stays last (ordinal = stencil index).
- Runtime resolved colours (`UGeoBeamVFXComponent::BeamColor`, HUD) stay `FLinearColor`.

## GeoNiagaraParams
- One `inline FName const` per `User.*` param (bullet, beam, wave AOE). Never type a name at a call site; Material / MPC names stay named constants local to their owner.
- `FBeamVfxAssetSet` + `ApplySwappableAsset(Component, Assets, bWantPreview)`: reassigns only when the asset differs (used by `UGeoBeamVFXComponent`, `UBeamPattern`).
