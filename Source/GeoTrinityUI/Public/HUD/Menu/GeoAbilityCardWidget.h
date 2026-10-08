// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/AbilityInfo.h"
#include "CoreMinimal.h"
#include "HUD/GeoUserWidget.h"

#include "GeoAbilityCardWidget.generated.h"

class APlayableCharacter;
class UGeoFrame;
class UGeoIconImage;
class URichTextBlock;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGeoAbilityCardSelectedSignature, UGeoAbilityCardWidget*, Card);

/**
 * One ability on the abilities page: its icon on a tile glowing in the class colour (passives in PassiveColor), its
 * name, its key or a passive tag, its cooldown or fire delay, and its token-resolved description. The Reload's buffs
 * get one line each under it, with the buff's colour swatch. Layout, fonts and colours live in WBP_AbilityCard; the
 * rich texts take their styles from the data table set on them (DT_AbilityText: Default and Value rows).
 * A click, or Accept while focused, fires OnSelected.
 */
UCLASS()
class GEOTRINITYUI_API UGeoAbilityCardWidget : public UGeoUserWidget
{
	GENERATED_BODY()

public:
	/** Fills the card for Info at AbilityLevel; ClassColor tints the icon tile's glow of an active ability. */
	virtual void SetAbility(FPlayersGameplayAbilityInfo const& Info, int32 AbilityLevel, FLinearColor const& ClassColor);

	/** Shows the ability Source shows. */
	void CopyAbility(UGeoAbilityCardWidget const& Source);

	/** Lights CardFrame as the card whose details are open. */
	void SetSelected(bool bSelected);

	UPROPERTY(BlueprintAssignable, Category = "GeoAbilityCard")
	FGeoAbilityCardSelectedSignature OnSelected;

	/** One CardClass card, owned by Owner, per catalogued ability of PlayableCharacter's class, actives first, each at
	 * the level it is granted at. */
	static TArray<UGeoAbilityCardWidget*> CreateClassCards(UUserWidget& Owner,
															TSubclassOf<UGeoAbilityCardWidget> CardClass,
															APlayableCharacter const& PlayableCharacter);

protected:
	/** Takes the mouse on a left press, so only a click that started on this card selects it. */
	virtual FReply NativeOnMouseButtonDown(FGeometry const& InGeometry, FPointerEvent const& InMouseEvent) override;
	/** Fires OnSelected when a left press made on the card is released over it. */
	virtual FReply NativeOnMouseButtonUp(FGeometry const& InGeometry, FPointerEvent const& InMouseEvent) override;
	/** Fires OnSelected on Accept (Enter, gamepad bottom face button). */
	virtual FReply NativeOnKeyDown(FGeometry const& InGeometry, FKeyEvent const& InKeyEvent) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoFrame> CardFrame;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoFrame> IconFrame;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoIconImage> Icon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	/** The ability's slot name, one of the labels below. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> KeyText;

	/** Cooldown or fire delay, "COOLDOWN <Value>3s</>". */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<URichTextBlock> TimingText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<URichTextBlock> DescriptionText;

	/** Takes one line per Reload buff, each in DescriptionText's styles. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> BuffBox;

	/** Glow of a passive's icon tile, and the colour of its passive tag. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoAbilityCard")
	FLinearColor PassiveColor = FLinearColor(0.2f, 0.5f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoAbilityCard")
	FText PassiveLabel = INVTEXT("PASSIVE");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoAbilityCard")
	FText BasicLabel = INVTEXT("BASIC ATTACK");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoAbilityCard")
	FText DeployableLabel = INVTEXT("DEPLOYABLE");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoAbilityCard")
	FText SpecialLabel = INVTEXT("SPECIAL");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoAbilityCard")
	FText ReloadLabel = INVTEXT("RELOAD");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoAbilityCard")
	FText DashLabel = INVTEXT("DASH");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoAbilityCard")
	FText CooldownLabel = INVTEXT("COOLDOWN");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoAbilityCard")
	FText DelayLabel = INVTEXT("DELAY");

	/** Side of the colour square before each Reload buff line. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoAbilityCard", meta = (ClampMin = "2", ClampMax = "64"))
	float BuffSwatchSize = 14.f;

	/** What SetAbility last showed. */
	FPlayersGameplayAbilityInfo Ability;
	int32 Level = 1;
	FLinearColor Color = FLinearColor::White;

private:
	/** The label of Info's slot, from its ability type tag. */
	FText GetSlotLabel(FPlayersGameplayAbilityInfo const& Info) const;
};
