// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Detail/ExternalProjectileParamsCustomization.h"

#include "Actor/Projectile/ExternalProjectileParams.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"

namespace
{
	FName const OverrideToggleMetaKey(TEXT("OverrideToggle"));

	/** Collapsed unless the toggle reads back a single OverrideValue — a multi-selection with differing toggles has no
	 * one value to edit. */
	EVisibility GetValueVisibility(TSharedPtr<IPropertyHandle> const& ToggleHandle)
	{
		uint8 ToggleValue = 0;
		if (ToggleHandle->GetValue(ToggleValue) != FPropertyAccess::Success
			|| static_cast<EOverrideParam>(ToggleValue) != EOverrideParam::OverrideValue)
		{
			return EVisibility::Collapsed;
		}

		return EVisibility::Visible;
	}
} // namespace

// ---------------------------------------------------------------------------------------------------------------------
TSharedRef<IPropertyTypeCustomization> FExternalProjectileParamsCustomization::MakeInstance()
{
	return MakeShared<FExternalProjectileParamsCustomization>();
}

// ---------------------------------------------------------------------------------------------------------------------
void FExternalProjectileParamsCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructHandle,
															 FDetailWidgetRow& HeaderRow,
															 IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	HeaderRow.NameContent()[StructHandle->CreatePropertyNameWidget()];
}

// ---------------------------------------------------------------------------------------------------------------------
void FExternalProjectileParamsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructHandle,
															   IDetailChildrenBuilder& ChildBuilder,
															   IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	uint32 ChildCount = 0;
	StructHandle->GetNumChildren(ChildCount);

	TArray<TSharedPtr<IPropertyHandle>> Children;
	TSet<FName> ChildNames;
	Children.Reserve(static_cast<int32>(ChildCount));
	for (uint32 Index = 0; Index < ChildCount; ++Index)
	{
		TSharedPtr<IPropertyHandle> Child = StructHandle->GetChildHandle(Index);
		if (!Child.IsValid() || !Child->GetProperty())
		{
			continue;
		}

		Children.Add(Child);
		ChildNames.Add(Child->GetProperty()->GetFName());
	}

	TMap<FName, TArray<TSharedPtr<IPropertyHandle>>> ValuesByToggleName;
	TSet<FName> GatedValueNames;
	for (TSharedPtr<IPropertyHandle> const& Child : Children)
	{
		FString const ToggleName = Child->GetMetaData(OverrideToggleMetaKey);
		if (!ToggleName.IsEmpty()
			&& ensureMsgf(ChildNames.Contains(FName(*ToggleName)),
						  TEXT("%s declares OverrideToggle=\"%s\", which is not a property of %s"),
						  *Child->GetProperty()->GetName(), *ToggleName, *StructHandle->GetProperty()->GetName()))
		{
			ValuesByToggleName.FindOrAdd(FName(*ToggleName)).Add(Child);
			GatedValueNames.Add(Child->GetProperty()->GetFName());
		}
	}

	for (TSharedPtr<IPropertyHandle> const& Child : Children)
	{
		if (GatedValueNames.Contains(Child->GetProperty()->GetFName()))
		{
			continue; // Emitted under its toggle below.
		}

		ChildBuilder.AddProperty(Child.ToSharedRef());

		TArray<TSharedPtr<IPropertyHandle>> const* GatedValues =
			ValuesByToggleName.Find(Child->GetProperty()->GetFName());
		if (!GatedValues)
		{
			continue;
		}

		for (TSharedPtr<IPropertyHandle> const& Value : *GatedValues)
		{
			ChildBuilder.AddProperty(Value.ToSharedRef())
				.Visibility(TAttribute<EVisibility>::CreateLambda([Child]() { return GetValueVisibility(Child); }));
		}
	}
}
