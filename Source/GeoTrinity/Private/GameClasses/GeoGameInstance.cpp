// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "GameClasses/GeoGameInstance.h"

#include "GenericTeamAgentInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
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
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
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
