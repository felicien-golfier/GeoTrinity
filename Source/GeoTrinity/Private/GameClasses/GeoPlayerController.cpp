// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GameClasses/GeoPlayerController.h"

#include "Actor/GeoHexArena.h"
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
#include "UserSettings/EnhancedInputUserSettings.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

#if !UE_BUILD_SHIPPING
static bool bShowPing = false;

static FAutoConsoleCommandWithWorld GShowPingCommand(TEXT("Geo.ShowPing"), TEXT("Toggle on-screen ping display"),
													 FConsoleCommandWithWorldDelegate::CreateLambda(
														 [](UWorld*)
														 {
															 bShowPing = !bShowPing;
														 }));

static FAutoConsoleCommandWithWorld GDestroyTileUnderMouseCommand(
	TEXT("Geo.DestroyTileUnderMouse"), TEXT("Destroys the hex arena tile under the mouse cursor"),
	FConsoleCommandWithWorldDelegate::CreateLambda(
		[](UWorld* World)
		{
			AGeoPlayerController* PC = AGeoPlayerController::GetLocalGeoPlayerController(World);
			AGeoHexArena* Arena =
				Cast<AGeoHexArena>(UGameplayStatics::GetActorOfClass(World, AGeoHexArena::StaticClass()));
			if (!ensureMsgf(PC && Arena, TEXT("Geo.DestroyTileUnderMouse: no local player controller or hex arena")))
			{
				return;
			}

			FVector WorldLocation;
			FVector WorldDirection;
			if (!PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection) ||
				FMath::IsNearlyZero(WorldDirection.Z))
			{
				return;
			}
			float const PlaneT = (Arena->GetActorLocation().Z - WorldLocation.Z) / WorldDirection.Z;
			FVector2D const MouseWorldLocation(WorldLocation + WorldDirection * PlaneT);

			FIntPoint Tile;
			if (Arena->GetTileUnderLocation(MouseWorldLocation, Tile))
			{
				Arena->DestroyTiles({Tile});
			}
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
	SetMenuInputMappingActive(false);
	if (IsLocalController())
	{
		SeedKeyBindingsForKeyboardLayout();
	}
}

#if PLATFORM_WINDOWS
namespace
{
	// Returns the key producing, on the active keyboard layout, the same physical key position DefaultKey occupies on
	// the QWERTY (US) layout mapping contexts are authored for. Returns an invalid FKey when no remap applies.
	FKey GetActiveLayoutKeyAtQwertyPosition(FKey const& DefaultKey, HKL QwertyLayout, HKL ActiveLayout)
	{
		uint32 const* KeyCode = nullptr;
		uint32 const* CharCode = nullptr;
		FInputKeyManager::Get().GetCodesFromKey(DefaultKey, KeyCode, CharCode);
		// Letter/digit keys are registered by character code, which equals their Windows virtual-key code. Other
		// character-coded keys (punctuation) have no such identity and are left untouched.
		uint32 const VirtualKey = KeyCode ? *KeyCode : (CharCode && FChar::IsAlnum(*CharCode) ? *CharCode : 0);
		if (VirtualKey == 0)
		{
			return FKey();
		}

		UINT const ScanCode = MapVirtualKeyExW(VirtualKey, MAPVK_VK_TO_VSC, QwertyLayout);
		UINT const LayoutVirtualKey = ScanCode ? MapVirtualKeyExW(ScanCode, MAPVK_VSC_TO_VK, ActiveLayout) : 0;
		if (LayoutVirtualKey == 0 || LayoutVirtualKey == VirtualKey)
		{
			return FKey();
		}
		UINT const LayoutCharCode = MapVirtualKeyExW(LayoutVirtualKey, MAPVK_VK_TO_CHAR, ActiveLayout) & 0xFFFF;
		return FInputKeyManager::Get().GetKeyFromCodes(LayoutVirtualKey, LayoutCharCode);
	}
}
#endif

void AGeoPlayerController::SeedKeyBindingsForKeyboardLayout()
{
#if PLATFORM_WINDOWS
	UEnhancedInputLocalPlayerSubsystem* InputSystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	UEnhancedInputUserSettings* UserSettings = InputSystem ? InputSystem->GetUserSettings() : nullptr;
	UEnhancedPlayerMappableKeyProfile* Profile = UserSettings ? UserSettings->GetActiveKeyProfile() : nullptr;
	if (!ensureMsgf(Profile, TEXT("AGeoPlayerController: no active Enhanced Input key profile to seed")))
	{
		return;
	}

	for (TPair<FName, FKeyMappingRow> const& RowPair : Profile->GetPlayerMappingRows())
	{
		for (FPlayerKeyMapping const& Mapping : RowPair.Value.Mappings)
		{
			if (Mapping.IsCustomized())
			{
				return;
			}
		}
	}

	HKL const ActiveLayout = GetKeyboardLayout(0);
	HKL const QwertyLayout = LoadKeyboardLayoutW(L"00000409", KLF_NOTELLSHELL);
	if (!QwertyLayout || QwertyLayout == ActiveLayout)
	{
		return;
	}

	bool bChanged = false;
	for (TPair<FName, FKeyMappingRow> const& RowPair : Profile->GetPlayerMappingRows())
	{
		for (FPlayerKeyMapping const& Mapping : RowPair.Value.Mappings)
		{
			FKey const LayoutKey =
				GetActiveLayoutKeyAtQwertyPosition(Mapping.GetDefaultKey(), QwertyLayout, ActiveLayout);
			if (!LayoutKey.IsValid() || LayoutKey == Mapping.GetDefaultKey())
			{
				continue;
			}

			FMapPlayerKeyArgs Args;
			Args.MappingName = RowPair.Key;
			Args.Slot = Mapping.GetSlot();
			Args.NewKey = LayoutKey;
			FGameplayTagContainer FailureReason;
			UserSettings->MapPlayerKey(Args, FailureReason);
			if (ensureMsgf(FailureReason.IsEmpty(), TEXT("AGeoPlayerController: failed to seed key %s for %s (%s)"),
						   *LayoutKey.ToString(), *RowPair.Key.ToString(), *FailureReason.ToString()))
			{
				UE_LOG(LogTemp, Log, TEXT("AGeoPlayerController: seeded %s -> %s for keyboard layout"),
					   *Mapping.GetDefaultKey().ToString(), *LayoutKey.ToString());
				bChanged = true;
			}
		}
	}
	if (bChanged)
	{
		UserSettings->SaveSettings();
	}
#endif
}

void AGeoPlayerController::SetMenuInputMappingActive(bool bMenuActive)
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	UEnhancedInputLocalPlayerSubsystem* InputSystem =
		LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!InputSystem || !ensureMsgf(!InputMapping.IsNull() && !MenuInputMapping.IsNull(),
									TEXT("AGeoPlayerController: InputMapping or MenuInputMapping is not set")))
	{
		return;
	}
	// bNotifyUserSettings registers the context with UEnhancedInputUserSettings so the key-rebinding
	// screen can enumerate its player-mappable rows and saved remaps get applied on rebuild.
	FModifyContextOptions Options;
	Options.bNotifyUserSettings = true;
	InputSystem->RemoveMappingContext(
		bMenuActive ? InputMapping.LoadSynchronous() : MenuInputMapping.LoadSynchronous(), Options);
	InputSystem->AddMappingContext(bMenuActive ? MenuInputMapping.LoadSynchronous() : InputMapping.LoadSynchronous(),
								   0, Options);
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

bool AGeoPlayerController::IsPauseMenuOpen() const
{
	return PauseMenuWidget && PauseMenuWidget->IsInViewport();
}

void AGeoPlayerController::TogglePauseMenu()
{
	if (IsPauseMenuOpen())
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
	SetMenuInputMappingActive(true);
	FlushPressedKeys();
	SetInputMode(FInputModeGameAndUI()
					 .SetHideCursorDuringCapture(false)
					 .SetWidgetToFocus(PauseMenuWidget->TakeWidget()));
}

void AGeoPlayerController::ClosePauseMenu()
{
	if (PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
	}
	SetMenuInputMappingActive(false);
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
