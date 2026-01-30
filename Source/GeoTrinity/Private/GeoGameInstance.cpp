// Fill out your copyright notice in the Description page of Project Settings.


#include "GeoGameInstance.h"

#include "GenericTeamAgentInterface.h"


// ---------------------------------------------------------------------------------------------------------------------
// TEAM
// ---------------------------------------------------------------------------------------------------------------------
static ETeamAttitude::Type GeoAttitudeSolver(FGenericTeamId A, FGenericTeamId B)
{
	uint8 const IdA = A.GetId();
	uint8 const IdB = B.GetId();

	// Same team → friendly
	if (IdA == IdB)
	{
		return ETeamAttitude::Friendly;
	}

	// Keep it simple for now
	return ETeamAttitude::Hostile;
}


void UGeoGameInstance::Init()
{
	Super::Init();

	FGenericTeamId::SetAttitudeSolver(&GeoAttitudeSolver);
}
