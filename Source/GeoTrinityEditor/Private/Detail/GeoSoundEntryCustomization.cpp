// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Detail/GeoSoundEntryCustomization.h"

#include "AbilitySystem/Data/GeoSoundRow.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"

// ---------------------------------------------------------------------------------------------------------------------
TSharedRef<IPropertyTypeCustomization> FGeoSoundEntryCustomization::MakeInstance()
{
	return MakeShared<FGeoSoundEntryCustomization>();
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoSoundEntryCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructHandle, FDetailWidgetRow& HeaderRow,
												  IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	HeaderRow.NameContent()[StructHandle->CreatePropertyNameWidget()];
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoSoundEntryCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructHandle,
													IDetailChildrenBuilder& ChildBuilder,
													IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	uint32 ChildCount = 0;
	StructHandle->GetNumChildren(ChildCount);
	for (uint32 Index = 0; Index < ChildCount; ++Index)
	{
		TSharedPtr<IPropertyHandle> Child = StructHandle->GetChildHandle(Index);
		if (!Child.IsValid() || !Child->GetProperty())
		{
			continue;
		}

		IDetailPropertyRow& Row = ChildBuilder.AddProperty(Child.ToSharedRef());
		FName const ChildName = Child->GetProperty()->GetFName();
		GateSourceRow(Row, ChildName, StructHandle, GET_MEMBER_NAME_CHECKED(FGeoSoundEntry, VolumeAttribute),
					  GET_MEMBER_NAME_CHECKED(FGeoSoundEntry, bVolumeFromAbilityLevel),
					  GET_MEMBER_NAME_CHECKED(FGeoSoundEntry, VolumeMultiplierCurve));
		GateSourceRow(Row, ChildName, StructHandle, GET_MEMBER_NAME_CHECKED(FGeoSoundEntry, PitchAttribute),
					  GET_MEMBER_NAME_CHECKED(FGeoSoundEntry, bPitchFromAbilityLevel),
					  GET_MEMBER_NAME_CHECKED(FGeoSoundEntry, PitchCurve));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoSoundEntryCustomization::GateSourceRow(IDetailPropertyRow& Row, FName const ChildName,
												TSharedRef<IPropertyHandle> const& StructHandle,
												FName const AttributeName, FName const AbilityLevelName,
												FName const CurveName) const
{
	if (ChildName != AttributeName && ChildName != AbilityLevelName && ChildName != CurveName)
	{
		return;
	}

	TSharedPtr<IPropertyHandle> const AttributeHandle = StructHandle->GetChildHandle(AttributeName);
	TSharedPtr<IPropertyHandle> const AbilityLevelHandle = StructHandle->GetChildHandle(AbilityLevelName);
	if (!ensureMsgf(AttributeHandle.IsValid() && AbilityLevelHandle.IsValid(),
					TEXT("%hs: FGeoSoundEntry has no %s / %s to gate %s on"), __FUNCTION__, *AttributeName.ToString(),
					*AbilityLevelName.ToString(), *ChildName.ToString()))
	{
		return;
	}

	if (ChildName == AttributeName)
	{
		Row.Visibility(TAttribute<EVisibility>::CreateLambda(
			[this, AbilityLevelHandle]()
			{
				return IsFromAbilityLevel(AbilityLevelHandle) ? EVisibility::Collapsed : EVisibility::Visible;
			}));
	}
	else if (ChildName == AbilityLevelName)
	{
		Row.Visibility(TAttribute<EVisibility>::CreateLambda(
			[this, AttributeHandle]()
			{
				return IsAttributeSet(AttributeHandle) ? EVisibility::Collapsed : EVisibility::Visible;
			}));
	}
	else
	{
		Row.Visibility(TAttribute<EVisibility>::CreateLambda(
			[this, AttributeHandle, AbilityLevelHandle]()
			{
				bool const bHasSource = IsAttributeSet(AttributeHandle) || IsFromAbilityLevel(AbilityLevelHandle);
				return bHasSource ? EVisibility::Visible : EVisibility::Collapsed;
			}));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
bool FGeoSoundEntryCustomization::IsAttributeSet(TSharedPtr<IPropertyHandle> const& AttributeHandle) const
{
	TArray<void const*> RawData;
	AttributeHandle->AccessRawData(RawData);
	for (void const* Data : RawData)
	{
		if (Data && static_cast<FGameplayAttribute const*>(Data)->IsValid())
		{
			return true;
		}
	}

	return false;
}

// ---------------------------------------------------------------------------------------------------------------------
bool FGeoSoundEntryCustomization::IsFromAbilityLevel(TSharedPtr<IPropertyHandle> const& AbilityLevelHandle) const
{
	bool bFromAbilityLevel = false;
	AbilityLevelHandle->GetValue(bFromAbilityLevel);
	return bFromAbilityLevel;
}
