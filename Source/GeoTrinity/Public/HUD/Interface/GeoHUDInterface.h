// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"

#include "GeoHUDInterface.generated.h"

class APlayableCharacter;
class AEnemyCharacter;
class APlayerController;
class APlayerState;
class UAbilitySystemComponent;
class UAttributeSet;

UINTERFACE()
class UGeoHUDInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by the UI module's AGeoHUD so gameplay (AGeoPlayerState, AGeoGameState) can drive the HUD via
 * GetHUD<AHUD>() + this interface, without the gameplay module naming the concrete AGeoHUD type.
 */
class IGeoHUDInterface
{
	GENERATED_BODY()

public:
	/** Caches the player GAS references and creates the overlay widget. */
	virtual void InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC,
							 UAttributeSet* AS) = 0;
	/** Binds pawn-dependent HUD callbacks once the pawn and its components exist. */
	virtual void BindToPawn(APlayableCharacter* PlayableCharacter) = 0;
	/** Rebuilds the ability bar from the pawn's current ability set. */
	virtual void BuildAbilityBar(APlayableCharacter* PlayableCharacter) = 0;
	/** Shows the dedicated on-screen boss health bar for Boss. */
	virtual void ShowBossHealthBar(AEnemyCharacter* Boss) = 0;
	/** Hides the dedicated on-screen boss health bar. */
	virtual void HideBossHealthBar() = 0;

	/** Tutorial: shows the Cat's Line (Tutorial Board markup) in place of any line shown. OnEnded is called once when
	 * this line ends, whatever the reason. */
	virtual void ShowCatLine(FText const& Line, FSimpleDelegate OnEnded) = 0;
	/** Tutorial: opens the pause menu on the character sheet, the details of AbilityTag open. */
	virtual void ShowAbilityPage(FGameplayTag AbilityTag) = 0;
	/** Tutorial quest card: Title, a count over Max, a countdown from TimeLimitSeconds, stopping at 0 (0: none). */
	virtual void ShowQuestCard(FText const& Title, int32 Max, float TimeLimitSeconds) = 0;
	virtual void SetQuestCount(int32 Count) = 0;
	virtual void HideQuestCard() = 0;
	/** Opens the pause menu on the Gems menu, on its Forge page (bForge) or its loadout. */
	virtual void OpenGemsPage(bool bForge) = 0;
};
