// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GeoCat.generated.h"

class UAnimMontage;
class USkeletalMeshComponent;

/** Where and how the Cat roams: random spots inside Area's bounds, a pause between two walks. */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGeoCatRoam
{
	GENERATED_BODY()

	/** The room roamed in (its bounds): the room's AGeoArenaVolume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoCat")
	TObjectPtr<AActor> Area;

	/** Inset from Area's bounds, so the cat never walks into a wall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoCat", meta = (ClampMin = "0"))
	float Margin = 150.f;

	/** Seconds of pause between two walks: random between X (min) and Y (max). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoCat")
	FVector2D Pause = FVector2D(1.5f, 4.f);

	/** cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoCat", meta = (ClampMin = "1"))
	float Speed = 150.f;
};

/** The order the Cat is carrying out. */
enum class EGeoCatOrder : uint8
{
	Walk,
	Roam,
	Montage
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGeoCatOrderEnded, bool /*bCompleted*/);

/**
 * The Tutorial Cat in the world (SKM_Cat + ABP_Cat, set by BP_Cat). Knows how to walk, roam and play a montage; what,
 * where and when come from its orders (encounter actions). One order at a time: a new one ends the current. With no
 * order it stands, facing the player. Walks kinematically: straight segments, no collision, no navmesh, Z kept. Not
 * replicated: it lives on the solo tutorial host.
 */
UCLASS()
class GEOTRINITY_API AGeoCat : public AActor
{
	GENERATED_BODY()

public:
	AGeoCat();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Walks through Points in order at Speed, stopping StopDistance short of the last one, facing it; completes
	 * there. */
	void Walk(TArray<FVector> const& Points, float Speed, float StopDistance);

	/** Walks to random spots of Settings.Area, pausing between them. Ends only when replaced. */
	void Roam(FGeoCatRoam const& Settings);

	/** Plays Montage on the anim instance; completes when it starts blending out, so an order given then (a hide) lands
	 * before the blend back to locomotion shows. No montage or anim instance: completes at once. */
	void PlayMontage(UAnimMontage* Montage);

	/** Ends the running order as replaced and stops any montage: the cat stands where it is. Nothing when idle. */
	void Stop();

	/** True from an order's start until it ends (completed or replaced). */
	bool IsOrderRunning() const { return bOrderRunning; }

	/** Moves the cat along its walk, turns it, eases the AnimBP's Speed and counts down to the next idle variant while
	 * it stands with no montage playing; called by Tick while visible. */
	void AdvanceMove(float DeltaSeconds);

	/** The next idle variant of the shuffled bag: all of IdleVariants come round before any repeats, never the same
	 * twice in a row. Null when IdleVariants is empty. */
	UAnimMontage* PickIdleVariant();

	/** Seconds of standing left before the next idle variant is drawn. */
	float GetIdleVariantCountdown() const { return IdleVariantCountdown; }

	/** Every order ends through it: true when it completed, false when a new order replaced it. */
	FOnGeoCatOrderEnded OnOrderEnded;

	/** Degrees per second toward the travel direction, or the player when free. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoCat", meta = (ClampMin = "1"))
	float TurnRate = 360.f;

	/** cm/s per second the AnimBP's Speed moves toward 0 (standing) or the walk's speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoCat", meta = (ClampMin = "0.1"))
	float SpeedBlendRate = 600.f;

	/** Montages played at random while the cat stands, a roam pause included (BP_Cat: the Cat_Idle_* montages). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoCat")
	TArray<TObjectPtr<UAnimMontage>> IdleVariants;

	/** Seconds of standing between the end of one idle variant and the next: random between X (min) and Y (max). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoCat")
	FVector2D IdleVariantPause = FVector2D(6.f, 14.f);

	/** Standing with no montage: turns toward the first local pawn, so the idle reads as talking to the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoCat")
	bool bFacePlayer = true;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoCat")
	TObjectPtr<USceneComponent> Root;

	/** Faces +X. No collision. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoCat")
	TObjectPtr<USkeletalMeshComponent> Mesh;

private:
	/** Stops, then starts NewOrder. */
	void BeginOrder(EGeoCatOrder NewOrder);

	/** Drops the walk and the roam pause, then broadcasts OnOrderEnded; nothing when no order runs. */
	void EndOrder(bool bCompleted);

	/** The last waypoint is reached: the walk completes, or the roam pauses. */
	void OnWalkEnded();

	void WalkToRandomRoamSpot();

	/** Draws and plays an idle variant. */
	void PlayIdleVariant();

	void StopMontages() const;

	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	void TurnToward(FVector const& Direction, float DeltaSeconds);

	/** Counts the countdown down by DeltaSeconds and plays the next idle variant at 0. */
	void TickIdleVariant(float DeltaSeconds);

	/** Writes AnimSpeed to the anim instance's Speed variable (cat_anim_blueprint.py SPEED_VARIABLE). */
	void WriteAnimSpeed() const;

	EGeoCatOrder Order = EGeoCatOrder::Walk;
	bool bOrderRunning = false;

	/** Spots still to reach, in order; empty = standing. */
	TArray<FVector> Waypoints;

	float WalkSpeed = 0.f;

	/** What the cat faces once its walk ends short of its last point. */
	TOptional<FVector> FaceAtEnd;

	FGeoCatRoam RoamSettings;
	FTimerHandle RoamPauseTimer;

	/** The real speed in cm/s (0 standing), eased at SpeedBlendRate. */
	float AnimSpeed = 0.f;

	/** Standing seconds left before the next idle variant; held while any montage plays. */
	float IdleVariantCountdown = 0.f;

	/** Variants still to come in this round; the next one is the last. */
	UPROPERTY()
	TArray<TObjectPtr<UAnimMontage>> IdleVariantBag;

	UPROPERTY()
	TObjectPtr<UAnimMontage> LastIdleVariant;
};
