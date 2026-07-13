// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "CoreMinimal.h"
#include "Tickable.h"
#include "Tool/Team.h"

#include "GeoChannelBeamAbility.generated.h"

class ACharacter;
class UNiagaraSystem;

/**
 * Base for channelled beam abilities (Circle's Moira beam, Square's sacrifice beam).
 * Owns the replicated UGeoBeamVFXComponent lifecycle, the per-tick beam-state push,
 * and the per-tick line scan. Subclasses implement TickBeam for their per-target logic.
 */
UCLASS(Abstract)
class GEOTRINITY_API UGeoChannelBeamAbility
	: public UGeoGameplayAbility
	, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/** Registers as a Conditional tickable on the game thread; opts out on async loading threads where CDO construction may run. */
	UGeoChannelBeamAbility();

protected:
	/** Server: adds the replicated BeamVFXComponent to the avatar for as long as the ability is granted. */
	virtual void OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec) override;
	/** Destroys the BeamVFXComponent previously added to the avatar. */
	virtual void OnRemoveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec) override;

	/** Starts the beam channel. */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	/** Stops the channel and switches the beam VFX off before calling Super. */
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	/** Pushes the beam VFX state, then calls TickBeam with the actors currently inside the beam. */
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return IsInstantiated() && IsActive() && bIsBeamActive; }
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UGeoChannelBeamAbility, STATGROUP_Tickables);
	}

	/** Per-tick subclass logic on the actors currently inside the beam. */
	virtual void TickBeam(float DeltaTime, TArray<AActor*> const& ActorsInLine)
		PURE_VIRTUAL(UGeoChannelBeamAbility::TickBeam, );

	/** Beam half-width in cm. Default: half the avatar's capsule radius. */
	virtual float GetCurrentBeamHalfWidth(ACharacter const* Character) const;

	/** Attitude bitmask used by the per-tick line scan. */
	virtual uint8 GetScanAttitudeMask() const { return TeamAttitudeMask::All; }

#ifdef WITH_EDITOR
	void DrawBeamDebugLines(float DeltaTime) const;
#endif

	UPROPERTY(EditDefaultsOnly, Category = "Ability|GameFeel", meta = (AllowPrivateAccess = true))
	TObjectPtr<UNiagaraSystem> BeamNiagaraSystem;

	/** Tint pushed to the beam Niagara system's "Color" user parameter. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|GameFeel", meta = (AllowPrivateAccess = true))
	FLinearColor BeamColor = FLinearColor::White;

	bool bIsBeamActive = false;
};
