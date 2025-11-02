// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoPlayerController.h"

#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "EnhancedInput/Public/EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/GeoInputGameInstanceSubsystem.h"
#include "TimerManager.h"

AGeoPlayerController::AGeoPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
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
	if (!IsLocalController())
	{
		SendTimeSyncRequest();
	}
}

void AGeoPlayerController::OnPossess(APawn* APawn)
{
	Super::OnPossess(APawn);
}

void AGeoPlayerController::ClientSendServerTimeOffset_Implementation(float ServerTimeOffset)
{
	UGeoInputGameInstanceSubsystem::GetInstance(GetWorld())->SetServerTimeOffset(this, ServerTimeOffset);
}

void AGeoPlayerController::SendTimeSyncRequest()
{
	if (!IsServerTimeOffsetStable())
	{
		TimeSyncTimerHandle =
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AGeoPlayerController::SendTimeSyncRequest);
	}

	RequestClientTime(FGeoTime::GetAccurateRealTime());
}

void AGeoPlayerController::RequestClientTime_Implementation(const FGeoTime ServerTimeSeconds)
{
	ReportClientTime(FGeoTime::GetAccurateRealTime(), ServerTimeSeconds);
}

void AGeoPlayerController::ReportClientTime_Implementation(const FGeoTime ClientSendTimeSeconds,
	const FGeoTime ServerTimeSeconds)
{
	// In case we have buffered RPC that arrived after NumSamplesToStabilize times, ignore them.
	if (IsServerTimeOffsetStable())
	{
		return;
	}

	const float Forth = ClientSendTimeSeconds - ServerTimeSeconds;
	const float Back = FGeoTime::GetAccurateRealTime() - ClientSendTimeSeconds;
	ServerTimeOffsetSamples.Add((Forth + Back) / 2.f);

	if (ServerTimeOffsetSamples.Num() == NumSamplesToStabilize)
	{
		float ServerTimeOffset = 0.f;
		GetWorld()->GetTimerManager().ClearTimer(TimeSyncTimerHandle);
		for (float ServerTimeOffsetSample : ServerTimeOffsetSamples)
		{
			ServerTimeOffset += ServerTimeOffsetSample;
		}

		ServerTimeOffset /= ServerTimeOffsetSamples.Num();
		UGeoInputGameInstanceSubsystem::GetInstance(GetWorld())->SetServerTimeOffset(this, ServerTimeOffset);
		ClientSendServerTimeOffset(ServerTimeOffset);
	}
}

bool AGeoPlayerController::IsServerTimeOffsetStable() const
{
	return ServerTimeOffsetSamples.Num() >= NumSamplesToStabilize;
}