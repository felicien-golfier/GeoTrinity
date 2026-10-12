# SlotClassGauge — pending

Goal: the ability / passive tile of each class-gauge ability (sweet-spot, sacrifice detonate, recall turret) charges like the character icon's ring (same fill, rising bottom → top) and shows its value as a number in the middle.
The player card's gauge text follows the same label rule as the tile.
Docs written: `Abilities/Base/CLAUDE.md`, `Abilities/Circle/CLAUDE.md`, `Abilities/Square/CLAUDE.md`, `Abilities/Triangle/CLAUDE.md`, `GeoTrinityUI/.../HUD/CLAUDE.md`, `HUD/AbilityBar/CLAUDE.md`, `HUD/Team/CLAUDE.md`, `HUD/Style/Meter/CLAUDE.md`, `AI/Python/UI/CLAUDE.md`.

## Code (to implement)
- [x] `Abilities/Base/GeoGameplayAbility.h/.cpp` — `virtual bool GetGaugeLabel(UAbilitySystemComponent const& ASC, FText& OutLabel) const`, default false
- [x] `Abilities/Circle/GeoSweetSpotChargePassiveAbility` — label = whole % of `GetGaugeRatio`
- [x] `Abilities/Square/GeoSacrificeDetonateAbility` — label = rounded `SacrificeValue` when > 0
- [x] `Abilities/Triangle/GeoRecallTurretAbility` — label = blinking-turret count when `IsBlinkRecallReady`, else false
- [x] `GeoTrinityUI/HUD/Style/Meter/GeoMeterStyle.h` — `EGeoMeterShape::Rise`; `SGeoMeter` draws it
- [x] `GeoTrinityUI/HUD/GeoHUD.h/.cpp` — `GetAbilityGauge(Tag, Fill, bReady, Label)`
- [x] `GeoTrinityUI/HUD/AbilityBar/GeoAbilitySlotWidget.h/.cpp` — `GaugeMeter`, `GaugeText`, class tint, per-tick refresh
- [x] `GeoTrinityUI/HUD/Team/GeoPlayerCardWidget.h/.cpp` — label through `GaugeFormat` (`"·  {0}"`), text collapsed without a label
- [ ] `WBP_PlayerWing` / `WBP_TeamRow`: check no Blueprint override of `GaugeFormat` still says `CHARGE {0}%` (reset to default if so)

## Scripts
- [x] `AI/Python/UI/wings_hud.py` — `METER_STYLES["DA_Meter_SlotGauge"]`
- [x] `AI/Python/UI/slot_gauge.py` (new) — written; it also runs `wings_hud.build_meter_styles()` so the style exists
- [ ] Run `slot_gauge.py` headless once built (after `passive_bar.py`); check `AI/Output/slot_gauge.txt`

## Tests
- [x] `Source/GeoTrinity/Private/Tests/GeoAbilityReadyTests.cpp` — label cases added (test actor now carries a `UGeoDeployableManagerComponent` for the recall)
- [ ] Not automated: slot / meter drawing (needs a viewport) — PIE look only if asked
- [ ] Build: `set GEO_NO_PAUSE=1 && Tools\Build_Editor.bat` (editor closed)
- [ ] Run: `set GEO_NO_PAUSE=1 && Tools\Run_Tests.bat GeoTrinity.AbilityReady`

## Verify
- [ ] Tests green (parsed report); in-game look only if the user asks (PIE)
