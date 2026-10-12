// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "GeoIndicatorComponent.generated.h"

class UGeoIndicatorComponent;
class UNiagaraComponent;
class USceneComponent;

UENUM()
enum class EGeoIndicatorShape : uint8
{
	/** A circle Size.X in radius — UGameDataSettings::RoundIndicatorSystem. */
	Round,
	/** A ray Size.X long and Size.Y wide, along its parent's forward — UGameDataSettings::RayIndicatorSystem. */
	Ray
};

/** Everything one telegraph shows: what, where, in which colours, and when. */
USTRUCT()
struct FGeoIndicatorState
{
	GENERATED_BODY()

	UPROPERTY()
	EGeoIndicatorShape Shape = EGeoIndicatorShape::Round;

	/** World location, or the offset from the owner's root when bAttachToOwner. */
	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	/** Follows the owner, turning with it — a ray telegraphing its aim. */
	UPROPERTY()
	bool bAttachToOwner = false;

	/** See EGeoIndicatorShape. */
	UPROPERTY()
	FVector2D Size = FVector2D::ZeroVector;

	/** One per meaning, as GeoColor::GetMeaningColors resolves them. */
	UPROPERTY()
	TArray<FLinearColor> Colors;

	/** Server time the telegraph started at; a machine showing it later starts it that far in. */
	UPROPERTY()
	float StartServerTime = 0.f;

	/** Seconds from StartServerTime until it ends by itself. 0 lasts until removed. */
	UPROPERTY()
	float Duration = 0.f;
};

USTRUCT()
struct FGeoIndicatorItem : public FFastArraySerializerItem
{
	GENERATED_BODY()

	/** Spawns the telegraph on the client it just reached. */
	void PostReplicatedAdd(struct FGeoIndicatorArray const& InArraySerializer);
	/** Removes the telegraph from the client it was removed for. */
	void PreReplicatedRemove(struct FGeoIndicatorArray const& InArraySerializer);

	UPROPERTY()
	FGeoIndicatorState State;

	/** This machine's own drawing of State; null on a dedicated server or once it was over. */
	UPROPERTY(NotReplicated, Transient)
	TObjectPtr<UNiagaraComponent> NiagaraComponent;
};

USTRUCT()
struct FGeoIndicatorArray : public FFastArraySerializer
{
	GENERATED_BODY()

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FGeoIndicatorItem, FGeoIndicatorArray>(Items, DeltaParms, *this);
	}

	UPROPERTY()
	TArray<FGeoIndicatorItem> Items;

	/**
	 * Set in UGeoIndicatorComponent::OnRegister: the template's copy into a spawned instance lands after
	 * PostInitProperties and would leave it pointing at the template, which has no world.
	 */
	UGeoIndicatorComponent* Owner = nullptr;
};

template <>
struct TStructOpsTypeTraits<FGeoIndicatorArray> : public TStructOpsTypeTraitsBase2<FGeoIndicatorArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

/**
 * Replicated telegraphs of what its owner is about to do — any number at once, each on its own timeline. The server's
 * telegraphs reach every other machine through replication, reliably and late joiners included, and each machine
 * starts one as far in as the server time it began at. The owning client never receives them: it adds its own, at
 * once, from the same predicted code path.
 * Telegraphs from something already running on every machine (a pattern, a replicated actor) need none of this and
 * spawn their own through SpawnIndicator.
 */
UCLASS()
class GEOTRINITY_API UGeoIndicatorComponent : public UActorComponent
{
	GENERATED_BODY()

	friend FGeoIndicatorItem;

public:
	/** Enables component replication. */
	UGeoIndicatorComponent();

	/** Points Indicators back at this component, for its replication callbacks. */
	virtual void OnRegister() override;
	/** Registers Indicators for replication, skipping the owner. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/** Removes every telegraph still drawn before delegating to Super. */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/**
	 * Shows State until its Duration runs out or RemoveIndicator. Call it on the server and on the owning client alike:
	 * the server's replicates to everyone else, the owner's stays local. The server keeps StartServerTime within
	 * MaxLatencyCompensation of now, so a client can neither shorten the warning others get nor delay it.
	 *
	 * @return Handle for RemoveIndicator, valid only on the machine that added it. INDEX_NONE when already over.
	 */
	int32 AddIndicator(FGeoIndicatorState State);

	/** Ends the telegraph Handle names at once. No-op when it already ended. */
	void RemoveIndicator(int32 Handle);

	/**
	 * Draws State on this machine only, AttachParent-relative when set, already as far in as the server time says.
	 * Returns null on a dedicated server or when State is already over. The caller destroys it if it must end early.
	 */
	static UNiagaraComponent* SpawnIndicator(UObject const* WorldContextObject, FGeoIndicatorState const& State,
											 USceneComponent* AttachParent = nullptr);

private:
	/** SpawnIndicator attached to the owner's root when State asks for it. */
	UNiagaraComponent* SpawnOwnIndicator(FGeoIndicatorState const& State) const;

	UPROPERTY(Replicated)
	FGeoIndicatorArray Indicators;
};
