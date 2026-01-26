#pragma once
#include "GenericTeamAgentInterface.h"

class GameplayLibrary
{
public:
	static bool GetTeamInterface(const AActor* Actor, const IGenericTeamAgentInterface*& OutInterface);
	static FColor GetColorForObject(const UObject* Object);
	static float IsServer(const UWorld* World);
	static float GetServerTime(const UWorld* World, bool bUpdatedWithPing = false);
	static int GetAndCheckSection(const UAnimMontage* AnimMontage, FName Section);
	static UAnimInstance* GetAnimInstance(const struct FAbilityPayload& Payload);

	inline static const FName SectionStartName{"Start"};
	inline static const FName SectionFireName{"Fire"};
	inline static const FName SectionEndName{"End"};
};
