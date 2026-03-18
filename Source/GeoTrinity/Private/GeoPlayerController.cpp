// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GeoPlayerController.h"

#include "Camera/CameraActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EnhancedInput/Public/EnhancedInputSubsystems.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "GeoPlayerState.h"
#include "System/GeoCombatStatsSubsystem.h"
#include "TimerManager.h"
#include "Tool/UGameplayLibrary.h"

#if !UE_BUILD_SHIPPING
static bool bShowPing = false;

static FAutoConsoleCommandWithWorld GShowPingCommand(TEXT("Geo.ShowPing"), TEXT("Toggle on-screen ping display"),
													 FConsoleCommandWithWorldDelegate::CreateLambda(
														 [](UWorld*)
														 {
															 bShowPing = !bShowPing;
														 }));

static TAutoConsoleVariable<bool>
	CVarShowCombatStats(TEXT("Geo.ShowCombatStats"), false,
						TEXT("When true, shows per-player DPS / HPS / damage received on screen"));
#endif

AGeoPlayerController::AGeoPlayerController(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	SetShowMouseCursor(true);
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
}

AGeoPlayerController* AGeoPlayerController::GetLocalGeoPlayerController(UWorld const* World)
{
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if (APlayerController* PlayerController = Iterator->Get())
		{
			if (PlayerController && PlayerController->IsLocalController() && PlayerController->IsA(StaticClass()))
			{
				return CastChecked<AGeoPlayerController>(PlayerController);
			}
		}
	}
	return nullptr;
}

#if !UE_BUILD_SHIPPING
void AGeoPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bShowPing && IsLocalController() && PlayerState)
	{
		float const Ping = PlayerState->GetPingInMilliseconds();
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green, FString::Printf(TEXT("Ping: %.0f ms"), Ping));
	}

	if (CVarShowCombatStats.GetValueOnGameThread())
	{
		if (UGeoCombatStatsSubsystem* CombatStats = GetWorld()->GetSubsystem<UGeoCombatStatsSubsystem>())
		{
			if (HasAuthority())
			{
				CombatStats->ComputePlayerStats(GetWorld()->GetTimeSeconds());
			}
			if (IsLocalController())
			{
				CombatStats->DisplayLines(Cast<AGeoPlayerState>(PlayerState));
			}
		}
	}
}
#endif
