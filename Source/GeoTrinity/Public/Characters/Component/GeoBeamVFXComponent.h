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

	/** True while showing the windup preview (the component's own IndicatorSystem) instead of the ability's BeamSystem.
	 * Lets one NiagaraComponent serve both — ApplyBeamState swaps its asset on transition. */
	UPROPERTY()
	bool bIsIndicator = false;

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
	 * bIsIndicator swaps the NiagaraComponent onto the component's own IndicatorSystem instead of the ability's
	 * BeamSystem — used for the windup telegraph shown before Fire(). IndicatorSystem is a class default (same asset
	 * on every machine already), so only the bool needs to replicate, not a second system pointer. Lifetime
	 * (only meaningful while bIsIndicator) is pushed to the indicator's Lifetime user param, e.g. the ability's fire
	 * delay, so the telegraph animation times out exactly when the beam actually fires.
	 */
	void SetBeamState(bool bActive, float Width, float Length, bool bIsIndicator = false, float LifeTime = 0.f);
	/** Assigns the Niagara system before BeginPlay; call from the owning ability's OnGiveAbility. */
	void SetNiagaraSystem(TObjectPtr<UNiagaraSystem> const Object) { BeamSystem = Object; };
	/** Assigns the beam tint pushed to the Niagara Color user parameter; call from the owning ability's OnGiveAbility.
	 */
	void SetBeamColor(FLinearColor const Color) ;

private:
	UFUNCTION()
	void OnRep_BeamState() const;	
	UFUNCTION()
	void OnRep_BeamColor() const;
	UFUNCTION()
	void CreateNiagaraComponent();

	/** Pushes BeamState into the local NiagaraComponent (activation + user parameters, swapping the asset when
	 * bIsIndicator changes). No-op on dedicated server. */
	void ApplyBeamState() const;

	UPROPERTY(ReplicatedUsing = OnRep_BeamState)
	FBeamVFXState BeamState;

	UPROPERTY(ReplicatedUsing = CreateNiagaraComponent)
	TObjectPtr<UNiagaraSystem> BeamSystem;

	/** Windup preview asset (Ray Zone Indicator), loaded once from UGameDataSettings::RayIndicatorSystem in
	 * CreateNiagaraComponent — no per-component configuration needed. Same project-wide asset on every machine
	 * already, so it needs no replication, unlike BeamSystem. */
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> IndicatorSystem;

	UPROPERTY(ReplicatedUsing = OnRep_BeamColor)
	FLinearColor BeamColor = FLinearColor::White;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> NiagaraComponent;
};
