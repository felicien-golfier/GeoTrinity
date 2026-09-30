// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "GameClasses/GeoGameInstance.h"

#include "Engine/Engine.h"
#include "GeoTrinity/GeoTrinity.h"
#include "GenericTeamAgentInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/NetDriver.h"
#include "Kismet/KismetSystemLibrary.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "TimerManager.h"
#include "Tool/Team.h"

THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END


// ---------------------------------------------------------------------------------------------------------------------
// TEAM
// ---------------------------------------------------------------------------------------------------------------------
/**
 * Global team attitude resolver registered with the engine on game startup.
 * Registered once in Init() via FGenericTeamId::SetAttitudeSolver — any two actors that implement
 * IGenericTeamAgentInterface will route through here for team-based queries (damage filtering, aura targeting, etc.).
 */
// The one session this game ever creates. Sessions are matched by name equality, so the name is spelled once here
// rather than at each call.
static FName const GeoSessionName(TEXT("GameSession"));

static ETeamAttitude::Type GeoAttitudeSolver(FGenericTeamId A, FGenericTeamId B)
{
	ETeam const TeamA = static_cast<ETeam>(A.GetId());
	ETeam const TeamB = static_cast<ETeam>(B.GetId());

	if (TeamA == TeamB)
	{
		return ETeamAttitude::Friendly;
	}

	// Consider NoTeam as neutral. Everyone is Neutral against NoTeam.
	if (TeamA == ETeam::Neutral || A == FGenericTeamId::NoTeam || TeamB == ETeam::Neutral
		|| B == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}

	return ETeamAttitude::Hostile;
}


void UGeoGameInstance::Init()
{
	Super::Init();

	FGenericTeamId::SetAttitudeSolver(&GeoAttitudeSolver);

	GEngine->OnNetworkFailure().AddUObject(this, &UGeoGameInstance::OnNetworkFailure);
	GEngine->OnTravelFailure().AddUObject(this, &UGeoGameInstance::OnTravelFailure);
}

// ---------------------------------------------------------------------------------------------------------------------
// SESSION
// ---------------------------------------------------------------------------------------------------------------------
FName const UGeoGameInstance::GameSessionKey(TEXT("GEOGAME"));

// The one on-screen slot every session debug message shares, so each replaces the last.
static int32 const SessionDebugMessageKey = 0x6E0;
static float const SessionDebugMessageSeconds = 15.f;

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::CreateSession(FOnlineSessionSettings SessionSettings, FString const& MapPackageName)
{
	if (!ensureMsgf(GetSessionInterface().IsValid(), TEXT("%hs: no online session interface"), __FUNCTION__))
	{
		return;
	}

	SessionSettings.Set(GameSessionKey, 1, EOnlineDataAdvertisementType::ViaOnlineService);
	GetTimerManager().ClearTimer(OwnLobbyCheckTimer);
	GEngine->RemoveOnScreenDebugMessage(SessionDebugMessageKey);
	DestroySessionThen(
		[this, SessionSettings, MapPackageName]()
		{
			PendingMapURL = MapPackageName;
			IOnlineSessionPtr Sessions = GetSessionInterface();
			CreateSessionDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
				FOnCreateSessionCompleteDelegate::CreateUObject(this, &UGeoGameInstance::OnCreateSessionComplete));
			Sessions->CreateSession(0, GeoSessionName, SessionSettings);
		});
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	GetSessionInterface()->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);

	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("%hs: session created, opening %s"), __FUNCTION__, *PendingMapURL);
		UGameplayStatics::OpenLevel(this, FName(*PendingMapURL), true, TEXT("listen"));
		// Lets Steam index the new lobby before asking for the list.
		GetTimerManager().SetTimer(OwnLobbyCheckTimer, this, &UGeoGameInstance::CheckOwnLobbyListed, 5.f, false);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%hs: failed to create session '%s'"), __FUNCTION__, *SessionName.ToString());
		ReportSessionError(FString::Printf(TEXT("Could not create the server: %s refused to create the session "
												"(details in the log, LogOnlineSession)."),
										   *Online::GetSubsystem(GetWorld())->GetSubsystemName().ToString()));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::CheckOwnLobbyListed()
{
	FName const SubsystemName = Online::GetSubsystem(GetWorld())->GetSubsystemName();
	if (SubsystemName != STEAM_SUBSYSTEM)
	{
		ShowSessionDebugMessage(
			FString::Printf(TEXT("SERVER NOT LISTED: hosted on the %s online subsystem, not Steam. Steam failed to start "
								 "(Steam not running, or a second game instance on this PC)."),
							*SubsystemName.ToString()),
			FColor::Red);
	}
	else
	{
		ISteamMatchmaking* const Matchmaking = SteamMatchmaking();
		Matchmaking->AddRequestLobbyListResultCountFilter(100);
		Matchmaking->AddRequestLobbyListDistanceFilter(k_ELobbyDistanceFilterDefault);
		// The Steam subsystem suffixes an int32 session key with "_i" in the lobby data.
		Matchmaking->AddRequestLobbyListNumericalFilter(TCHAR_TO_UTF8(*(GameSessionKey.ToString() + TEXT("_i"))), 1,
														k_ELobbyComparisonEqual);
		OwnLobbyListCall = Matchmaking->RequestLobbyList();
		ShowSessionDebugMessage(TEXT("Checking whether this server is in the server list..."), FColor::Yellow);
		GetTimerManager().SetTimer(OwnLobbyCheckTimer, this, &UGeoGameInstance::PollOwnLobbyListed, 0.25f, true);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::PollOwnLobbyListed()
{
	bool bCallFailed = false;
	if (SteamUtils()->IsAPICallCompleted(OwnLobbyListCall, &bCallFailed))
	{
		GetTimerManager().ClearTimer(OwnLobbyCheckTimer);

		LobbyMatchList_t LobbyList;
		bool bResultFailed = false;
		bool const bGotResult = SteamUtils()->GetAPICallResult(OwnLobbyListCall, &LobbyList, sizeof(LobbyList),
															   LobbyMatchList_t::k_iCallback, &bResultFailed);
		if (!bGotResult || bCallFailed || bResultFailed)
		{
			ShowSessionDebugMessage(TEXT("Server list check failed: Steam did not answer the lobby query."), FColor::Red);
		}
		else
		{
			CSteamID const LocalSteamId = SteamUser()->GetSteamID();
			bool bOwnLobbyListed = false;
			for (int32 LobbyIndex = 0; LobbyIndex < static_cast<int32>(LobbyList.m_nLobbiesMatching); ++LobbyIndex)
			{
				CSteamID const LobbyId = SteamMatchmaking()->GetLobbyByIndex(LobbyIndex);
				bOwnLobbyListed |= SteamMatchmaking()->GetLobbyOwner(LobbyId) == LocalSteamId;
			}

			FString const Summary = FString::Printf(
				TEXT("(%u GeoTrinity lobbies listed near you, build id %d: a player only sees servers with the same "
					 "build id)"),
				LobbyList.m_nLobbiesMatching, GetBuildUniqueId());
			ShowSessionDebugMessage(
				(bOwnLobbyListed ? TEXT("SERVER LISTED: Steam lists your server. ")
								 : TEXT("SERVER NOT LISTED: Steam's lobby list does not contain your server. "))
					+ Summary,
				bOwnLobbyListed ? FColor::Green : FColor::Red);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::ShowSessionDebugMessage(FString const& Message, FColor const Color)
{
	GEngine->AddOnScreenDebugMessage(SessionDebugMessageKey, SessionDebugMessageSeconds, Color, Message);
	GetTimerManager().SetTimer(
		SessionDebugMessageTimer, []() { GEngine->RemoveOnScreenDebugMessage(SessionDebugMessageKey); },
		SessionDebugMessageSeconds, false);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::JoinFoundSession(FOnlineSessionSearchResult const& SearchResult)
{
	if (!ensureMsgf(GetSessionInterface().IsValid(), TEXT("%hs: no online session interface"), __FUNCTION__))
	{
		return;
	}

	DestroySessionThen(
		[this, SearchResult]()
		{
			IOnlineSessionPtr Sessions = GetSessionInterface();
			JoinSessionDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
				FOnJoinSessionCompleteDelegate::CreateUObject(this, &UGeoGameInstance::OnJoinSessionComplete));
			Sessions->JoinSession(0, GeoSessionName, SearchResult);
		});
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);

	FString ConnectString;
	if (Result == EOnJoinSessionCompleteResult::Success
		&& Sessions->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LogTemp, Log, TEXT("%hs: session joined, traveling to %s"), __FUNCTION__, *ConnectString);
		GetFirstLocalPlayerController()->ClientTravel(ConnectString, TRAVEL_Absolute);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%hs: failed to join session '%s' (%s)"), __FUNCTION__, *SessionName.ToString(),
			   LexToString(Result));
		ReportSessionError(FString::Printf(TEXT("Could not join the server: %s."), LexToString(Result)));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::OnNetworkFailure(UWorld* /*World*/, UNetDriver* NetDriver, ENetworkFailure::Type FailureType,
										FString const& ErrorString)
{
	ReportSessionError(
		FString::Printf(TEXT("Network failure (%s): %s"), ENetworkFailure::ToString(FailureType), *ErrorString));
	if (NetDriver && NetDriver->GetNetMode() == NM_Client)
	{
		DestroySessionThen([]() {});
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::OnTravelFailure(UWorld* /*World*/, ETravelFailure::Type FailureType, FString const& ErrorString)
{
	ReportSessionError(
		FString::Printf(TEXT("Travel failure (%s): %s"), ETravelFailure::ToString(FailureType), *ErrorString));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::ReportSessionError(FString const& Message)
{
	UE_LOG(LogTemp, Error, TEXT("%hs: %s"), __FUNCTION__, *Message);
	PendingSessionError += PendingSessionError.IsEmpty() ? Message : TEXT("\n") + Message;
}

// ---------------------------------------------------------------------------------------------------------------------
FString UGeoGameInstance::TakeSessionError()
{
	return MoveTemp(PendingSessionError);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::LeaveSessionAndReturnToMenu()
{
	if (!ensureMsgf(!MainMenuMap.IsNull(), TEXT("%hs: MainMenuMap is not set"), __FUNCTION__))
	{
		return;
	}

	DestroySessionThen(
		[this]()
		{
			UGameplayStatics::OpenLevel(this, FName(*MainMenuMap.ToSoftObjectPath().GetLongPackageName()));
		});
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::QuitGame()
{
	DestroySessionThen(
		[this]()
		{
			UKismetSystemLibrary::QuitGame(this, GetFirstLocalPlayerController(), EQuitPreference::Quit, true);
		});
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::DestroySessionThen(TFunction<void()> OnDone)
{
	if (AfterSessionDestroyed)
	{
		UE_LOG(LogTemp, Warning, TEXT("%hs: a session teardown is already pending, request dropped"), __FUNCTION__);
		return;
	}

	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || Sessions->GetSessionState(GeoSessionName) == EOnlineSessionState::NoSession)
	{
		// Direct-IP/no-Steam session, or none at all: nothing to tear down.
		OnDone();
	}
	else
	{
		AfterSessionDestroyed = MoveTemp(OnDone);
		DestroySessionDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &UGeoGameInstance::OnDestroySessionComplete));
		Sessions->DestroySession(GeoSessionName);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::OnDestroySessionComplete(FName /*SessionName*/, bool /*bWasSuccessful*/)
{
	GetSessionInterface()->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);

	// Emptied before the call, so OnDone can itself start another teardown.
	TFunction<void()> const OnDone = MoveTemp(AfterSessionDestroyed);
	AfterSessionDestroyed = nullptr;
	OnDone();
}

// ---------------------------------------------------------------------------------------------------------------------
IOnlineSessionPtr UGeoGameInstance::GetSessionInterface() const
{
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	return OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
}
