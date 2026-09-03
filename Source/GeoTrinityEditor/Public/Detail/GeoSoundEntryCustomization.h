// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class IDetailPropertyRow;
class IPropertyHandle;

/**
 * Details customization for FGeoSoundEntry: gates each curve's two source rows against each other.
 * Volume and pitch each read from an attribute or from the ability level. The two are mutually exclusive, so each
 * hides while the other is set, and the curve stays hidden until one of them is — it has nothing to sample against
 * otherwise.
 *
 * The half that gates on the ability-level bool could be a plain EditCondition, but the half that gates on the
 * attribute cannot: FEditConditionContext resolves an operand only to a bool, enum, numeric or object property, and
 * FGameplayAttribute is a struct. An unresolved EditCondition silently evaluates to true, so the row would stay
 * visible with only a LogEditCondition error to show for it. Both halves live here so one mechanism drives every row.
 */
class FGeoSoundEntryCustomization : public IPropertyTypeCustomization
{
public:
	/** Returns a new instance of this customization; required by
	 * FPropertyEditorModule::RegisterCustomPropertyTypeLayout. */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** Default name + value header (the struct's own row is untouched). */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructHandle, FDetailWidgetRow& HeaderRow,
								 IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	/** Emits every child in declaration order, gating the volume and the pitch source rows. */
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructHandle, IDetailChildrenBuilder& ChildBuilder,
								   IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	/** Attaches one source trio's mutual-exclusion visibility to Row when ChildName is one of its three properties.
	 * Does nothing for any other property. */
	void GateSourceRow(IDetailPropertyRow& Row, FName ChildName, TSharedRef<IPropertyHandle> const& StructHandle,
					   FName AttributeName, FName AbilityLevelName, FName CurveName) const;

	/** True when the handled attribute names a real attribute on any object of the current selection. */
	bool IsAttributeSet(TSharedPtr<IPropertyHandle> const& AttributeHandle) const;

	/** True when the handled ability-level bool is checked; false for a multi-selection that disagrees. */
	bool IsFromAbilityLevel(TSharedPtr<IPropertyHandle> const& AbilityLevelHandle) const;
};
