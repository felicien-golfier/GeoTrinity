// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoPlayerController.h"

#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "EnhancedInput/Public/EnhancedInputSubsystems.h"
#include "GeoInputComponent.h"
#include "GeoPawn.h"
#include "InputStep.h"
#include "Kismet/GameplayStatics.h"
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
		if (UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (!InputMapping.IsNull())
			{
				InputSystem->AddMappingContext(InputMapping.LoadSynchronous(), 0);
			}
		}
	}

	// Start periodic time synchronization on owning client only
	if (IsLocalController())
	{
		ScheduleTimeSync();
	}
}

void AGeoPlayerController::OnPossess(APawn* APawn)
{
	Super::OnPossess(APawn);
}

static double GetAccurateRealTimeSeconds()
{
	int32 Secs = 0;
	double Part = 0.0;
	UGameplayStatics::GetAccurateRealTime(Secs, Part);
	return static_cast<double>(Secs) + Part;
}

void AGeoPlayerController::ScheduleTimeSync()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TimeSyncTimerHandle, this, &AGeoPlayerController::SendTimeSyncRequest, 0.5f, true,
			FMath::FRandRange(0.1f, 0.3f));
	}
}

void AGeoPlayerController::SendTimeSyncRequest()
{
	if (!IsLocalController())
	{
		return;
	}
	ServerRequestServerTime(GetAccurateRealTimeSeconds());
}

void AGeoPlayerController::ServerRequestServerTime_Implementation(double ClientSendTimeSeconds)
{
	// Called on server: respond with current server real time seconds
	const double ServerNow = GetAccurateRealTimeSeconds();
	ClientReportServerTime(ClientSendTimeSeconds, ServerNow);
}

void AGeoPlayerController::ClientReportServerTime_Implementation(double ClientSendTimeSeconds, double ServerTimeSeconds)
{
	const double ClientReceive = GetAccurateRealTimeSeconds();
	const double Rtt = FMath::Max(0.0, ClientReceive - ClientSendTimeSeconds);
	const double OneWay = 0.5 * Rtt;
	const double EstimatedOffset = ServerTimeSeconds - (ClientReceive - OneWay);
	if (!bHasServerTimeOffset)
	{
		ServerTimeOffsetSeconds = EstimatedOffset;
		bHasServerTimeOffset = true;
	}
	else
	{
		// Smooth to reduce jitter
		ServerTimeOffsetSeconds = FMath::Lerp(ServerTimeOffsetSeconds, EstimatedOffset, 0.1);
	}
}

double AGeoPlayerController::GetEstimatedServerTimeSeconds() const
{
	if (!IsLocalController())
	{
		return 0.0;
	}
	int32 Secs = 0;
	double Part = 0.0;
	UGameplayStatics::GetAccurateRealTime(Secs, Part);
	const double ClientNow = static_cast<double>(Secs) + Part;
	return bHasServerTimeOffset ? (ClientNow + ServerTimeOffsetSeconds) : 0.0;
}