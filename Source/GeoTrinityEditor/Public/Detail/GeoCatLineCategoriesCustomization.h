// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;
class SWidget;

/**
 * Details customization of FGeoCatLineCategories: a dropdown of checkboxes (several at once, like a bitmask) over the
 * categories of Data/CatLines.txt, re-read each time the dropdown opens so a category just added to the file is offered
 * at once. "All" = the empty set. A stored name the file no longer has stays listed, flagged missing.
 */
class FGeoCatLineCategoriesCustomization : public IPropertyTypeCustomization
{
public:
	/** Returns a new instance of this customization; required by
	 * FPropertyEditorModule::RegisterCustomPropertyTypeLayout. */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** Name on the left, the dropdown button (its text lists the picked categories) on the right. */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructHandle, FDetailWidgetRow& HeaderRow,
								 IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	/** The struct's one array is edited through the dropdown: no children. */
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructHandle, IDetailChildrenBuilder& ChildBuilder,
								   IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	/** The checkbox rows, built from the file as it is now. */
	TSharedRef<SWidget> BuildMenu() const;

	/** The picked names of the first customized object. */
	TArray<FName> GetPickedNames() const;

	/** Replaces the picked names with Names, in one undoable edit. */
	void SetPickedNames(TArray<FName> const& Names) const;

	/** Picks nothing: every category. */
	void PickAll() const;

	/** Adds Name to the picked names when absent, removes it when present. */
	void TogglePicked(FName Name) const;

	/** True when Name is among the picked names. */
	bool IsPicked(FName Name) const;

	/** True when nothing is picked: every category. */
	bool IsAllPicked() const;

	/** The button text: "All", or the picked names. Painted every frame, so it never reads the file. */
	FText GetButtonText() const;

	/** The dropdown's heading: how many lines the picked categories hold (reads the file; shown only while open). */
	FText GetLineCountText() const;

	TSharedPtr<IPropertyHandle> NamesHandle;
};
