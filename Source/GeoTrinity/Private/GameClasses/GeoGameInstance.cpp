// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "GameClasses/GeoGameInstance.h"

#include "FindSessionsCallbackProxy.h"
#include "GeoTrinity/GeoTrinity.h"
#include "GenericTeamAgentInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/GameplayStatics.h"
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
}

// ---------------------------------------------------------------------------------------------------------------------
// ADVANCED SESSION
// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::CreateAdvancedSession(FOnlineSessionSettings const& SessionSettings, FString MapToGoTo/* = ""*/)
{
	// Advanced session plugin only wraps for blueprint with a latent task, so better do it directly when in CPP
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!ensureMsgf(Sessions.IsValid(), TEXT("%hs: no online session interface"), __FUNCTION__))
	{
		return;
	}

	if (MapToGoTo.IsEmpty()
		&& !ensureMsgf(!DefaultMap.IsNull(),
					   TEXT("%hs: no map URL — DefaultMap is not set on the Blueprint subclass"), __FUNCTION__))
	{
		return;
	}
	PendingMapURL = MapToGoTo.IsEmpty() ? DefaultMap.ToSoftObjectPath().GetLongPackageName() : MapToGoTo;

	CreateSessionDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UGeoGameInstance::OnCreateSessionComplete));
	Sessions->CreateSession(0, GeoSessionName, SessionSettings);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
	}

	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("%hs: failed to create session '%s'"), __FUNCTION__, *SessionName.ToString());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("%hs: session created, traveling to %s"), __FUNCTION__, *PendingMapURL);
	GetWorld()->ServerTravel(PendingMapURL + TEXT("?listen"));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::JoinAdvancedSession(const FOnlineSessionSearchResult& SearchResult)
{
	FBlueprintSessionResult BPResult;
	BPResult.OnlineResult = SearchResult;
	BP_JoinAdvancedSession(BPResult);
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
	if (bDestroyingSession)
	{
		// Already tearing down (e.g. the leave/quit button was pressed twice before the async destroy completed):
		// registering a second lambda here would leave two of them on the session interface's shared multicast
		// delegate while DestroySessionDelegateHandle only ever remembers one, so the first completion would clear
		// the wrong, still-pending delegate mid-broadcast.
		return;
	}

	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || Sessions->GetSessionState(GeoSessionName) == EOnlineSessionState::NoSession)
	{
		// Direct-IP/no-Steam session, or none at all: nothing to tear down, so run the ending now.
		OnDone();
		return;
	}

	bDestroyingSession = true;
	DestroySessionDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateWeakLambda(
			this,
			[this, OnDone = MoveTemp(OnDone)](FName /*SessionName*/, bool /*bWasSuccessful*/)
			{
				if (IOnlineSessionPtr CompletedSessions = GetSessionInterface())
				{
					CompletedSessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
				}
				bDestroyingSession = false;
				OnDone();
			}));
	Sessions->DestroySession(GeoSessionName);
}

// ---------------------------------------------------------------------------------------------------------------------
IOnlineSessionPtr UGeoGameInstance::GetSessionInterface() const
{
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	return OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
}
