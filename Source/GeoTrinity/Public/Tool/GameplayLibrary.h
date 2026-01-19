#pragma once
#include "GenericTeamAgentInterface.h"

class GameplayLibrary
{
public:
	static bool GetTeamInterface(const AActor* Actor, const IGenericTeamAgentInterface*& OutInterface);
	static FColor GetColorForObject(const UObject* Object);
	static float GetServerTime(const UWorld* World, bool bUpdatedWithPing = false);
};
