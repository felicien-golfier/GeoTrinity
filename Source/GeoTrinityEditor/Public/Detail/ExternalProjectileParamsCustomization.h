// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

/**
 * Details customization for FExternalProjectileParams: shows each value right under the EOverrideParam toggle named by
 * its "OverrideToggle" metadata, and hides it while that toggle is not on OverrideValue.
 *
 * This cannot be a plain EditCondition: the engine resolves an EditCondition operand only against the struct that
 * declares the conditioned property (FEditConditionContext uses FProperty::GetOwnerStruct), so the values inherited
 * from FProjectileParamsBase can never see the toggles declared in FExternalProjectileParams. An EditCondition naming
 * a member function does not work either — the lookup is FindUField<UFunction> on that same owner struct, and a
 * USTRUCT has no UFunctions at all (UHT only accepts UFUNCTION inside a UCLASS/UINTERFACE). An unresolved
 * EditCondition silently evaluates to true, so the property stays visible and editable.
 */
class FExternalProjectileParamsCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** Default name + value header (the struct's own row is untouched). */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructHandle, FDetailWidgetRow& HeaderRow,
								 IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	/** Emits ProjectileClass and each toggle in declaration order, each toggle followed by the values it gates. */
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructHandle, IDetailChildrenBuilder& ChildBuilder,
								   IPropertyTypeCustomizationUtils& CustomizationUtils) override;
};
