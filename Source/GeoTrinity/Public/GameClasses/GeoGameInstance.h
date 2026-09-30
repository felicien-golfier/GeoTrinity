// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "GeoGameInstance.generated.h"

class FOnlineSessionSearchResult;
class FOnlineSessionSettings;
class UNetDriver;
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
	/** Session key every GeoTrinity session carries and the server browser filters on: under the Spacewar test app
	 * id (480) a lobby search otherwise lists every game's lobbies. */
	static FName const GameSessionKey;

	/**
	 * Hosts a Steam session with SessionSettings, then opens MapPackageName as a listen server. A session left over
	 * from an earlier game is destroyed first, or the create would fail on the already-used session name.
	 */
	void CreateSession(FOnlineSessionSettings SessionSettings, FString const& MapPackageName);

	/** Joins SearchResult's session and travels to its host, destroying a leftover session first like CreateSession. */
	void JoinFoundSession(FOnlineSessionSearchResult const& SearchResult);

	/**
	 * Leaves the current game session and returns to the main menu. Destroys the Steam online session first if one
	 * exists (travel happens in the destroy-completion callback); otherwise (direct-IP/no-Steam session, or no
	 * session at all) opens the main menu map immediately.
	 */
	UFUNCTION(BlueprintCallable, Category = "GeoOnline")
	void LeaveSessionAndReturnToMenu();

	/**
	 * Quits the game process. Destroys the Steam online session first if one exists (exit happens in the
	 * destroy-completion callback) — quitting with a live session leaves the process hanging on Steam shutdown.
	 */
	UFUNCTION(BlueprintCallable, Category = "GeoOnline")
	void QuitGame();

	/**
	 * Takes, and clears, the reason the last create, join or connection failed, for the menu's error popup. Empty when
	 * nothing failed since the last take. Held rather than broadcast: a failed connection lands on a freshly loaded
	 * main menu, which does not exist yet when the failure is reported.
	 */
	FString TakeSessionError();

	/**
	 * Debug: shows Message on screen for 15 real seconds, replacing the previous session message. Removed by a game
	 * instance timer rather than the message's own duration, which only counts down with the world's frame time and
	 * left results standing on the menu, where a stale "listed" read as the answer for the next server created.
	 */
	void ShowSessionDebugMessage(FString const& Message, FColor Color);

	/** Map to return to when leaving a session via LeaveSessionAndReturnToMenu. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoOnline")
	TSoftObjectPtr<UWorld> MainMenuMap;

private:
	/** Online **/
	/** Session interface of the active online subsystem, or invalid if the subsystem is unavailable. */
	IOnlineSessionPtr GetSessionInterface() const;

	/** Tears the online session down and then runs OnDone — immediately when there is no session to destroy. The one
	 * description of "end the session, then act", shared by hosting, joining, returning to the menu and quitting. */
	void DestroySessionThen(TFunction<void()> OnDone);

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	/** A client dropped by its host (or failing to connect) is sent back to the menu by the engine, which leaves its
	 * session behind: still a member of the host's lobby, it would keep a dead lobby listed. */
	void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType,
						  FString const& ErrorString);
	void OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, FString const& ErrorString);

	/** Queues Message for the menu's error popup (see TakeSessionError), after any message not yet taken. */
	void ReportSessionError(FString const& Message);

	/**
	 * Debug, on the host right after its session is created: shows on screen whether the server browser would list
	 * it. The browser can't answer that for its own host — the Steam subsystem drops every lobby the searcher is in
	 * from its results — so this sends Steam the browser's lobby query itself and PollOwnLobbyListed looks for this
	 * host's lobby among the matches.
	 */
	void CheckOwnLobbyListed();
	void PollOwnLobbyListed();

	FString PendingMapURL;
	FString PendingSessionError;
	FTimerHandle OwnLobbyCheckTimer;
	FTimerHandle SessionDebugMessageTimer;
	/** Steam's SteamAPICall_t for the lobby list CheckOwnLobbyListed requested. */
	uint64 OwnLobbyListCall = 0;
	FDelegateHandle CreateSessionDelegateHandle;
	FDelegateHandle JoinSessionDelegateHandle;
	FDelegateHandle DestroySessionDelegateHandle;
	/**
	 * What runs once the pending DestroySession completes; set exactly while one is pending, so a second
	 * DestroySessionThen() in that window is dropped rather than stacking a second completion. Held here and never
	 * captured by the completion delegate: that delegate clears itself, which frees its captures mid-call.
	 */
	TFunction<void()> AfterSessionDestroyed;
};
