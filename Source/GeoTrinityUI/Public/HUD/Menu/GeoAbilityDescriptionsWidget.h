// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoAbilityDescriptionsWidget.generated.h"

class UGeoMenuButton;
class UScrollBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoAbilityDescriptionsClosedSignature);

/**
 * Full-screen ability compendium for the local player's class: AbilityList is rebuilt on each open with one row
 * per catalogued ability (UAbilityInfo::GetAbilitiesForClass), showing the ability icon at large size next to its
 * display name and token-resolved description (FGameplayAbilityInfo::GetResolvedDescription). Communicates back
 * to the parent menu exclusively via the OnClosed delegate.
 * Required in the BP hierarchy: UGeoMenuButton "BackButton", UScrollBox "AbilityList".
 */
UCLASS()
class GEOTRINITYUI_API UGeoAbilityDescriptionsWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Menu")
	FGeoAbilityDescriptionsClosedSignature OnClosed;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual UWidget* GetInitialFocusWidget() const override;
	virtual bool HandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> BackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UScrollBox> AbilityList;

private:
	UFUNCTION()
	void HandleBack();

	void BuildAbilityList();
};
