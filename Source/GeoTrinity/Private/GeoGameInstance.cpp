// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "GeoGameInstance.h"

#include "GenericTeamAgentInterface.h"
#include "Tool/UGameplayLibrary.h"


// ---------------------------------------------------------------------------------------------------------------------
// TEAM
// ---------------------------------------------------------------------------------------------------------------------
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
