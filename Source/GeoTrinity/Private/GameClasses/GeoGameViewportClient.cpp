// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GameClasses/GeoGameViewportClient.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Settings/GeoGameUserSettings.h"
#include "Widgets/SViewport.h"

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
	// player's index, which never covers a pad that has not joined yet. Focusing all of them also arms Slate's
	// "last all users" focus widget, which FSlateApplication::RegisterNewUser copies onto every Slate user created
	// afterwards — that inheritance is what lets a pad's first press reach InputKey.
	// This client's own viewport, never SetAllUserFocusToGameViewport: that one targets Slate's single global game
	// viewport widget, so with several client windows every window pulls focus into whichever registered last, and the
	// window losing it hands it straight back through its own input-mode focus reply.
	TSharedPtr<SViewport> const ViewportWidget = GetGameViewportWidget();
	if (FSlateApplication::IsInitialized() && ViewportWidget)
	{
		FSlateApplication::Get().SetAllUserFocus(ViewportWidget);
	}
}

bool UGeoGameViewportClient::InputKey(FInputKeyEventArgs const& EventArgs)
{
	// A gamepad owning no local player is dropped outright by the engine, so re-derive its owner before that happens:
	// this is also how a pad plugged in after launch, which the mapper gives a fresh platform user, reaches its player.
	if (EventArgs.Key.IsGamepadKey() && !GEngine->GetLocalPlayerFromInputDevice(this, EventArgs.InputDevice))
	{
		ApplyCouchCoopSetting();
		if (EventArgs.Event == IE_Pressed && EventArgs.Key == EKeys::Gamepad_Special_Right
			&& !GEngine->GetLocalPlayerFromInputDevice(this, EventArgs.InputDevice))
		{
			FString Error;
			FPlatformUserId const User = IPlatformInputDeviceMapper::Get().GetUserForInputDevice(EventArgs.InputDevice);
			if (!GameInstance->CreateLocalPlayer(User, Error, true))
			{
				UE_LOG(LogTemp, Warning, TEXT("UGeoGameViewportClient: gamepad %d could not join (%s)"),
					   EventArgs.InputDevice.GetId(), *Error);
			}
			return true;
		}
	}
	return Super::InputKey(EventArgs);
}

void UGeoGameViewportClient::ApplyCouchCoopSetting()
{
	IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
	TArray<FInputDeviceId> Gamepads;
	DeviceMapper.GetAllConnectedInputDevices(Gamepads);
	Gamepads.Remove(DeviceMapper.GetDefaultInputDevice());
	Gamepads.Sort();

	// Character N is local player N, which holds platform user N: player 0 is created on the keyboard's primary user,
	// and every gamepad joins with the id handed to it here. So the setting is only where the gamepads start.
	int32 const FirstCharacter = UGeoGameUserSettings::Get()->UseFirstGamepadForSecondPlayer() ? 1 : 0;

	// A gamepad already playing a character keeps it, so a controller coming back lands on the character its old
	// device left behind instead of pushing everyone along. Changing the setting is the one thing that re-seats the
	// whole row, since every gamepad then has to shift a character up or down.
	TArray<int32> HeldCharacters;
	if (FirstCharacter == AppliedFirstCharacter)
	{
		for (int32 Index = Gamepads.Num() - 1; Index >= 0; --Index)
		{
			int32 const Character = DeviceMapper.GetUserForInputDevice(Gamepads[Index]).GetInternalId();
			if (Character >= FirstCharacter && GEngine->GetLocalPlayerFromInputDevice(this, Gamepads[Index]))
			{
				HeldCharacters.Add(Character);
				Gamepads.RemoveAt(Index);
			}
		}
	}
	AppliedFirstCharacter = FirstCharacter;

	int32 NextCharacter = FirstCharacter;
	for (FInputDeviceId const Gamepad : Gamepads)
	{
		while (HeldCharacters.Contains(NextCharacter))
		{
			++NextCharacter;
		}
		DeviceMapper.Internal_ChangeInputDeviceUserMapping(Gamepad, FPlatformUserId::CreateFromInternalId(NextCharacter),
														   DeviceMapper.GetUserForInputDevice(Gamepad));
		HeldCharacters.Add(NextCharacter);
	}

	for (int32 Index = GameInstance->GetNumLocalPlayers() - 1; Index > 0; --Index)
	{
		ULocalPlayer* const Player = GameInstance->GetLocalPlayerByIndex(Index);
		if (!DeviceMapper.GetPrimaryInputDeviceForUser(Player->GetPlatformUserId()).IsValid())
		{
			GameInstance->RemoveLocalPlayer(Player);
		}
	}
}
