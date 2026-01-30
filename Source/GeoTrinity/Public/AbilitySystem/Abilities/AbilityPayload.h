#pragma once
#include "GameplayTagContainer.h"

#include "AbilityPayload.generated.h"

USTRUCT(BlueprintType)
struct GEOTRINITY_API FAbilityPayload
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly)
	FVector2D Origin{}; // position X,Y

	UPROPERTY(Transient, BlueprintReadOnly)
	float Yaw{}; // orientation

	UPROPERTY(Transient, BlueprintReadOnly)
	float ServerSpawnTime{}; // server world time (seconds)

	UPROPERTY(Transient, BlueprintReadOnly)
	int Seed{}; // seed pour variations RNG

	UPROPERTY(Transient, BlueprintReadOnly)
	int AbilityLevel{};

	// TODO: optimise AbilityTag : remove from payload and set only once on Pattern Creation.
	UPROPERTY(Transient, BlueprintReadOnly)
	FGameplayTag AbilityTag{};

	UPROPERTY(Transient, BlueprintReadOnly)
	AActor* Owner{nullptr};

	UPROPERTY(Transient, BlueprintReadOnly)
	AActor* Instigator{nullptr};
};
