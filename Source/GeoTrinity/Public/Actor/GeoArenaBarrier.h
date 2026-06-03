// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GeoArenaBarrier.generated.h"

class UBoxComponent;

/**
 * An actor moved between two world locations when the barrier opens/closes.
 * FightOnLocation is the resting place while the fight is active (barrier closed),
 * FightOffLocation while the fight is inactive (barrier open).
 */
USTRUCT(BlueprintType)
struct FBarrierAnimatedActor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
	bool bHasMovement = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier",
			  meta = (EditCondition = "bHasMovement", EditConditionHides))
	FVector FightOnLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier",
			  meta = (EditCondition = "bHasMovement", EditConditionHides))
	FVector FightOffLocation = FVector::ZeroVector;
};

/**
 * Replicated barrier that blocks the arena entrance. Collision and visibility sync to all clients
 * via OnRep_bIsClosed. Visual/VFX are handled by BlueprintImplementableEvent.
 * On state change, animated actors lerp between their fight-on and fight-off locations over LerpDuration.
 */
UCLASS()
class GEOTRINITY_API AGeoArenaBarrier : public AActor
{
	GENERATED_BODY()

public:
	AGeoArenaBarrier();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void BeginPlay() override;
	/** Server-only. Opens or closes the barrier and replicates state to all clients. */
	void SetClosed(bool bNewClosed);

protected:
	/** BP implementable event so designers can add VFX / dissolve on state change. */
	UFUNCTION(BlueprintNativeEvent, Category = "Barrier")
	void OnBarrierStateChanged(bool bClosed);

	/** Editor: store each animated actor's current world location as its fight-on location. */
	UFUNCTION(CallInEditor, Category = "Barrier")
	void CaptureFightOnLocations();

	/** Editor: store each animated actor's current world location as its fight-off location. */
	UFUNCTION(CallInEditor, Category = "Barrier")
	void CaptureFightOffLocations();

	/** Actors lerped between their two locations when the barrier state changes. */
	UPROPERTY(EditAnywhere, Category = "Barrier")
	TArray<FBarrierAnimatedActor> AnimatedActors;

	/** Time in seconds to lerp animated actors from one location to the other. */
	UPROPERTY(EditAnywhere, Category = "Barrier", meta = (ClampMin = "0.0"))
	float LerpDuration = 1.0f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_bIsClosed, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bIsClosed = false;

	/** Lerp progress: 0 = fight-off location, 1 = fight-on location. */
	float LerpAlpha = 0.0f;

	UFUNCTION()
	void OnRep_bIsClosed();

	void TickLerp();
};
