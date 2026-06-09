// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "GameClasses/GeoGameInstance.h"

#include "GenericTeamAgentInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
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
	
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if (!ensureMsgf(OnlineSubsystem, TEXT("UGeoCreateServerWidget: Online subsystem not available")))
	{
		return;
	}
	IOnlineSessionPtr Sessions = OnlineSubsystem->GetSessionInterface();
	if (!ensureMsgf(Sessions.IsValid(), TEXT("UGeoCreateServerWidget: Session interface not valid")))
	{
		return;
	}
	
	CreateSessionDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UGeoGameInstance::OnCreateSessionComplete)
	);

	// Map
	if (MapToGoTo.IsEmpty())
	{
		if (!ensureMsgf(!DefaultMap.IsNull(), TEXT("GeoGameInstance: No map URL — DefaultMap is not set on the Blueprint subclass")))
		{
			return;
		}
		PendingMapURL = DefaultMap.ToSoftObjectPath().GetLongPackageName();
	}
	else
	{
		PendingMapURL = MapToGoTo;
	}
	
	Sessions->CreateSession(0, FName(TEXT("GameSession")), SessionSettings);
	
	
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, 
		FString::Printf(TEXT("[%u] CreateAdvancedSession done, waiting for callback"), FNetworkVersion::GetLocalNetworkVersion()));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("OnCreateSessionComplete complete")));

	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
		}
	}

	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("UGeoCreateServerWidget: Failed to create session '%s'"), *SessionName.ToString());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("UGeoCreateServerWidget: Session created, traveling to %s"), *PendingMapURL);
	GetWorld()->ServerTravel(PendingMapURL + TEXT("?listen"));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::JoinAdvancedSession(const FOnlineSessionSearchResult& SearchResult)
{
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if (!ensureMsgf(OnlineSubsystem, TEXT("GeoGameInstance::JoinAdvancedSession: Online subsystem not available")))
	{
		return;
	}
	IOnlineSessionPtr Sessions = OnlineSubsystem->GetSessionInterface();
	if (!ensureMsgf(Sessions.IsValid(), TEXT("GeoGameInstance::JoinAdvancedSession: Session interface not valid")))
	{
		return;
	}

	JoinSessionDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UGeoGameInstance::OnJoinSessionComplete)
	);

	Sessions->JoinSession(0, FName(TEXT("GameSession")), SearchResult);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
		}
	}

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Error, TEXT("GeoGameInstance: Failed to join session '%s': %s"), *SessionName.ToString(), LexToString(Result));
		return;
	}

	if (!OnlineSub)
	{
		UE_LOG(LogTemp, Error, TEXT("GeoGameInstance::OnJoinSessionComplete: Online subsystem not available for travel"));
		return;
	}

	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (!Sessions.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("GeoGameInstance::OnJoinSessionComplete: Session interface not valid for travel"));
		return;
	}

	FString ConnectString;
	if (!Sessions->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LogTemp, Error, TEXT("GeoGameInstance::OnJoinSessionComplete: Failed to get connect string for '%s'"), *SessionName.ToString());
		return;
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("GeoGameInstance::OnJoinSessionComplete: No player controller available"));
		return;
	}

	PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
}
