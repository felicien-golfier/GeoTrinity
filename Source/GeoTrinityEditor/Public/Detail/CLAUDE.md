# GeoTrinityEditor / Detail

Details-panel customizations (`IPropertyTypeCustomization`). Registered in `FGeoTrinityEditorModule::StartupModule` (`RegisterCustomPropertyTypeLayout` keyed on the struct's name, unregistered in `ShutdownModule`) — a new customization must be added there or it never runs.

## `ExternalProjectileParamsCustomization.h`
Lays out `FExternalProjectileParams`: emits `ProjectileClass` and each `EOverrideParam` toggle in declaration order, and right under each toggle the values whose `OverrideToggle` metadata names it, visible only while that toggle is on `OverrideValue`. Purely metadata-driven — a new param needs no change here.

Why not `EditCondition`: the engine resolves an EditCondition operand only against the struct that *declares* the conditioned property (`FEditConditionContext` → `FProperty::GetOwnerStruct`), so values inherited from `FProjectileParamsBase` can never see toggles declared in `FExternalProjectileParams`. Naming a member function fails for the same reason (`FindUField<UFunction>` on that struct), and UHT does not accept `UFUNCTION` inside a `USTRUCT` at all. An unresolved EditCondition evaluates to **true** — the property stays visible and editable, with only a `LogEditCondition: Error` line to show for it. Same trap for any future struct that wants a base-class value gated by a derived-class toggle.
