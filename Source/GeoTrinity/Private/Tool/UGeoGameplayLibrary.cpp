#include "Tool/UGeoGameplayLibrary.h"

#include "Camera/CameraShakeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "VisualLogger/VisualLogger.h"


FColor UGeoGameplayLibrary::GetColorForObject(UObject const* Object)
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

void UGeoGameplayLibrary::TriggerCameraShake(UObject const* WorldContextObject,
											 TSubclassOf<UCameraShakeBase> ShakeClass, float Scale)
{
	if (!ensureMsgf(ShakeClass, TEXT("TriggerCameraShake: ShakeClass is null")))
	{
		return;
	}
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}
	PlayerController->ClientStartCameraShake(ShakeClass, Scale);
}

bool UGeoGameplayLibrary::IsServer(UObject const* WorldContextObject)
{
	if (!WorldContextObject)
	{
		ensureMsgf(false, TEXT("WorldContextObject is invalid!"));
		return 0.f;
	}
	return IsServer(WorldContextObject->GetWorld());
}

bool UGeoGameplayLibrary::IsServer(UWorld const* World)
{
	return World->IsNetMode(NM_DedicatedServer) || World->IsNetMode(NM_ListenServer);
}

float UGeoGameplayLibrary::GetServerTime(UObject const* WorldContextObject, bool bUpdatedWithPing)
{
	if (!WorldContextObject)
	{
		ensureMsgf(false, TEXT("WorldContextObject is invalid!"));
		return 0.f;
	}

	return GetServerTime(WorldContextObject->GetWorld(), bUpdatedWithPing);
}

float UGeoGameplayLibrary::GetServerTime(UWorld const* World, bool const bUpdatedWithPing)
{
	if (IsServer(World))
	{
		return World->GetTimeSeconds();
	}

	if (!World->GetGameState())
	{
		ensureMsgf(World->GetGameState(), TEXT("GameState does not exist"));
		return 0.f;
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
