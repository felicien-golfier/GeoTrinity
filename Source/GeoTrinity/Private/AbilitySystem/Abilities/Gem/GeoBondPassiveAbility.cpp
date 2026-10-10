// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Gem/GeoBondPassiveAbility.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Types/GeoAscTypes.h"
#include "Characters/PlayableCharacter.h"
#include "GameClasses/GeoGameState.h"
#include "Tool/UGeoGameplayLibrary.h"

void UGeoBondPassiveAbility::BindEvent(UGeoAbilitySystemComponent& /*HolderASC*/)
{
	AGeoGameState* GameState = GetWorld()->GetGameState<AGeoGameState>();
	if (GeoLib::IsServer(GetWorld()) && ensureMsgf(GameState, TEXT("%hs: no AGeoGameState"), __FUNCTION__))
	{
		GameState->OnIncomingDamage.AddUObject(this, &ThisClass::ShareIncomingDamage);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBondPassiveAbility::UnbindEvent(UGeoAbilitySystemComponent& /*HolderASC*/)
{
	if (AGeoGameState* GameState = GetWorld()->GetGameState<AGeoGameState>())
	{
		GameState->OnIncomingDamage.RemoveAll(this);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBondPassiveAbility::ShareIncomingDamage(UAbilitySystemComponent& VictimASC,
												 FGameplayEffectContextHandle const& DamageContext, float const Damage,
												 float& SparedDamage)
{
	FGeoGameplayEffectContext const* GeoDamageContext =
		static_cast<FGeoGameplayEffectContext const*>(DamageContext.Get());
	APlayableCharacter const* Victim = Cast<APlayableCharacter>(VictimASC.GetAvatarActor());
	UGeoAbilitySystemComponent* HolderASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (Victim && &VictimASC != HolderASC && !Victim->IsDead() && !Victim->IsInvulnerable()
		&& !(GeoDamageContext && GeoDamageContext->DoNotRedirectSacrifice()))
	{
		bool const bVictimReduced = !(GeoDamageContext && GeoDamageContext->ShouldSkipStatModifiers());
		float const VictimReduction = bVictimReduced
			? FMath::Clamp(GeoASLib::GetStatValue(&VictimASC, UCharacterAttributeSet::GetDamageReductionAttribute(), 0.f),
						   0.f, 0.99f)
			: 0.f;
		float const HolderReduction = FMath::Clamp(
			GeoASLib::GetStatValue(HolderASC, UCharacterAttributeSet::GetDamageReductionAttribute(), 0.f), 0.f, 1.f);
		float const Share = Damage * SharedDamageFraction;

		FDamageEffectData ShareEffect;
		ShareEffect.Amount = FScalableFloat(Share / (1.f - VictimReduction) * (1.f - HolderReduction));
		ShareEffect.bDoNotRedirectSacrifice = true;
		ShareEffect.bSkipStatModifiers = true;
		ShareEffect.bLimitGameplayCue = true;

		UAbilitySystemComponent* SourceASC = DamageContext.GetOriginalInstigatorAbilitySystemComponent();
		GeoASLib::ApplySingleEffectData(ShareEffect, IsValid(SourceASC) ? SourceASC : HolderASC, HolderASC, 1, 0,
										FGameplayTag());
		SparedDamage += Share;
	}
}
