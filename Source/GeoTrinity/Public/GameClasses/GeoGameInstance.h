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
	/** Creates a Steam session with the given settings and travels to MapToGoTo when complete. */
	void CreateAdvancedSession(FOnlineSessionSettings const& SessionSettings, FString MapToGoTo = "");
	/** Delegate callback for session creation; travels to the pending map on success. */
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	/** Blueprint entry point for hosting: assembles SessionSettings from human-readable params and calls CreateAdvancedSession. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Online")
	void BP_CreateAdvancedSession(FString const& ServerName, int32 NbOfSlots, bool bUseLan);

	/** Joins an existing Steam session and travels to the host. */
	void JoinAdvancedSession(const FOnlineSessionSearchResult& SearchResult);
	/** Delegate callback for session join; performs client travel on success. */
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	/** Blueprint entry point for joining: resolves the search result from Blueprint and calls JoinAdvancedSession. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Online")
	void BP_JoinAdvancedSession(FBlueprintSessionResult const& SessionData);

	/**
	 * Leaves the current game session and returns to the main menu. Destroys the Steam online session first if one
	 * exists (travel happens in the destroy-completion callback); otherwise (direct-IP/no-Steam session, or no
	 * session at all) opens the main menu map immediately.
	 */
	UFUNCTION(BlueprintCallable, Category = "Online")
	void LeaveSessionAndReturnToMenu();

	/** Delegate callback for session destruction; opens the main menu map once the session is fully torn down. */
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	/**
	 * Quits the game process. Destroys the Steam online session first if one exists (exit happens in the
	 * destroy-completion callback) — quitting with a live session leaves the process hanging on Steam shutdown.
	 */
	UFUNCTION(BlueprintCallable, Category = "Online")
	void QuitGame();

	/** Delegate callback for session destruction during quit; requests engine exit once the session is torn down. */
	void OnDestroySessionForQuitComplete(FName SessionName, bool bWasSuccessful);

	/** Default map to travel to when creating a session without an explicit map URL */
	UPROPERTY(EditDefaultsOnly, Category = "Online")
	TSoftObjectPtr<UWorld> DefaultMap;

	/** Map to return to when leaving a session via LeaveSessionAndReturnToMenu. */
	UPROPERTY(EditDefaultsOnly, Category = "Online")
	TSoftObjectPtr<UWorld> MainMenuMap;

private:
	/** Online **/
	/** Session interface of the active online subsystem, or invalid if the subsystem is unavailable. */
	IOnlineSessionPtr GetSessionInterface() const;

	FString PendingMapURL;
	FDelegateHandle CreateSessionDelegateHandle;
	FDelegateHandle JoinSessionDelegateHandle;
	FDelegateHandle DestroySessionDelegateHandle;
};
