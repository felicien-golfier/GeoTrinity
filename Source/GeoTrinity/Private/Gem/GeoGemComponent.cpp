// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Gem/GeoGemComponent.h"

#include "AbilitySystem/Abilities/Gem/GeoCorePassiveAbility.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "Characters/PlayableCharacter.h"
#include "Engine/LocalPlayer.h"
#include "GameClasses/Match/GeoPlayerState.h"
#include "GameFramework/GameState.h"
#include "GameFramework/PlayerController.h"
#include "Gem/GeoGemCatalog.h"
#include "Gem/GeoGemProfileSave.h"
#include "Gem/GeoGemStatsEffect.h"
#include "Gem/GeoGemSubsystem.h"

UGeoGemComponent::UGeoGemComponent()
{
	SetIsReplicatedByDefault(true);
}

void UGeoGemComponent::SendAllToServer(UGeoGemProfileSave const& Profile)
{
	ServerSetEchoLevel(Profile.GetEchoLevel());
	for (uint8 i = static_cast<uint8>(EPlayerClass::None) + 1; i < static_cast<uint8>(EPlayerClass::All); i++)
	{
		SendLoadoutToServer(Profile, static_cast<EPlayerClass>(i));
	}
}

void UGeoGemComponent::SendLoadoutToServer(UGeoGemProfileSave const& Profile, EPlayerClass const PlayerClass)
{
	ServerSetLoadout(PlayerClass, Profile.GetLoadout(PlayerClass));
}

void UGeoGemComponent::SetLoadout(EPlayerClass const PlayerClass, FGeoGemLoadout const& Loadout)
{
	Loadouts.Add(PlayerClass, Loadout);

	AGeoPlayerState const* PlayerState = GetOwner<AGeoPlayerState>();
	AGameState const* GameState = GetWorld()->GetGameState<AGameState>();
	APlayableCharacter* Character = PlayerState->GetPawn<APlayableCharacter>();
	if (PlayerClass == PlayerState->GetPlayerClass() && GameState && !GameState->IsMatchInProgress() && Character
		&& !Character->IsDead())
	{
		ApplyGems();
		Character->RefillHealthAndAmmo();
	}
}

void UGeoGemComponent::ServerSetLoadout_Implementation(EPlayerClass const PlayerClass, FGeoGemLoadout const& Loadout)
{
	SetLoadout(PlayerClass, Loadout);
}

void UGeoGemComponent::ServerSetEchoLevel_Implementation(int32 const NewEchoLevel)
{
	SetEchoLevel(NewEchoLevel);
}

void UGeoGemComponent::ApplyGems()
{
	AGeoPlayerState const* PlayerState = GetOwner<AGeoPlayerState>();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	if (!ensureMsgf(PlayerState, TEXT("%hs: %s is not on an AGeoPlayerState"), __FUNCTION__, *GetName()) || !Catalog)
	{
		return;
	}

	UGeoAbilitySystemComponent* ASC = PlayerState->GetGeoAbilitySystemComponent();
	ASC->RemoveActiveGameplayEffect(StatsEffectHandle);
	FGameplayEffectSpecHandle const SpecHandle =
		UGeoGemStatsEffect::MakeSpec(*ASC, *Catalog, Loadouts.FindRef(PlayerState->GetPlayerClass()), EchoLevel);
	StatsEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);

	for (FGameplayAbilitySpecHandle const& Handle : GrantedAbilityHandles)
	{
		ASC->ClearAbility(Handle);
	}
	GrantedAbilityHandles.Reset();

	for (FName const GemId : Loadouts.FindRef(PlayerState->GetPlayerClass()).Sockets)
	{
		FGeoGemInfo const* Gem = Catalog->Find(GemId);
		if (Gem && Gem->GrantedAbility)
		{
			FGameplayAbilitySpecHandle const Handle = ASC->GiveAbility(FGameplayAbilitySpec(Gem->GrantedAbility, 1));
			ASC->TryActivateAbility(Handle);
			GrantedAbilityHandles.Add(Handle);
		}
	}
}

TArray<FActiveGameplayEffectHandle> UGeoGemComponent::GetSurvivingEffectHandles() const
{
	TArray<FActiveGameplayEffectHandle> Handles;
	if (StatsEffectHandle.IsValid())
	{
		Handles.Add(StatsEffectHandle);
	}

	return Handles;
}

void UGeoGemComponent::GrantReward(FGeoGemReward const& Reward)
{
	ClientGrantReward(Reward);
}

void UGeoGemComponent::ClientGrantReward_Implementation(FGeoGemReward const& Reward)
{
	APlayerController const* PlayerController = GetOwner<AGeoPlayerState>()->GetPlayerController();
	ULocalPlayer const* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	if (ensureMsgf(LocalPlayer, TEXT("%hs: %s has no local player to keep its reward"), __FUNCTION__,
				   *GetOwner()->GetName()))
	{
		LocalPlayer->GetSubsystem<UGeoGemSubsystem>()->GrantReward(Reward);
	}
}
