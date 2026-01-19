#include "Tool/GameplayLibrary.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

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

float GameplayLibrary::GetServerTime(const UWorld* World, const bool bUpdatedWithPing)
{
	if (World->IsNetMode(NM_DedicatedServer) || World->IsNetMode(NM_ListenServer))
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
