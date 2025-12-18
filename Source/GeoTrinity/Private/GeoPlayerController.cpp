// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoPlayerController.h"

#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "EnhancedInput/Public/EnhancedInputSubsystems.h"
#include "GameFramework/PlayerState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Tool/GameplayLibrary.h"

AGeoPlayerController::AGeoPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetShowMouseCursor(true);
	ServerTimeOffsetSamples.Reserve(NumSamplesToStabilize - NumSamplesToStabilize % 8 + 8);
}

void AGeoPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetViewTarget(UGameplayStatics::GetActorOfClass(GetWorld(), ACameraActor::StaticClass()));

	if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSystem =
				LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (!InputMapping.IsNull())
			{
				InputSystem->AddMappingContext(InputMapping.LoadSynchronous(), 0);
			}
		}
	}

	// Start periodic time synchronization on server
	if (HasAuthority())
	{
		RequestTimeSync();
	}
}

void AGeoPlayerController::ServerSetAimYaw_Implementation(const float YawDegrees)
{
	if (IsValid(GetPawn()))
	{
		FRotator Rotation = GetPawn()->GetActorRotation();
		Rotation.Yaw = YawDegrees;
		GetPawn()->SetActorRotation(Rotation);
	}
}

void AGeoPlayerController::RequestTimeSync()
{
	// Server
	if (HasServerTime())
	{
		return;
	}

	TimeSyncTimerHandle =
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AGeoPlayerController::RequestTimeSync);
	Pings.Add(GetPlayerState<APlayerState>()->GetPingInMilliseconds());

	SendToClientTheServerTime(GameplayLibrary::GetTime());
}

void AGeoPlayerController::SendToClientTheServerTime_Implementation(const double ServerTimeSeconds)
{
	// Client
	if (HasServerTime())
	{
		return;
	}

	ServerTimeOffsetSamples.Add(GameplayLibrary::GetTime() - ServerTimeSeconds);
	if (GetPlayerState<APlayerState>())
	{
		Pings.Add(GetPlayerState<APlayerState>()->GetPingInMilliseconds());
	}

	if (ServerTimeOffsetSamples.Num() == NumSamplesToStabilize)
	{
		CalculateStableServerTimeOffset();
		bHasServerTime = true;
		SendServerTimeOffsetToServer(ServerTimeOffset);
	}
}

void AGeoPlayerController::SendServerTimeOffsetToServer_Implementation(float StabilizedServerTimeOffset)
{
	// Server
	ServerTimeOffset = StabilizedServerTimeOffset;
	bHasServerTime = true;
	GetWorld()->GetTimerManager().ClearTimer(TimeSyncTimerHandle);
}

void AGeoPlayerController::CalculateStableServerTimeOffset()
{
	checkf(ServerTimeOffsetSamples.Num() > 2 && ServerTimeOffsetSamples.Num() >= NumSamplesToStabilize,
		TEXT("Not enough samples !"));

	ServerTimeOffsetSamples.Sort();

	float Median;

	int32 Count = ServerTimeOffsetSamples.Num();
	int32 MidIndex = Count / 2;

	if (Count % 2 == 0)
	{
		Median = (ServerTimeOffsetSamples[MidIndex - 1] + ServerTimeOffsetSamples[MidIndex]) * 0.5f;
	}
	else
	{
		Median = ServerTimeOffsetSamples[MidIndex];
	}

	TArray<float> FilteredSamples;
	for (float Value : ServerTimeOffsetSamples)
	{
		if (FMath::Abs(Value - Median) <= MaxDeviationFromMedian)
		{
			FilteredSamples.Add(Value);
		}
	}

	// If filtering was too aggressive, fall back to all samples
	if (FilteredSamples.Num() <= 1)
	{
		UE_LOG(LogGeoTrinity, Error, TEXT("filtering was too aggressive, fall back to all samples"))
		FilteredSamples = ServerTimeOffsetSamples;
	}

	float StableServerTimeOffset = 0.f;
	for (float Value : FilteredSamples)
	{
		StableServerTimeOffset += Value;
	}
	StableServerTimeOffset /= FilteredSamples.Num();

	ServerTimeOffset = StableServerTimeOffset;
}

double AGeoPlayerController::GetServerTime() const
{
	// TODO: Check if HasAuth is on server only !
	if (HasAuthority())
	{
		return GameplayLibrary::GetTime();
	}
	else
	{
		ensureMsgf(HasServerTime(), TEXT("Server time is not available yet. please request only when ready !"));
		return GameplayLibrary::GetTime() + ServerTimeOffset;
	}
}

bool AGeoPlayerController::HasServerTime(const UWorld* World)
{
	if (World->IsNetMode(NM_DedicatedServer) || World->IsNetMode(NM_ListenServer))
	{
		return true;
	}

	if (AGeoPlayerController* GeoPlayerController = GetLocalGeoPlayerController(World))
	{
		return GeoPlayerController->HasServerTime();
	}

	return false;
}

double AGeoPlayerController::GetServerTime(const UWorld* World)
{
	if (World->IsNetMode(NM_DedicatedServer) || World->IsNetMode(NM_ListenServer))
	{
		return GameplayLibrary::GetTime();
	}

	if (AGeoPlayerController* GeoPlayerController = GetLocalGeoPlayerController(World))
	{
		return GeoPlayerController->GetServerTime();
	}

	ensureMsgf(false, TEXT("No local GeoPlayerController found!"));
	return 0.0;
}

AGeoPlayerController* AGeoPlayerController::GetLocalGeoPlayerController(const UWorld* World)
{
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if (APlayerController* PlayerController = Iterator->Get())
		{
			if (PlayerController && PlayerController->IsLocalController()
				&& PlayerController->IsA(AGeoPlayerController::StaticClass()))
			{
				return CastChecked<AGeoPlayerController>(PlayerController);
			}
		}
	}
	return nullptr;
}