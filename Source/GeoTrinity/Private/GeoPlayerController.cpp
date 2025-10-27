// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoPlayerController.h"

#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "EnhancedInput/Public/EnhancedInputSubsystems.h"
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

double AGeoPlayerController::GetServerTimeOffsetSeconds() const
{
	return ServerTimeOffsetSeconds;
}
FGeoTime AGeoPlayerController::GetHestimatedServerTime() const
{
	FGeoTime LocalTime = FGeoTime::GetAccurateRealTime();
	return LocalTime + ServerTimeOffsetSeconds;
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
	ServerRequestServerTime(FGeoTime::GetAccurateRealTime());
}

void AGeoPlayerController::ServerRequestServerTime_Implementation(const FGeoTime ClientSendTimeSeconds)
{
	// Called on server: respond with current server real time seconds
	const FGeoTime ServerNow = FGeoTime::GetAccurateRealTime();
	ClientReportServerTime(ClientSendTimeSeconds, ServerNow);
}

void AGeoPlayerController::ClientReportServerTime_Implementation(const FGeoTime ClientSendTimeSeconds, const FGeoTime ServerTimeSeconds)
{
	const FGeoTime ClientReceive = FGeoTime::GetAccurateRealTime();
	const double Rtt = FMath::Max(0.0, ClientReceive.GetTimeDiff(ClientSendTimeSeconds));
	const double OneWay = 0.5 * Rtt;
	const double EstimatedOffset = ServerTimeSeconds.GetTimeDiff(ClientReceive - OneWay);
	if (ServerTimeOffsetSeconds == 0.0f)
	{
		ServerTimeOffsetSeconds = EstimatedOffset;
	}
	else
	{
		// Smooth to reduce jitter
		ServerTimeOffsetSeconds = FMath::Lerp(ServerTimeOffsetSeconds, EstimatedOffset, 0.1);
	}
}
