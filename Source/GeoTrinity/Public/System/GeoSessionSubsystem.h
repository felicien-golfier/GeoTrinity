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
	/**
	 * Hosts a listen server: the local player travels to MapPackagePath as the authority and also plays. Joiners reach
	 * it over direct IP via JoinByAddress. No separate server process is launched. The travel is absolute: nothing of
	 * the current URL (portal, options) carries over.
	 *
	 * @param MapPackagePath  Package path of the gameplay map to host (e.g. /Game/Maps/DraftMap).
	 * @param Portal          PlayerStartTag of the start the host spawns at; empty = any start.
	 */
	UFUNCTION(BlueprintCallable, Category = "GeoTrinity|Session")
	void HostListen(FString const& MapPackagePath, FString const& Portal = TEXT(""));

	/** Travel URL of HostListen: <Map>?listen, then #Portal when Portal is not empty. */
	static FString MakeListenUrl(FString const& MapPackagePath, FString const& Portal);

	/** Travels the local player to a host at Address (IP, optional :port). */
	UFUNCTION(BlueprintCallable, Category = "GeoTrinity|Session")
	void JoinByAddress(FString const& Address);

	/** Best-effort local IPv4 to read out to friends; "127.0.0.1" on failure. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GeoTrinity|Session")
	FString GetLocalIPv4() const;
};
