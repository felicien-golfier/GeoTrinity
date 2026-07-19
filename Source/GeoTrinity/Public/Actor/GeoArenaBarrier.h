// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GeoArenaBarrier.generated.h"

class UBoxComponent;

/**
 * An actor paired with the transforms it lerps between when the barrier opens or closes.
 * FightOnTransform is the resting place while the fight is active (barrier closed);
 * FightOffTransform is the resting place while the fight is inactive (barrier open).
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
	FTransform FightOnTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier",
			  meta = (EditCondition = "bHasMovement", EditConditionHides))
	FTransform FightOffTransform = FTransform::Identity;
};

/**
 * Replicated barrier that blocks the arena entrance. Collision and visibility sync to all clients
 * via OnRep_bIsClosed. Visual/VFX are handled by BlueprintImplementableEvent.
 * On state change, animated actors lerp between their fight-on and fight-off locations over the
 * game state's CommitFightTime. Ticks only while that lerp is in flight.
 */
UCLASS()
class GEOTRINITY_API AGeoArenaBarrier : public AActor
{
	GENERATED_BODY()

public:
	/** Sets up replication and enables tick capability; tick starts disabled and is re-enabled by BeginPlay and OnRep_bIsClosed. */
	AGeoArenaBarrier();

	/** Registers bIsClosed for replication so open/close state is pushed to all clients. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Enables one tick so animated actors snap to their default (fight-off) positions, after which Tick disables itself. */
	virtual void BeginPlay() override;
	/** Advances the open/close lerp, then disables itself once the animated actors reach their resting transform. */
	virtual void Tick(float DeltaSeconds) override;
	/** Server-only. Opens or closes the barrier and replicates state to all clients. */
	void SetClosed(bool bNewClosed);

protected:
	/** BP implementable event so designers can add VFX / dissolve on state change. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Barrier")
	void OnBarrierStateChanged(bool bClosed);

	/** Editor: store each animated actor's current world transform as its fight-on transform. */
	UFUNCTION(CallInEditor, Category = "Barrier")
	void CaptureFightOnTransforms();

	/** Editor: store each animated actor's current world transform as its fight-off transform. */
	UFUNCTION(CallInEditor, Category = "Barrier")
	void CaptureFightOffTransforms();

	/** Editor: move each animated actor to its stored fight-on transform. */
	UFUNCTION(CallInEditor, Category = "Barrier")
	void SetToFightOnTransforms();

	/** Editor: move each animated actor to its stored fight-off transform. */
	UFUNCTION(CallInEditor, Category = "Barrier")
	void SetToFightOffTransforms();

	/** Actors lerped between their two locations when the barrier state changes. */
	UPROPERTY(EditAnywhere, Category = "Barrier")
	TArray<FBarrierAnimatedActor> AnimatedActors;


private:
	UPROPERTY(ReplicatedUsing = OnRep_bIsClosed, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bIsClosed = false;

	/** Lerp progress: 0 = fight-off location, 1 = fight-on location. */
	float LerpAlpha = 0.0f;

	UFUNCTION()
	void OnRep_bIsClosed();
};
