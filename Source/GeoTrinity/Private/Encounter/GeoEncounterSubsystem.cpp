// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/GeoEncounterSubsystem.h"

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/HiddenCasterCharacter.h"
#include "Encounter/GeoEncounter.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Tool/UGeoGameplayLibrary.h"

void UGeoEncounterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	WorldTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddLambda(
		[this](UWorld* const TornDownWorld)
		{
			if (TornDownWorld == GetWorld())
			{
				AbortEncounters();
			}
		});

#if WITH_EDITOR
	PIEEndedHandle = FWorldDelegates::OnPIEEnded.AddLambda(
		[this](UGameInstance* const EndedGameInstance)
		{
			if (EndedGameInstance == GetWorld()->GetGameInstance())
			{
				AbortEncounters();
			}
		});
#endif
}

void UGeoEncounterSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldBeginTearDown.Remove(WorldTearDownHandle);
#if WITH_EDITOR
	FWorldDelegates::OnPIEEnded.Remove(PIEEndedHandle);
#endif

	Super::Deinitialize();
}

void UGeoEncounterSubsystem::Raise(FGameplayTag const Tag, FGeoEncounterPayload Payload)
{
	Payload.Tag = Tag;
	OnRaised.Broadcast(Payload);
}

AHiddenCasterCharacter* UGeoEncounterSubsystem::GetSharedCaster()
{
	if (!IsValid(SharedCaster) && GeoLib::IsServer(GetWorld()))
	{
		SharedCaster = Cast<AHiddenCasterCharacter>(AEnemyCharacter::SpawnEnemy(
			*GetWorld(), AHiddenCasterCharacter::StaticClass(), FVector::ZeroVector, nullptr, nullptr, nullptr));
	}

	return SharedCaster;
}

FGameplayAbilitySpecHandle
UGeoEncounterSubsystem::GrantSharedAbility(TSubclassOf<UGeoGameplayAbility> const AbilityClass)
{
	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(GetSharedCaster());
	if (!ASC || !AbilityClass
		|| !ensureMsgf(!GetDefault<UGeoGameplayAbility>(AbilityClass)->RequiresArena(),
					   TEXT("%s needs its caster's arena: the encounters' shared caster has none"),
					   *AbilityClass->GetName()))
	{
		return FGameplayAbilitySpecHandle();
	}

	FGameplayAbilitySpec const* const Spec = ASC->FindAbilitySpecFromClass(AbilityClass);
	return Spec ? Spec->Handle : ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
}

void UGeoEncounterSubsystem::AbortEncounters()
{
	if (!bSessionEnding)
	{
		bSessionEnding = true;
		for (TActorIterator<AGeoEncounter> It(GetWorld()); It; ++It)
		{
			It->Abort();
		}
	}
}
