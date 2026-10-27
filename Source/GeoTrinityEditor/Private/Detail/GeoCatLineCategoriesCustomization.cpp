// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Detail/GeoCatLineCategoriesCustomization.h"

#include "DetailWidgetRow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Tool/CatLine/GeoCatLineBank.h"
#include "Tool/CatLine/GeoCatLineCategories.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

// ---------------------------------------------------------------------------------------------------------------------
TSharedRef<IPropertyTypeCustomization> FGeoCatLineCategoriesCustomization::MakeInstance()
{
	return MakeShared<FGeoCatLineCategoriesCustomization>();
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoCatLineCategoriesCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructHandle,
														 FDetailWidgetRow& HeaderRow,
														 IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	NamesHandle = StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGeoCatLineCategories, Names));

	// clang-format off
	HeaderRow
		.NameContent()
		[
			StructHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.f)
		[
			SNew(SComboButton)
			.OnGetMenuContent(this, &FGeoCatLineCategoriesCustomization::BuildMenu)
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &FGeoCatLineCategoriesCustomization::GetButtonText)
			]
		];
	// clang-format on
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoCatLineCategoriesCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> /*StructHandle*/,
														   IDetailChildrenBuilder& /*ChildBuilder*/,
														   IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
}

// ---------------------------------------------------------------------------------------------------------------------
TSharedRef<SWidget> FGeoCatLineCategoriesCustomization::BuildMenu() const
{
	FMenuBuilder Menu(false, nullptr);
	TArray<FName> const Picked = GetPickedNames();
	TArray<FName> const InFile = GeoCatLines::GetCategories();

	Menu.BeginSection(NAME_None,
					  TAttribute<FText>::CreateSP(this, &FGeoCatLineCategoriesCustomization::GetLineCountText));
	Menu.AddMenuEntry(INVTEXT("All"), INVTEXT("No category picked: every category of the file"), FSlateIcon(),
					  FUIAction(FExecuteAction::CreateSP(this, &FGeoCatLineCategoriesCustomization::PickAll),
								FCanExecuteAction(),
								FIsActionChecked::CreateSP(this, &FGeoCatLineCategoriesCustomization::IsAllPicked)),
					  NAME_None, EUserInterfaceActionType::Check);
	Menu.AddSeparator();

	TArray<FName> Listed = InFile;
	for (FName const Name : Picked)
	{
		Listed.AddUnique(Name);
	}

	for (FName const Name : Listed)
	{
		FText const Label = InFile.Contains(Name)
			? FText::FromName(Name)
			: FText::Format(INVTEXT("{0} (missing from the file)"), FText::FromName(Name));
		Menu.AddMenuEntry(
			Label, FText::GetEmpty(), FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &FGeoCatLineCategoriesCustomization::TogglePicked, Name),
					  FCanExecuteAction(),
					  FIsActionChecked::CreateSP(this, &FGeoCatLineCategoriesCustomization::IsPicked, Name)),
			NAME_None, EUserInterfaceActionType::Check);
	}

	Menu.EndSection();
	return Menu.MakeWidget();
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FName> FGeoCatLineCategoriesCustomization::GetPickedNames() const
{
	TArray<FName> Names;
	TSharedPtr<IPropertyHandleArray> const Array = NamesHandle->AsArray();
	uint32 Count = 0;
	Array->GetNumElements(Count);
	for (uint32 Index = 0; Index < Count; ++Index)
	{
		FName Name;
		Array->GetElement(Index)->GetValue(Name);
		Names.Add(Name);
	}
	return Names;
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoCatLineCategoriesCustomization::SetPickedNames(TArray<FName> const& Names) const
{
	FScopedTransaction const Transaction(INVTEXT("Pick Cat line categories"));
	TSharedPtr<IPropertyHandleArray> const Array = NamesHandle->AsArray();
	Array->EmptyArray();
	for (int32 Index = 0; Index < Names.Num(); ++Index)
	{
		Array->AddItem();
		Array->GetElement(Index)->SetValue(Names[Index]);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoCatLineCategoriesCustomization::PickAll() const
{
	SetPickedNames({});
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoCatLineCategoriesCustomization::TogglePicked(FName const Name) const
{
	TArray<FName> Names = GetPickedNames();
	if (Names.Remove(Name) == 0)
	{
		Names.Add(Name);
	}
	SetPickedNames(Names);
}

// ---------------------------------------------------------------------------------------------------------------------
bool FGeoCatLineCategoriesCustomization::IsPicked(FName const Name) const
{
	return GetPickedNames().Contains(Name);
}

// ---------------------------------------------------------------------------------------------------------------------
bool FGeoCatLineCategoriesCustomization::IsAllPicked() const
{
	return GetPickedNames().IsEmpty();
}

// ---------------------------------------------------------------------------------------------------------------------
FText FGeoCatLineCategoriesCustomization::GetButtonText() const
{
	TArray<FString> Labels;
	for (FName const Name : GetPickedNames())
	{
		Labels.Add(Name.ToString());
	}

	return Labels.IsEmpty() ? INVTEXT("All") : FText::FromString(FString::Join(Labels, TEXT(", ")));
}

// ---------------------------------------------------------------------------------------------------------------------
FText FGeoCatLineCategoriesCustomization::GetLineCountText() const
{
	FGeoCatLineCategories Picked;
	Picked.Names = GetPickedNames();
	return FText::Format(INVTEXT("{0} lines picked"), GeoCatLines::CountLines(Picked));
}
