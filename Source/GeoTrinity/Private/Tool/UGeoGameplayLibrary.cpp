#include "Tool/UGeoGameplayLibrary.h"

#include "Actor/Arena/GeoArenaVolume.h"
#include "Actor/GeoTargetPoint.h"
#include "Camera/CameraShakeBase.h"
#include "Characters/PlayableCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "Tool/GeoColor.h"
#include "VisualLogger/VisualLogger.h"


FColor UGeoGameplayLibrary::GetRandomColor()
{
	return ColorPalette[FMath::RandRange(0, std::size(ColorPalette) - 1)];
}

FColor UGeoGameplayLibrary::GetColorForObject(UObject const* Object)
{
	if (!IsValid(Object))
	{
		return FColor::White;
	}

	return ColorPalette[Object->GetUniqueID() % std::size(ColorPalette)];
}

FLinearColor UGeoGameplayLibrary::GetPaletteColorFromIndex(int const ColorIndex, float const Alpha)
{
	return GetPaletteColor(static_cast<EGeoColor>(ColorIndex), Alpha);
}

FLinearColor UGeoGameplayLibrary::GetPaletteColor(EGeoColor const Color, float const Alpha)
{
	FGeoColorParam ColorParam;
	ColorParam.Color = Color;
	return ColorParam.GetColor(Alpha);
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
	if (!ensureMsgf(WorldContextObject, TEXT("%hs: WorldContextObject is invalid"), __FUNCTION__))
	{
		return false;
	}
	return IsServer(WorldContextObject->GetWorld());
}

bool UGeoGameplayLibrary::IsServer(UWorld const* World)
{
	return World->IsNetMode(NM_DedicatedServer) || World->IsNetMode(NM_ListenServer);
}

bool UGeoGameplayLibrary::IsDedicatedServer(UObject const* WorldContextObject)
{
	if (!ensureMsgf(WorldContextObject, TEXT("%hs: WorldContextObject is invalid"), __FUNCTION__))
	{
		return false;
	}
	return IsDedicatedServer(WorldContextObject->GetWorld());
}

bool UGeoGameplayLibrary::IsDedicatedServer(UWorld const* World)
{
	return World->IsNetMode(NM_DedicatedServer);
}

bool UGeoGameplayLibrary::IsLocalPlayerAvatar(AActor const* Actor)
{
	return IsLocalPlayerAvatar(Cast<APawn>(Actor));
}

bool UGeoGameplayLibrary::IsLocalPlayerAvatar(APawn const* Pawn)
{
	return Pawn && Pawn->IsPlayerControlled() && Pawn->IsLocallyControlled();
}

bool UGeoGameplayLibrary::IsKeyboardMousePlayer(APlayerController const* PlayerController)
{
	ULocalPlayer const* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	return LocalPlayer && LocalPlayer->GetLocalPlayerIndex() == 0;
}
float UGeoGameplayLibrary::GetServerTime(UObject const* WorldContextObject, bool bUpdatedWithPing)
{
	if (!ensureMsgf(WorldContextObject, TEXT("%hs: WorldContextObject is invalid"), __FUNCTION__))
	{
		return 0.f;
	}

	return GetServerTime(WorldContextObject->GetWorld(), bUpdatedWithPing);
}

float UGeoGameplayLibrary::GetOnWayPingSec(UWorld const* World)
{
	APlayerController const* LocalPlayerController = World->GetFirstPlayerController();
	if (!IsValid(LocalPlayerController))
	{
		UE_LOG(LogTemp, Error, TEXT("No local player controller found"));
		return 0.f;
	}

	APlayerState const* PlayerState = LocalPlayerController->GetPlayerState<APlayerState>();
	if (!IsValid(PlayerState))
	{
		UE_LOG(LogTemp, Error, TEXT("No local player state found"));
		return 0.f;
	}

	float const OnWayPingSec = LocalPlayerController->GetPlayerState<APlayerState>()->GetPingInMilliseconds() * 0.0005f;
	return OnWayPingSec;
}
float UGeoGameplayLibrary::GetServerTime(UWorld const* World, bool const bUpdatedWithPing)
{
	if (IsServer(World))
	{
		return World->GetTimeSeconds();
	}

	if (!ensureMsgf(World->GetGameState(), TEXT("%hs: GameState does not exist"), __FUNCTION__))
	{
		return 0.f;
	}

	float ServerTimeSeconds = World->GetGameState()->GetServerWorldTimeSeconds();

	if (bUpdatedWithPing)
	{
		ServerTimeSeconds += GetOnWayPingSec(World);
	}

	return ServerTimeSeconds;
}

TArray<AActor*> UGeoGameplayLibrary::GetTargetPoints(UObject const* WorldContextObject, FGameplayTag const PurposeTag,
													 FGameplayTag const ArenaTag)
{
	TArray<AActor*> AllPoints;
	UGameplayStatics::GetAllActorsOfClass(WorldContextObject->GetWorld(), AGeoTargetPoint::StaticClass(), AllPoints);

	TArray<AActor*> SpawnPoints = AllPoints.FilterByPredicate(
		[&PurposeTag, &ArenaTag](AActor const* Actor)
		{
			FGameplayTagContainer const& Tags = CastChecked<AGeoTargetPoint>(Actor)->GameplayTags;
			return Tags.HasTag(PurposeTag) && Tags.HasTag(ArenaTag);
		});

	if (SpawnPoints.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("GeoLib::GetTargetPoints — no AGeoTargetPoint tagged %s for arena %s"),
			   *PurposeTag.ToString(), *ArenaTag.ToString());
	}

	return SpawnPoints;
}

void UGeoGameplayLibrary::TeleportPlayersToTargetPoints(UObject const* WorldContextObject,
														FGameplayTag const PurposeTag, FGameplayTag const ArenaTag,
														bool const bSkipPlayersInArenaVolume)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!ensureMsgf(World, TEXT("TeleportPlayersToTargetPoints: no world")))
	{
		return;
	}

	TArray<AActor*> const SpawnPoints = GetTargetPoints(WorldContextObject, PurposeTag, ArenaTag);
	if (!ensureMsgf(!SpawnPoints.IsEmpty(), TEXT("Ensure to add Spawn points tagged %s + %s in your map, DUMBASS"),
					*PurposeTag.GetTagName().ToString(), *ArenaTag.GetTagName().ToString()))
	{
		return;
	}

	int32 SpawnIndex = 0;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APawn* Pawn = It->IsValid() ? (*It)->GetPawn() : nullptr;
		if (!IsValid(Pawn) ||
			(bSkipPlayersInArenaVolume && AGeoArenaVolume::IsPawnInside(WorldContextObject, *Pawn, ArenaTag)))
		{
			continue;
		}

		Pawn->SetActorLocation(SpawnPoints[SpawnIndex % SpawnPoints.Num()]->GetActorLocation());
		++SpawnIndex;
	}
}

TArray<APlayableCharacter*> UGeoGameplayLibrary::GetAlivePlayers(UObject const* WorldContextObject)
{
	TArray<APlayableCharacter*> AlivePlayers;
	UWorld const* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!ensureMsgf(World, TEXT("GetAlivePlayers: no world")))
	{
		return AlivePlayers;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayableCharacter* Player = It->IsValid() ? Cast<APlayableCharacter>((*It)->GetPawn()) : nullptr;
		if (IsValid(Player) && !Player->IsDead())
		{
			AlivePlayers.Add(Player);
		}
	}

	return AlivePlayers;
}

APawn* UGeoGameplayLibrary::ResolveOwnerPawn(UObject* Owner)
{
	if (AController const* Controller = Cast<AController>(Owner))
	{
		return Controller->GetPawn();
	}

	return Cast<APawn>(Owner);
}
