// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Containers/RingBuffer.h"
#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "GeoCharacterMovementComponent.generated.h"

class AGeoCharacter;

/** Where an actor stood, and which way it faced, at ServerTime. */
struct FGeoPose
{
	float ServerTime = 0.f;
	FVector Location = FVector::ZeroVector;
	float Yaw = 0.f;
};

/** Client move stamped with the pattern clock (GeoLib::GetServerTime with ping) of the frame it was simulated on. */
class FGeoSavedMove_Character : public FSavedMove_Character
{
	using Super = FSavedMove_Character;

public:
	/** Resets PerceivedServerTime to 0 so a recycled move does not carry a stale server-time stamp. */
	virtual void Clear() override;
	/** Stamps this move with the owning character's current perceived server time before the base fields are recorded. */
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAcceleration,
							FNetworkPredictionData_Client_Character& ClientData) override;

	float PerceivedServerTime = 0.f;
};

/** Allocates FGeoSavedMove_Character so every client move carries its pattern-clock stamp. */
class FGeoNetworkPredictionData_Client_Character : public FNetworkPredictionData_Client_Character
{
public:
	/** Passes the movement component reference to the base class prediction data. */
	explicit FGeoNetworkPredictionData_Client_Character(UCharacterMovementComponent const& ClientMovement);

	/** Allocates FGeoSavedMove_Character instead of the base type so each move carries a perceived-server-time stamp. */
	virtual FSavedMovePtr AllocateNewMove() override;
};

/** Packed move data sending FGeoSavedMove_Character::PerceivedServerTime to the server. */
struct FGeoCharacterNetworkMoveData : public FCharacterNetworkMoveData
{
	using Super = FCharacterNetworkMoveData;

	/** Copies PerceivedServerTime from the client's saved move into the packed network data alongside the base fields. */
	virtual void ClientFillNetworkMoveData(FSavedMove_Character const& ClientMove, ENetworkMoveType MoveType) override;
	/** Serializes PerceivedServerTime to/from the network packet so the server receives the client's pattern-clock stamp. */
	virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Archive, UPackageMap* PackageMap,
						   ENetworkMoveType MoveType) override;

	float PerceivedServerTime = 0.f;
};

/** Points the new, pending and old move slots at FGeoCharacterNetworkMoveData. */
struct FGeoCharacterNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer
{
	/** Wires the three move slots (new, pending, old) to GeoMoveData so the engine allocates FGeoCharacterNetworkMoveData. */
	FGeoCharacterNetworkMoveDataContainer();

	FGeoCharacterNetworkMoveData GeoMoveData[3];
};

/**
 * Custom movement component that caches the character's base walk speed and acceleration on OnRegister
 * so that attribute-driven multipliers can be applied and restored correctly.
 * Also stamps every client move with the pattern clock, so the server knows at which pattern time a remote player
 * stood where it holds them (see GetPerceivedServerTime).
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	/** Registers GeoNetworkMoveDataContainer so stamped FGeoCharacterNetworkMoveData is used for all server moves. */
	UGeoCharacterMovementComponent();

	/** Caches MaxWalkSpeed and MaxAcceleration as base values for subsequent multiplier application. */
	virtual void OnRegister() override;

	/** Server. Records this frame's pose, dropping the ones no hazard can look back to anymore (see GetPoseAt). */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * Scales MaxWalkSpeed and MaxAcceleration by Multiplier relative to their cached base values.
	 *
	 * @param Multiplier  Scaling factor. 1.0 = base speed, 2.0 = double speed.
	 */
	void ApplySpeedMultiplier(float Multiplier);

	/** Returns FGeoNetworkPredictionData_Client_Character so the engine allocates FGeoSavedMove_Character instances. */
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	/** Server. Records the pattern-clock stamp of the move once it is performed. */
	virtual void ServerMove_PerformMovement(FCharacterNetworkMoveData const& MoveData) override;

	/**
	 * Server. Pattern time at which the character stood at its current server location: the stamp of its last
	 * performed move for a remote player, the current time for a locally controlled or AI character. Hazards judge a
	 * remote player against their state at this time, so the half ping their moves travel costs them no reaction time.
	 * Clamped to [now - MaxLatencyCompensation, now]: a client can neither claim the future nor dodge by going silent.
	 */
	float GetPerceivedServerTime() const;

	/**
	 * Where the character stood, and which way it faced, at ServerTime, interpolated between the poses the server
	 * records every frame. They reach 2 × MaxLatencyCompensation back, as far as a hazard can look: a target's own time
	 * trails by up to one, and it saw this character up to one more earlier (GeoLib::GetReplicationDelay). An older time
	 * gets the oldest pose kept; a time at or past now, the current one — and so does any time on a client, which
	 * records none.
	 */
	FGeoPose GetPoseAt(float ServerTime) const;

protected:
	/**
	 * Server. While IsCorpseFollowingClient, turns on bIgnoreClientMovementErrorChecksAndCorrection and
	 * bServerAcceptClientAuthoritativePosition so Super never corrects the corpse and puts it where the client reports,
	 * movement base included; turns both off otherwise.
	 */
	virtual void ServerMoveHandleClientError(float ClientTimeStamp, float DeltaTime, FVector const& Accel,
											 FVector const& RelativeClientLocation,
											 UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName,
											 uint8 ClientMovementMode) override;

private:
	/**
	 * Server. True while a dead remote player's moves may still come from before its death reached it — up to a full
	 * compensated round trip after the death. A client keeps moving until then, so its corpse follows it there instead
	 * of the client being corrected back to where the hit landed.
	 */
	bool IsCorpseFollowingClient() const;

	float BaseMaxWalkSpeed = 0.f;
	float BaseMaxAcceleration = 0.f;

	FGeoCharacterNetworkMoveDataContainer GeoNetworkMoveDataContainer;
	/** Never decreases, so a client-side ping estimate jump cannot rewind a hazard it already passed. */
	float LastMoveServerTime = 0.f;

	/** Server. One pose per frame, oldest first. */
	TRingBuffer<FGeoPose> PoseHistory;

	AGeoCharacter* GetGeoCharacter() const;
};
