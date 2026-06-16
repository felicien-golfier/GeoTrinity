// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "GeoGameInstance.generated.h"

struct FBlueprintSessionResult;
class FOnlineSessionSearchResult;
class FOnlineSessionSettings;
class UWorld;

/**
 * Custom game instance for GeoTrinity.
 * Performs one-time initialization of global systems (e.g. native gameplay tags) on game start.
 */
UCLASS()
class GEOTRINITY_API UGeoGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	/** Initializes native gameplay tags and other global systems. */
	virtual void Init() override;

	/** Online **/
	void CreateAdvancedSession(FOnlineSessionSettings const& SessionSettings, FString MapToGoTo = "");
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Online")
	void BP_CreateAdvancedSession(FString const& ServerName, int32 NbOfSlots, bool bUseLan);

	void JoinAdvancedSession(const FOnlineSessionSearchResult& SearchResult);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	UFUNCTION(BlueprintImplementableEvent, Category = "Online")
	void BP_JoinAdvancedSession(FBlueprintSessionResult const& SessionData);

	/** Default map to travel to when creating a session without an explicit map URL */
	UPROPERTY(EditDefaultsOnly, Category = "Online")
	TSoftObjectPtr<UWorld> DefaultMap;

private:
	/** Online **/
	FString PendingMapURL;
	FDelegateHandle CreateSessionDelegateHandle;
	FDelegateHandle JoinSessionDelegateHandle;
};
