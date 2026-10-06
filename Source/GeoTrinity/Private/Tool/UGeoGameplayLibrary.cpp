#include "Tool/UGeoGameplayLibrary.h"

#include "Actor/Arena/GeoArenaVolume.h"
#include "Actor/GeoTargetPoint.h"
#include "Camera/CameraShakeBase.h"
#include "Characters/GeoCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HAL/PlatformProcess.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Settings/GameDataSettings.h"
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

float UGeoGameplayLibrary::GetPerceivedServerTime(AActor const* Actor)
{
	AGeoCharacter const* const Character = Cast<AGeoCharacter>(Actor);
	if (!IsValid(Character))
	{
		return GetServerTime(Actor, true);
	}

	return Character->GetGeoMovementComponent()->GetPerceivedServerTime();
}

float UGeoGameplayLibrary::GetReplicationDelay(AActor const* Viewer)
{
	APawn const* const Pawn = Cast<APawn>(Viewer);
	if (!IsValid(Pawn) || !Pawn->IsPlayerControlled() || Pawn->IsLocallyControlled())
	{
		return 0.f;
	}

	float const HalfPing = Pawn->GetPlayerState()->GetPingInMilliseconds() * 0.0005f;
	return FMath::Min(HalfPing, GetDefault<UGameDataSettings>()->MaxLatencyCompensation);
}

FGeoPose UGeoGameplayLibrary::GetPoseAt(AActor const* Actor, float const ServerTime)
{
	AGeoCharacter const* const Character = Cast<AGeoCharacter>(Actor);
	if (!IsValid(Character))
	{
		return GetCurrentPose(Actor);
	}

	return Character->GetGeoMovementComponent()->GetPoseAt(ServerTime);
}

FGeoPose UGeoGameplayLibrary::GetCurrentPose(AActor const* Actor)
{
	return {GetServerTime(Actor, true), Actor->GetActorLocation(), static_cast<float>(Actor->GetActorRotation().Yaw)};
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

USaveGame* UGeoGameplayLibrary::LoadUserSaveFile(FString const& FileName, TSubclassOf<USaveGame> SaveClass)
{
	FString const Path = GetUserSaveFilePath(FileName);
	TArray<uint8> SaveData;
	// No file yet is the normal first run, not a failure.
	if (FFileHelper::LoadFileToArray(SaveData, *Path))
	{
		USaveGame* Saved = UGameplayStatics::LoadGameFromMemory(SaveData);
		if (IsValid(Saved) && Saved->IsA(SaveClass))
		{
			return Saved;
		}
		UE_LOG(LogGeoTrinity, Warning, TEXT("%hs: %s could not be read — starting a fresh one"), __FUNCTION__, *Path);
	}
	return UGameplayStatics::CreateSaveGameObject(SaveClass);
}

void UGeoGameplayLibrary::WriteUserSaveFile(USaveGame* SaveGame, FString const& FileName)
{
	FString const Path = GetUserSaveFilePath(FileName);
	TArray<uint8> SaveData;
	bool const bWritten =
		UGameplayStatics::SaveGameToMemory(SaveGame, SaveData) && FFileHelper::SaveArrayToFile(SaveData, *Path);
	ensureMsgf(bWritten, TEXT("%hs: failed to write %s"), __FUNCTION__, *Path);
}

FString UGeoGameplayLibrary::GetUserSaveFilePath(FString const& FileName)
{
	return FPaths::Combine(FPlatformProcess::UserSettingsDir(), FApp::GetProjectName(), FileName);
}
