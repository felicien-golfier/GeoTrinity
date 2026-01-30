#include "Tool/GameplayLibrary.h"

#include "AbilitySystem/Abilities/AbilityPayload.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GeoTrinity/GeoTrinity.h"

bool GameplayLibrary::GetTeamInterface(AActor const* Actor, IGenericTeamAgentInterface const*& OutInterface)
{
	OutInterface = Cast<IGenericTeamAgentInterface const>(Actor);
	return OutInterface != nullptr;
}

FColor GameplayLibrary::GetColorForObject(UObject const* Object)
{
	if (!IsValid(Object))
	{
		return FColor::White;
	}

	static FColor const Palette[] = {FColor::Black,	  FColor::Red,	  FColor::Green,	 FColor::Blue,
									 FColor::Yellow,  FColor::Cyan,	  FColor::Magenta,	 FColor::Orange,
									 FColor::Emerald, FColor::Purple, FColor::Turquoise, FColor::Silver};

	return Palette[Object->GetUniqueID() % std::size(Palette)];
}

float GameplayLibrary::IsServer(UWorld const* World)
{
	return World->IsNetMode(NM_DedicatedServer) || World->IsNetMode(NM_ListenServer);
}

float GameplayLibrary::GetServerTime(UWorld const* World, bool const bUpdatedWithPing)
{
	if (IsServer(World))
	{
		return World->GetTimeSeconds();
	}

	float ServerTimeSeconds = World->GetGameState()->GetServerWorldTimeSeconds();

	if (bUpdatedWithPing)
	{
		APlayerController const* LocalPlayerController = World->GetFirstPlayerController();
		if (!IsValid(LocalPlayerController))
		{
			UE_LOG(LogTemp, Error, TEXT("No local player controller found"));
			return ServerTimeSeconds;
		}

		APlayerState const* PlayerState = LocalPlayerController->GetPlayerState<APlayerState>();
		if (!IsValid(PlayerState))
		{
			UE_LOG(LogTemp, Error, TEXT("No local player state found"));
			return ServerTimeSeconds;
		}
		float const OnWayPingSec =
			LocalPlayerController->GetPlayerState<APlayerState>()->GetPingInMilliseconds() * 0.0005f;
		ServerTimeSeconds += OnWayPingSec;
	}

	return ServerTimeSeconds;
}

int GameplayLibrary::GetAndCheckSection(UAnimMontage const* AnimMontage, FName const Section)
{
	int const SectionIndex = AnimMontage->GetSectionIndex(Section);
	if (SectionIndex == INDEX_NONE)
	{
		ensureMsgf(true, TEXT("Section %s not found in AnimMontage %s"), *Section.ToString(), *AnimMontage->GetName());
		UE_LOG(LogPattern, Error, TEXT("Section %s not found in AnimMontage %s"), *Section.ToString(),
			   *AnimMontage->GetName());
	}
	return SectionIndex;
}

UAnimInstance* GameplayLibrary::GetAnimInstance(FAbilityPayload const& Payload)
{
	ACharacter* InstigatorCharacter = Cast<ACharacter>(Payload.Instigator);
	if (!IsValid(InstigatorCharacter))
	{
		UE_LOG(LogPattern, Error, TEXT("We support only animation montage for character in pattern for now !"));
		return nullptr;
	}

	UAnimInstance* AnimInstance =
		InstigatorCharacter->GetMesh() ? InstigatorCharacter->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogPattern, Error, TEXT("Please set an anim instance (With the Default Slot filled in anim graph;)"));
		return nullptr;
	}

	return AnimInstance;
}
