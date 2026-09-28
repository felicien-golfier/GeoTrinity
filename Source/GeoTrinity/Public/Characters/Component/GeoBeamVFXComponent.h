// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Tool/GeoNiagaraParams.h"

#include "GeoBeamVFXComponent.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/** Replication bundle for the beam's visual dimensions and on/off state; a single OnRep fires when any field changes.
 */
USTRUCT()
struct FBeamVFXState
{
	GENERATED_BODY()

	UPROPERTY()
	bool bActive = false;

	UPROPERTY()
	float Width = 0.f;

	UPROPERTY()
	float Length = 0.f;

	UPROPERTY()
	float Lifetime = 0.f;
};

/**
 * Replicated component dynamically added to a character for as long as a beam ability is granted
 * (OnGiveAbility/OnRemoveAbility) and toggled on/off via SetBeamState. The ability drives BeamState on the server;
 * replication delivers it to all clients, where each rendering machine feeds a locally-spawned NiagaraComponent
 * attached to the owner's root — so the beam rotates with the character's aim. Subclass in Blueprint to assign
 * BeamNiagaraSystem and tune the user parameter names.
 */
UCLASS(Blueprintable, BlueprintType)
class GEOTRINITY_API UGeoBeamVFXComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Enables component replication. */
	UGeoBeamVFXComponent();

	/** Spawns the local NiagaraComponent on rendering machines and applies any already-replicated BeamState. */
	virtual void BeginPlay() override;
	/** Destroys the locally-spawned NiagaraComponent before delegating to Super. */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	/** Registers BeamState and BeamSystem for replication. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Activates/deactivates the beam and pushes its dimensions. The server's write replicates to all clients; the
	 * owning client may also call it for lag-free local visuals (its write only feeds the local NiagaraComponent).
	 * LifeTime is pushed to the beam's Lifetime user param. The windup telegraph is not this component's: it is a Ray on
	 * the owner's UGeoIndicatorComponent.
	 */
	void SetBeamState(bool bActive, float Width, float Length, float LifeTime = 0.f);
	/** Assigns the Niagara system before BeginPlay; call from the owning ability's OnGiveAbility. */
	void SetNiagaraSystem(TObjectPtr<UNiagaraSystem> const Object) { BeamSystem = Object; };
	/** Assigns the beam's colours, one per meaning as GeoColor::GetMeaningColors resolves them, pushed to the Niagara
	 * colour pattern parameters; call from the owning ability's OnGiveAbility. */
	void SetBeamColors(TArray<FLinearColor> Colors);

private:
	UFUNCTION()
	void OnRep_BeamState() const;
	UFUNCTION()
	void OnRep_BeamColors() const;
	UFUNCTION()
	void CreateNiagaraComponent();

	/** Pushes BeamState into the local NiagaraComponent (activation + user parameters). No-op on dedicated server. */
	void ApplyBeamState() const;

	UPROPERTY(ReplicatedUsing = OnRep_BeamState)
	FBeamVFXState BeamState;

	UPROPERTY(ReplicatedUsing = CreateNiagaraComponent)
	TObjectPtr<UNiagaraSystem> BeamSystem;

	UPROPERTY(ReplicatedUsing = OnRep_BeamColors)
	TArray<FLinearColor> BeamColors = {FLinearColor::White};

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> NiagaraComponent;
};
