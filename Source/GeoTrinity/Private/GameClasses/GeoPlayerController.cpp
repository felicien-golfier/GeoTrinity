// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GameClasses/GeoPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EnhancedInput/Public/EnhancedInputComponent.h"
#include "EnhancedInput/Public/EnhancedInputSubsystems.h"
#include "GameFramework/PlayerState.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"
#include "System/GeoCombatStatsSubsystem.h"
#include "TimerManager.h"
#include "Tool/UGeoGameplayLibrary.h"

#if !UE_BUILD_SHIPPING
static bool bShowPing = false;

static FAutoConsoleCommandWithWorld GShowPingCommand(TEXT("Geo.ShowPing"), TEXT("Toggle on-screen ping display"),
													 FConsoleCommandWithWorldDelegate::CreateLambda(
														 [](UWorld*)
														 {
															 bShowPing = !bShowPing;
														 }));
#endif

AGeoPlayerController::AGeoPlayerController(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	SetShowMouseCursor(true);
}

void AGeoPlayerController::BeginPlay()
{
	Super::BeginPlay();
	CurrentMouseCursor = EMouseCursor::Crosshairs;
	// With a visible cursor the viewport regularly loses mouse capture; the default GameOnly mode consumes the click
	// that re-acquires capture, so most ability clicks would never reach input processing.
	SetInputMode(FInputModeGameOnly().SetConsumeCaptureMouseDown(false));
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

void AGeoPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!ensureMsgf(EnhancedInput && !ToggleMenuAction.IsNull(),
					TEXT("AGeoPlayerController: ToggleMenuAction is not set")))
	{
		return;
	}
	EnhancedInput->BindAction(ToggleMenuAction.LoadSynchronous(), ETriggerEvent::Started, this,
							  &AGeoPlayerController::HandleToggleMenu);
}

void AGeoPlayerController::HandleToggleMenu(FInputActionInstance const& /*Instance*/)
{
	TogglePauseMenu();
}

void AGeoPlayerController::TogglePauseMenu()
{
	if (PauseMenuWidget && PauseMenuWidget->IsInViewport())
	{
		ClosePauseMenu();
		return;
	}
	if (!ensureMsgf(PauseMenuWidgetClass, TEXT("AGeoPlayerController: PauseMenuWidgetClass is not set")))
	{
		return;
	}
	if (!PauseMenuWidget)
	{
		PauseMenuWidget = CreateWidget<UUserWidget>(this, PauseMenuWidgetClass);
	}
	PauseMenuWidget->AddToViewport();
	SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
}

void AGeoPlayerController::ClosePauseMenu()
{
	if (PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
	}
	SetInputMode(FInputModeGameOnly().SetConsumeCaptureMouseDown(false));
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

	if (GeoLib::IsServer(this) && UGeoCombatStatsSubsystem::IsDebugDisplayEnabled())
	{
		if (UGeoCombatStatsSubsystem* CombatStats = GetWorld()->GetSubsystem<UGeoCombatStatsSubsystem>())
		{
			CombatStats->ComputePlayerStats(GetWorld()->GetTimeSeconds());
		}
	}
}
#endif
