# BuffFXEnd — pending

Goal: a body-worn buff's VFX ends with a short, clean fade instead of being cut dead (today `FGeoRunningSustainedFX::Stop()` destroys it at once).
Priority: 3
Docs written: none yet (add the rule to `Source/GeoTrinity/Public/Characters/Component/FX/CLAUDE.md` when built).

## Read map
- Source/GeoTrinity/Private/Characters/Component/FX/GeoFXComponent.cpp — `FGeoRunningSustainedFX::Stop()`, `SetSustainedFX`
- Source/GeoTrinity/Public/Characters/Component/FX/GeoFXComponent.h — `MaxLoopFXFadeOutDuration`, `Stop()` doc
- AI/Python/Niagara/geo_aura_vfx.py — `HELD = 100000.0`: held layers never die, so a graceful `Deactivate` never completes (lingers to the 3 s cap, then cuts)
- AI/Python/Niagara/buff_vfx.py — `NS_SwiftWake`, `NS_BulwarkShell` layers
- AI/VFX.md — "Buff VFX" table
- `Config/DefaultGame.ini` — `BuffFX=/Game/Art/Data/DA_BuffFX`, systems used: `NS_ChargedHalo`, `NS_VitalHalo`, `NS_BulwarkShell`, `NS_SwiftWake` (body), `NS_ChargedTrail` (shot)

## Code (to implement)
- [ ] `Stop()` — ramp `User.EndFade` 1 → 0 over ~0.25 s (timer), then `DeactivateImmediate` + destroy; sound stays cut at once. Param name added to `Tool/Visual/GeoNiagaraParams.h`.
- [ ] Alternative without code: Emitter State `Inactive Response` = `Kill` per held emitter (instant, no fade).

## Scripts
- [ ] `geo_aura_vfx.py` / `buff_vfx.py` — held layers multiply alpha and scale by `User.EndFade` (default 1); rebuild `NS_ChargedHalo`, `NS_VitalHalo`, `NS_BulwarkShell`, `NS_SwiftWake`.

## Editor / Blueprint (by hand or script)
- [ ] Confirm in the editor that the held layers still have lifetime 100000 s (stack dump, `AI/MCP/MCP_Niagara.md`).

## Verify
- [ ] No headless test: Niagara spawns return null under `-NullRHI`. Look in the editor: take a buff, let it expire, the VFX fades out fast and is gone.
