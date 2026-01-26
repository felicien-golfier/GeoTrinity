#include "Tool/GameplayLibrary.h"

#include "AbilitySystem/Abilities/AbilityPayload.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GeoTrinity/GeoTrinity.h"

bool GameplayLibrary::GetTeamInterface(const AActor* Actor, const IGenericTeamAgentInterface*& OutInterface)
{
	OutInterface = Cast<const IGenericTeamAgentInterface>(Actor);
	return OutInterface != nullptr;
}

FColor GameplayLibrary::GetColorForObject(const UObject* Object)
{
	if (!IsValid(Object))
	{
		return FColor::White;
	}

	static const FColor Palette[] = {FColor::Black, FColor::Red, FColor::Green, FColor::Blue, FColor::Yellow,
		FColor::Cyan, FColor::Magenta, FColor::Orange, FColor::Emerald, FColor::Purple, FColor::Turquoise,
		FColor::Silver};

	return Palette[Object->GetUniqueID() % std::size(Palette)];
}

float GameplayLibrary::IsServer(const UWorld* World)
{
	return World->IsNetMode(NM_DedicatedServer) || World->IsNetMode(NM_ListenServer);
}

float GameplayLibrary::GetServerTime(const UWorld* World, const bool bUpdatedWithPing)
{
	if (IsServer(World))
	{
		return World->GetTimeSeconds();
	}

	float ServerTimeSeconds = World->GetGameState()->GetServerWorldTimeSeconds();

	if (bUpdatedWithPing)
	{
		const APlayerController* LocalPlayerController = World->GetFirstPlayerController();
		if (!IsValid(LocalPlayerController))
		{
			UE_LOG(LogTemp, Error, TEXT("No local player controller found"));
			return ServerTimeSeconds;
		}

		const APlayerState* PlayerState = LocalPlayerController->GetPlayerState<APlayerState>();
		if (!IsValid(PlayerState))
		{
			UE_LOG(LogTemp, Error, TEXT("No local player state found"));
			return ServerTimeSeconds;
		}
		const float OnWayPingSec =
			LocalPlayerController->GetPlayerState<APlayerState>()->GetPingInMilliseconds() * 0.0005f;
		ServerTimeSeconds += OnWayPingSec;
	}

	return ServerTimeSeconds;
}

int GameplayLibrary::GetAndCheckSection(const UAnimMontage* AnimMontage, const FName Section)
{
	const int SectionIndex = AnimMontage->GetSectionIndex(Section);
	if (SectionIndex == INDEX_NONE)
	{
		ensureMsgf(true, TEXT("Section %s not found in AnimMontage %s"), *Section.ToString(), *AnimMontage->GetName());
		UE_LOG(LogPattern, Error, TEXT("Section %s not found in AnimMontage %s"), *Section.ToString(),
			*AnimMontage->GetName());
	}
	return SectionIndex;
}

UAnimInstance* GameplayLibrary::GetAnimInstance(const FAbilityPayload& Payload)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(Payload.Owner);
	if (!IsValid(OwnerCharacter))
	{
		UE_LOG(LogPattern, Error, TEXT("We support only animation montage for character in pattern for now !"));
		return nullptr;
	}

	UAnimInstance* AnimInstance = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogPattern, Error, TEXT("Please set an anim instance (With the Default Slot filled in anim graph;)"));
		return nullptr;
	}

	return AnimInstance;
}