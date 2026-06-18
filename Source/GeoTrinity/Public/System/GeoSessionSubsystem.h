// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "GeoSessionSubsystem.generated.h"

/**
 * GameInstance subsystem that owns the connect/host seam for direct-IP (no-Steam) multiplayer. Lives on the
 * GameInstance so it survives level travel. Steam sessions are handled separately by UGeoGameInstance; this subsystem
 * is the local/LAN path only.
 */
UCLASS()
class GEOTRINITY_API UGeoSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Terminates a server this subsystem launched, so quitting the client never leaves an orphan server running. */
	virtual void Deinitialize() override;

	/**
	 * Launches a dedicated server behind the local player, then joins it as a pure client.
	 *
	 * The local player is always a client (never the authority), so authority-gated UI/HUD init runs the same way it
	 * does for a remote joiner. In the editor the server is an UnrealEditor-Cmd.exe instance running MapPackagePath
	 * with -server; in a packaged build it is the GeoTrinityServer.exe shipped alongside the client. The launched
	 * process is tracked and killed in Deinitialize.
	 *
	 * @param MapPackagePath  Package path of the gameplay map the server should load (e.g. /Game/Maps/DraftMap).
	 */
	UFUNCTION(BlueprintCallable, Category = "GeoTrinity|Session")
	void HostDedicated(FString const& MapPackagePath);

	/** Travels the local player to a host at Address (IP, optional :port). */
	UFUNCTION(BlueprintCallable, Category = "GeoTrinity|Session")
	void JoinByAddress(FString const& Address);

	/** Best-effort local IPv4 to read out to friends; "127.0.0.1" on failure. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GeoTrinity|Session")
	FString GetLocalIPv4() const;

private:
	/** Launches the dedicated server process for MapPackagePath; returns false (and flags) if the process fails to start. */
	bool LaunchServerProcess(FString const& MapPackagePath);

	/** Terminates and closes the launched server process if one is running. */
	void ShutdownServerProcess();

	/** Direct-IP port the launched server listens on and the client connects to. */
	static constexpr uint16 ServerPort = 7777;

	/** Handle to the server process launched by HostDedicated, so Deinitialize can terminate it. */
	FProcHandle ServerProcessHandle;
};
