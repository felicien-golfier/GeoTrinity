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
#include "Tool/Team.h"


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
}

// ---------------------------------------------------------------------------------------------------------------------
// SESSION
// ---------------------------------------------------------------------------------------------------------------------
FName const UGeoGameInstance::GameSessionKey(TEXT("GEOGAME"));

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::CreateSession(FOnlineSessionSettings SessionSettings, FString const& MapPackageName)
{
	if (!ensureMsgf(GetSessionInterface().IsValid(), TEXT("%hs: no online session interface"), __FUNCTION__))
	{
		return;
	}

	SessionSettings.Set(GameSessionKey, 1, EOnlineDataAdvertisementType::ViaOnlineService);
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
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%hs: failed to create session '%s'"), __FUNCTION__, *SessionName.ToString());
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::JoinSession(FOnlineSessionSearchResult const& SearchResult)
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
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::OnNetworkFailure(UWorld* /*World*/, UNetDriver* NetDriver, ENetworkFailure::Type /*FailureType*/,
										FString const& /*ErrorString*/)
{
	if (NetDriver && NetDriver->GetNetMode() == NM_Client)
	{
		DestroySessionThen([]() {});
	}
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
