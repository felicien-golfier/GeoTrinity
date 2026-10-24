// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/FX/GeoSoundRow.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GeoGemDrop.generated.h"

class UMaterialInstanceDynamic;
class USphereComponent;
class UStaticMeshComponent;

/** What a drop is. */
UENUM()
enum class EGeoGemDropKind : uint8
{
	Gem,
	Shard
};

/** What one drop shows: a gem (its id gives the tier and the colour) or a shard of one of the shard shapes. */
USTRUCT()
struct FGeoGemDropSpec
{
	GENERATED_BODY()

	UPROPERTY()
	EGeoGemDropKind Kind = EGeoGemDropKind::Gem;

	/** The catalog gem shown; unused by a shard. */
	UPROPERTY()
	FName GemId;

	/** The shard shape, 0 .. GeoGem::ShardVariantCount - 1; unused by a gem. */
	UPROPERTY()
	int32 Variant = 0;

	/** Seeds the drop's tumble and rocking, so every machine turns it the same way. */
	UPROPERTY()
	int32 Seed = 0;
};

/** The mesh's vertical bounce at one moment after the landing. */
struct FGeoGemBounce
{
	/** Height above the resting place. */
	float Height = 0.f;

	/** 0 .. 1 glow spike of the latest touchdown, decaying. */
	float Flash = 0.f;
};

/**
 * One gem or shard of a boss kill lying in the world. Visual and audible only: the loot is already in the players'
 * profiles, so it grants nothing. It sounds SpawnSound when thrown and PickupSound where it lay when destroyed, the
 * latter outliving the actor. It is thrown from where it spawns along LaunchVelocity under Gravity, lands on
 * RestHeight. It tumbles in 3D, the turn winding down by friction to a stop in whatever pose it reached, bounces
 * BounceCount times on landing (lighting up at each touchdown), slides to a stop on the ground and is held in by the
 * world's static walls, then bobs until a player character touches it, which destroys it for everyone.
 * Replicated once with its launch data: every machine replays the same path, no movement is replicated. Pickup
 * collision exists on the server only, from the end of the slide on; the actor root stays still from then, so a lying
 * drop costs no overlap update.
 */
UCLASS()
class GEOTRINITY_API AGeoGemDrop : public AActor
{
	GENERATED_BODY()

public:
	/** Replicated, always relevant, ticking; pickup collision off until Land. */
	AGeoGemDrop();

	/** Registers Spec, LaunchVelocity and LaunchTime. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server. Sets what the drop shows and its throw; call between SpawnActorDeferred and FinishSpawning. The throw
	 *  starts at the spawn location, now. */
	void Init(FGeoGemDropSpec const& InSpec, FVector const& InLaunchVelocity);

	/** True once the throw has come down and slid to a stop: from then the drop rests, and a character can pick it up.
	 */
	bool IsLanded() const;

	/** The gem's static mesh component. */
	UStaticMeshComponent* GetMesh() const { return Mesh; }

	/** Where the drop comes to rest. */
	FVector GetRestLocation() const;

	/** The mesh's bounce RestingSeconds after the landing (0 or less: none). */
	FGeoGemBounce GetBounce(float RestingSeconds) const;

	/** 0 .. 1 how far the drop has settled RestingSeconds after the landing: 0 in the air, 1 once the bounces are over.
	 *  The bob fades in along it. */
	float GetSettled(float RestingSeconds) const;

	/** The mesh's turn ElapsedSeconds after the throw started: the tumble, its speed decaying by TumbleFriction, so the
	 *  turn only ever slows and ends in the pose it reached. */
	FQuat GetPose(float ElapsedSeconds) const;

	/** Mesh scale ElapsedSeconds after the throw started: 0 growing to 1 over PopTime, overshooting. */
	float GetPopScale(float ElapsedSeconds) const;

protected:
	/** Sizes the pickup sphere, anchors the throw on the local clock, shows the mesh and colour; on the server, binds
	 *  the pickup and lands the drop when the throw comes down. A dedicated server never ticks a drop. */
	virtual void BeginPlay() override;

	/** Plays PickupSound where the drop lies when it was landed and destroyed; any other end (level change, PIE end,
	 *  a drop still in the air) is silent. */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Plays Entry at Location as a world-owned one-shot, so it rings on after this actor is gone. */
	virtual void PlayDropSound(FGeoSoundEntry const& Entry, FVector const& Location);

	/** Visual: moves the actor along the arc, then poses, lifts, scales and lights the mesh. */
	virtual void Tick(float DeltaSeconds) override;

	/** Simulates the drop's horizontal path from the launch, one point per PathStep: straight in the air, slowing by
	 *  GroundFriction from the touchdown, reflected by world-static geometry. Sets TravelTime. */
	void BuildPath();

	/** Where the throw is TravelSeconds after it started: along Path, at the height of the arc. */
	FVector GetThrowLocation(float TravelSeconds) const;

	/** Server. Puts the drop at its rest location and turns the pickup collision on; a player character already
	 *  standing there picks it up at once. */
	void Land();

	/** Server. Destroys the drop when OtherActor is a player character. Bound to the pickup's begin overlap. */
	UFUNCTION()
	void OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
						 UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
						 FHitResult const& SweepResult);

	/** Shape and colour as Spec says: the tier's solid tinted by the gem's stat colour, or a shard keeping its own. */
	void ApplyLook();

	/** Pitch multiplier range both drop sounds roll in, so one asset each sounds different on every drop. */
	static inline FVector2D const DropPitchRange{0.85, 1.25};

	/** Name of the material parameter the stat colour goes in. */
	static inline FName const ColorParameter{"Color"};

	/** Name of the material parameter the touchdown glow goes in. */
	static inline FName const FlashParameter{"Flash"};

	/** World Z the drop lands and rests at (its centre). */
	UPROPERTY(EditDefaultsOnly, Category = "GeoGemDrop")
	float RestHeight = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "GeoGemDrop")
	float Gravity = 980.f;

	/** Degrees per second the tumble turns at the throw, rolled in this range. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoGemDrop|Motion")
	FFloatInterval TumbleSpeed = FFloatInterval(600.f, 1500.f);

	/** Per second the tumble's speed decays at. The turn ends TumbleSpeed / TumbleFriction degrees after the throw. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoGemDrop|Motion", meta = (ClampMin = 0.01))
	float TumbleFriction = 1.f;

	/** Per second the sliding speed on the ground decays at. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoGemDrop|Motion", meta = (ClampMin = 0))
	float GroundFriction = 2.5f;

	/** Share of its speed a drop keeps off a wall. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoGemDrop|Motion", meta = (ClampMin = 0, ClampMax = 1))
	float WallRestitution = 0.5f;

	/** Radius of the sweep that meets the walls, at RestHeight: keep it under RestHeight or the floor blocks the drop.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "GeoGemDrop|Motion", meta = (ClampMin = 1))
	float CollisionRadius = 15.f;

	/** Cm per second under which a sliding drop is at rest. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoGemDrop|Motion", meta = (ClampMin = 0.1))
	float RestSpeed = 5.f;

	/** Seconds the mesh takes to grow from nothing at the throw. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoGemDrop|Motion", meta = (ClampMin = 0.01))
	float PopTime = 0.2f;

	/** Bounces after the landing. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoGemDrop|Motion", meta = (ClampMin = 0))
	int32 BounceCount = 3;

	/** Share of the impact speed each bounce leaves with, from the one before. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoGemDrop|Motion", meta = (ClampMin = 0.05, ClampMax = 0.95))
	float BounceRestitution = 0.4f;

	/** Per second the touchdown glow dies out at. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoGemDrop|Motion")
	float FlashDecay = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "GeoGemDrop")
	float BobHeight = 6.f;

	/** Radians per second of the bob. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoGemDrop")
	float BobSpeed = 2.f;

	/** Distance from the drop a player character picks it up at. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoGemDrop")
	float PickupRadius = 90.f;

	/** Played at the throw location when a drop starts its throw; a drop already lying on a late join stays silent. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoGemDrop|Sound")
	FGeoSoundEntry SpawnSound;

	/** Played where the drop lay when a player character picks it up. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoGemDrop|Sound")
	FGeoSoundEntry PickupSound;

private:
	UPROPERTY(VisibleAnywhere, Category = "GeoGemDrop")
	TObjectPtr<USphereComponent> Pickup;

	UPROPERTY(VisibleAnywhere, Category = "GeoGemDrop")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** The mesh's own material, to light up on touchdown; null when the look could not be applied. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	UPROPERTY(Replicated)
	FGeoGemDropSpec Spec;

	UPROPERTY(Replicated)
	FVector LaunchVelocity = FVector::ZeroVector;

	/** Server world time Init ran at. Replicated to become LocalLaunchTime and nothing else. */
	UPROPERTY(Replicated)
	float LaunchTime = 0.f;

	/** This machine's own world time the throw started at. */
	float LocalLaunchTime = 0.f;

	/** Where the throw started. */
	FVector LaunchLocation = FVector::ZeroVector;

	/** Seconds between two points of Path. */
	static constexpr float PathStep = 1.f / 30.f;

	/** Most points Path holds (20 s): a drop pinned by a wall stops there. */
	static constexpr int32 MaxPathPoints = 600;

	/** Seconds from the throw to RestHeight. */
	float FlightTime = 0.f;

	/** Seconds from the throw to the end of the slide: FlightTime or more. */
	float TravelTime = 0.f;

	/** Horizontal place of the drop every PathStep after the throw, from BuildPath. */
	TArray<FVector2D> Path;

	/** Downward speed at the landing. */
	float ImpactSpeed = 0.f;

	/** Axis of the tumble, from Spec.Seed. */
	FVector TumbleAxis = FVector::UpVector;

	/** Radians per second of the tumble at the throw, from Spec.Seed. */
	float TumbleRadians = 0.f;
};
