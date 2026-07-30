// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GameClasses/GeoGameViewportClient.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Settings/GeoGameUserSettings.h"

void UGeoGameViewportClient::Init(FWorldContext& WorldContext, UGameInstance* OwningGameInstance,
								  bool bCreateNewAudioDevice)
{
	Super::Init(WorldContext, OwningGameInstance, bCreateNewAudioDevice);
	SetForceDisableSplitscreen(true);
	ApplyCouchCoopSetting();
}

void UGeoGameViewportClient::ReceivedFocus(FViewport* InViewport)
{
	Super::ReceivedFocus(InViewport);

	// Every platform user gets its own Slate user with its own focus, and one that has no focused widget turns its
	// gamepad into menu navigation instead of game input. The engine only ever focuses the Slate user matching a local
	// player's *index*, which stops being the right user under DeviceMappingPolicy=2 and never covers a pad that has
	// not joined yet. Focusing all of them also arms Slate's "last all users" focus widget, which
	// FSlateApplication::RegisterNewUser copies onto every Slate user created afterwards — that inheritance is what
	// lets a pad's first press reach InputKey.
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

bool UGeoGameViewportClient::InputKey(FInputKeyEventArgs const& EventArgs)
{
	// A gamepad owning no local player is dropped outright by the engine, so re-derive its owner before that happens:
	// this is also how a pad plugged in after launch, which the mapper gives a fresh platform user, reaches player 1.
	if (EventArgs.Key.IsGamepadKey() && !GEngine->GetLocalPlayerFromInputDevice(this, EventArgs.InputDevice))
	{
		ApplyCouchCoopSetting();
		if (EventArgs.Event == IE_Pressed && EventArgs.Key == EKeys::Gamepad_Special_Right
			&& TryCreateSecondPlayer(EventArgs.InputDevice))
		{
			return true;
		}
	}
	return Super::InputKey(EventArgs);
}

void UGeoGameViewportClient::ApplyCouchCoopSetting()
{
	IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
	FInputDeviceId const KeyboardDevice = DeviceMapper.GetDefaultInputDevice();
	bool const bSecondPlayer = UGeoGameUserSettings::Get()->UseFirstGamepadForSecondPlayer();
	if (bSecondPlayer && !SecondPlayerUser.IsValid())
	{
		SecondPlayerUser = DeviceMapper.AllocateNewUserId();
	}

	TArray<FInputDeviceId> Devices;
	DeviceMapper.GetAllConnectedInputDevices(Devices);

	// Whichever pad already holds SecondPlayerUser keeps it while it stays connected: re-deriving the owner from device
	// ids on every call would hand player 2's pad back to player 1 the moment another pad enumerated below it.
	FInputDeviceId HeldGamepad = INPUTDEVICEID_NONE;
	FInputDeviceId LowestGamepad = INPUTDEVICEID_NONE;
	for (FInputDeviceId const Device : Devices)
	{
		if (Device != KeyboardDevice && (!LowestGamepad.IsValid() || Device < LowestGamepad))
		{
			LowestGamepad = Device;
		}
		if (Device != KeyboardDevice && SecondPlayerUser.IsValid()
			&& DeviceMapper.GetUserForInputDevice(Device) == SecondPlayerUser)
		{
			HeldGamepad = Device;
		}
	}
	FInputDeviceId const SecondPlayerDevice = HeldGamepad.IsValid() ? HeldGamepad : LowestGamepad;

	FPlatformUserId const PrimaryUser = DeviceMapper.GetPrimaryPlatformUser();
	ULocalPlayer const* FirstPlayer = GameInstance ? GameInstance->GetLocalPlayerByIndex(0) : nullptr;
	UE_LOG(LogTemp, Warning, TEXT("ApplyCouchCoopSetting: toggle=%d SecondPlayerUser=%d Primary=%d Player0=%d Players=%d"),
		   bSecondPlayer, SecondPlayerUser.GetInternalId(), PrimaryUser.GetInternalId(),
		   FirstPlayer ? FirstPlayer->GetPlatformUserId().GetInternalId() : -1,
		   GameInstance ? GameInstance->GetNumLocalPlayers() : -1);

	for (FInputDeviceId const Device : Devices)
	{
		FPlatformUserId const CurrentUser = DeviceMapper.GetUserForInputDevice(Device);
		FPlatformUserId const TargetUser = bSecondPlayer && Device == SecondPlayerDevice ? SecondPlayerUser : PrimaryUser;
		bool const bChanged = Device != KeyboardDevice && CurrentUser != TargetUser
							  && DeviceMapper.Internal_ChangeInputDeviceUserMapping(Device, TargetUser, CurrentUser);
		UE_LOG(LogTemp, Warning, TEXT("  device %d: user %d -> %d (keyboard=%d changed=%d, now %d)"), Device.GetId(),
			   CurrentUser.GetInternalId(), TargetUser.GetInternalId(), Device == KeyboardDevice, bChanged,
			   DeviceMapper.GetUserForInputDevice(Device).GetInternalId());
	}

	if (!bSecondPlayer && GameInstance && GameInstance->GetNumLocalPlayers() > 1)
	{
		GameInstance->RemoveLocalPlayer(GameInstance->GetLocalPlayerByIndex(1));
	}
}

bool UGeoGameViewportClient::TryCreateSecondPlayer(FInputDeviceId InputDevice)
{
	if (!GameInstance || IPlatformInputDeviceMapper::Get().GetUserForInputDevice(InputDevice) != SecondPlayerUser)
	{
		return false;
	}

	FString Error;
	if (!GameInstance->CreateLocalPlayer(SecondPlayerUser, Error, true))
	{
		UE_LOG(LogTemp, Warning, TEXT("UGeoGameViewportClient: no second local player added for input device %d (%s)"),
			   InputDevice.GetId(), *Error);
		return false;
	}
	return true;
}
