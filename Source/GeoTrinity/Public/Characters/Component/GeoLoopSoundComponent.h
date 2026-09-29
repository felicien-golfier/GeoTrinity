// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/GeoSoundRow.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GeoLoopSoundComponent.generated.h"

class UAnimInstance;
class UAudioComponent;
class USkeletalMeshComponent;

/** One layer of a running sound: a seamless loop, bent by the owner's montages and wandering slowly on its own. */
USTRUCT(BlueprintType)
struct FGeoLoopSound
{
	GENERATED_BODY()

	/** A looping asset, with its volume and audience. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoSound")
	FGeoSoundEntry Sound;

	/** Anim curve adding its value in semitones to the loop's pitch. 0 is the loop as authored; the montage's blend in
	 * and out fades it, so a montage whose curve starts and ends at 0 hands the loop back untouched. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoSound")
	FName PitchCurve = TEXT("LoopSemitones");

	/** Anim curve adding its value in decibels to the loop's volume, on top of RestDecibels. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoSound")
	FName VolumeCurve = TEXT("LoopDecibels");

	/** The loop's level in decibels while no montage bends it, 0 being as authored. At -80 the layer is silent until a
	 * montage's VolumeCurve raises it, by 80 to play it as authored. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoSound", meta = (ClampMax = "0"))
	float RestDecibels = 0.f;

	/** How far the pitch wanders on its own, in semitones either side. Layers with the same DriftPeriod wander as one,
	 * like parts of one machine. It settles as a montage blends in, so a montage bends the loop exactly as its curves
	 * say. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoSound", meta = (ClampMin = "0"))
	float DriftSemitones = 0.3f;

	/** Seconds the wander takes to swing from one side to the other, about. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoSound", meta = (ClampMin = "0.1"))
	float DriftPeriod = 6.f;
};

/** A layer of Loops while it plays. */
USTRUCT()
struct FGeoRunningLoopSound
{
	GENERATED_BODY()

	/** Null when the layer must not play on this machine. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> AudioComponent;

	/** Rolled when the layer starts and kept until it stops, so re-pushing the pitch never jumps it. */
	float PitchVariation = 1.f;
};

/**
 * Sounds an owner keeps running for as long as something turns them on — a boss's hum through its fight. Every layer
 * of Loops plays at once, attached to the owner.
 *
 * Nothing crossfades: a montage never replaces the loop, it bends it through the PitchCurve and VolumeCurve anim curves
 * it carries, which reach the loop weighted by the montage's own blend. That keeps a single continuous sound whatever
 * plays, with no seam where two takes of the same hum would beat against each other.
 *
 * Silent on a dedicated server.
 */
UCLASS(ClassGroup = "GeoTrinity", meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoLoopSoundComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Ticks only while the loops play or fade, after animation, so the curves it reads are this frame's. */
	UGeoLoopSoundComponent();

	/** Finds the owner's skeletal mesh, whose anim curves bend the loops. */
	virtual void BeginPlay() override;

	/** Ramps the loops in or out by FadeDuration, starting them on the way in and destroying them once faded out, then
	 * pushes each one's pitch and volume. */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	/** Fades every layer of Loops in (bPlay) or out. Idempotent, and either way mid-fade turns the fade around.
	 * bFade false jumps straight there instead, for a caller whose anim curves already bring the loops in. */
	void SetPlaying(bool bPlay, bool bFade = true);

	/** Layers played together, each bent by its own curves. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoLoopSound")
	TArray<FGeoLoopSound> Loops;

	/** Seconds the loops take to fade in when turned on, and out when turned off. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoLoopSound", meta = (ClampMin = "0.01"))
	float FadeDuration = 1.f;

private:
	/** Spawns one audio component per layer of Loops, rolling each one's pitch variation. The tick's UpdateLoops sets
	 * their real pitch and volume the same frame. */
	void StartLoops();

	/** Pushes each running layer's pitch and volume: the entry's own, bent by its curves and its drift, scaled by the
	 * fade. */
	void UpdateLoops() const;

	/** Stops and destroys every running layer. */
	void StopLoops();

	/** CurveName's value on the owner's anim instance: 0 when the playing montages don't carry it, or no anim plays. */
	float GetCurveValue(FName CurveName) const;

	/** Weight of the most blended-in montage on the owner, 0 when none plays: how far the drift has settled. */
	float GetMontageWeight() const;

	/** The owner's anim instance, null when it has no skeletal mesh or the mesh runs no anim. */
	UAnimInstance const* GetAnimInstance() const;

	/** The layers of Loops while they play or fade, index for index. Empty while silent. */
	UPROPERTY()
	TArray<FGeoRunningLoopSound> Running;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Mesh;

	/** Whether the loops are turned on — what the fade heads for. */
	bool bPlaying = false;

	/** How far faded in, 0 silent to 1 full. */
	float Presence = 0.f;
};
