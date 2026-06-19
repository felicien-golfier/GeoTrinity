// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "System/GeoSessionSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "IPAddress.h"
#include "SocketSubsystem.h"

void UGeoSessionSubsystem::HostListen(FString const& MapPackagePath)
{
	if (!ensureMsgf(!MapPackagePath.IsEmpty(), TEXT("HostListen: empty MapPackagePath")))
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!ensureMsgf(World, TEXT("HostListen: no World")))
	{
		return;
	}
	World->ServerTravel(MapPackagePath + TEXT("?listen"));
}

void UGeoSessionSubsystem::JoinByAddress(FString const& Address)
{
	FString const Trimmed = Address.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinByAddress: empty address"));
		return;
	}
	APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
	if (!ensureMsgf(PlayerController, TEXT("JoinByAddress: no local PlayerController")))
	{
		return;
	}
	PlayerController->ClientTravel(Trimmed, TRAVEL_Absolute);
}

FString UGeoSessionSubsystem::GetLocalIPv4() const
{
	ISocketSubsystem* Sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!Sockets)
	{
		return TEXT("127.0.0.1");
	}
	bool bCanBind = false;
	TSharedRef<FInternetAddr> Addr = Sockets->GetLocalHostAddr(*GLog, bCanBind);
	return Addr->IsValid() ? Addr->ToString(false /*bAppendPort*/) : TEXT("127.0.0.1");
}
