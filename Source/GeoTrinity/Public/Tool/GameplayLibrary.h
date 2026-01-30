#pragma once
#include "GenericTeamAgentInterface.h"

class GameplayLibrary
{
public:
	static bool GetTeamInterface(AActor const* Actor, IGenericTeamAgentInterface const*& OutInterface);
	static FColor GetColorForObject(UObject const* Object);
	static float IsServer(UWorld const* World);
	static float GetServerTime(UWorld const* World, bool bUpdatedWithPing = false);
	static int GetAndCheckSection(UAnimMontage const* AnimMontage, FName Section);
	static UAnimInstance* GetAnimInstance(struct FAbilityPayload const& Payload);

	inline static FName const SectionStartName{"Start"};
	inline static FString SectionStartString{SectionStartName.ToString()};
	inline static FName const SectionFireName{"Fire"};
	inline static FString SectionFireString{SectionFireName.ToString()};
	inline static FName const SectionEndName{"End"};
	inline static FString SectionEndString{SectionEndName.ToString()};
	inline static FName const SectionStopName{"Stop"};
	inline static FString SectionStopString{SectionStopName.ToString()};
};
