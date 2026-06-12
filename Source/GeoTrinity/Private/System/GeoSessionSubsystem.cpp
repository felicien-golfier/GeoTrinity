// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "System/GeoSessionSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformProcess.h"
#include "IPAddress.h"
#include "Misc/Paths.h"
#include "SocketSubsystem.h"

void UGeoSessionSubsystem::Deinitialize()
{
	ShutdownServerProcess();
	Super::Deinitialize();
}

void UGeoSessionSubsystem::HostDedicated(FString const& MapPackagePath)
{
	if (!ensureMsgf(!MapPackagePath.IsEmpty(), TEXT("HostDedicated: empty MapPackagePath")))
	{
		return;
	}
	if (!LaunchServerProcess(MapPackagePath))
	{
		return;
	}

	// Join the server we just launched as a pure client. The server needs a moment to bind its socket, so connect to
	// the loopback address — the engine retries the pending connection until the server accepts, then travels in.
	JoinByAddress(FString::Printf(TEXT("127.0.0.1:%u"), ServerPort));
}

bool UGeoSessionSubsystem::LaunchServerProcess(FString const& MapPackagePath)
{
	ShutdownServerProcess(); // never leave a previous server orphaned

	// In the editor there is no packaged GeoTrinityServer.exe — run the gameplay map headless with the editor cmd
	// binary instead. A packaged-build branch (locating GeoTrinityServer.exe) can be added when shipping.
	if (!ensureMsgf(GIsEditor, TEXT("HostDedicated: packaged dedicated-server launch not implemented yet")))
	{
		return false;
	}

	FString const Executable = FPlatformProcess::ExecutablePath();
	FString const ProjectPath = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
	// -nosteam: the local server has no Steam context; force the IP net driver fallback instead of SteamSockets.
	FString const Args =
		FString::Printf(TEXT("\"%s\" %s -server -log -port=%u -nosteam"), *ProjectPath, *MapPackagePath, ServerPort);

	ServerProcessHandle =
		FPlatformProcess::CreateProc(*Executable, *Args, /*bLaunchDetached*/ true,
									 /*bLaunchHidden*/ false, /*bLaunchReallyHidden*/ false,
									 /*OutProcessID*/ nullptr, /*PriorityModifier*/ 0,
									 /*OptionalWorkingDirectory*/ nullptr, /*PipeWriteChild*/ nullptr);

	if (!ServerProcessHandle.IsValid())
	{
		ensureMsgf(false, TEXT("HostDedicated: failed to launch server process %s %s"), *Executable, *Args);
		return false;
	}
	return true;
}

void UGeoSessionSubsystem::ShutdownServerProcess()
{
	if (ServerProcessHandle.IsValid())
	{
		FPlatformProcess::TerminateProc(ServerProcessHandle, /*KillTree*/ true);
		FPlatformProcess::CloseProc(ServerProcessHandle);
	}
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
