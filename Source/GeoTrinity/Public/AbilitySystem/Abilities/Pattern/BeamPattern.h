// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "CoreMinimal.h"

#include "BeamPattern.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/**
 * Beam fired from the payload origin along the payload yaw, staying on for BeamDuration.
 * SweepAngle turns it into a rotating sweep (0 leaves it pointing straight ahead); every hostile it crosses is hit
 * once per activation, so crossing the beam costs one hit no matter how slowly you walk through it.
 * With bDestroyLastTileHit the shot also carves out the furthest arena tile it reaches — the tank picks which rim
 * tile that is by choosing where they stand when the boss locks on.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API UBeamPattern : public UTickablePattern
{
	GENERATED_BODY()

protected:
	/** Spawns the deactivated beam Niagara component; the pattern instance and its component are reused per activation.
	 */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;
	/** Carves the last tile under the beam and switches the beam VFX on before the tick loop starts. */
	virtual void StartPattern() override;
	/** Aims the beam for the elapsed sweep fraction and applies the effect data to every actor it newly crosses. */
	virtual void TickPattern(float ServerTime, float SpentTime) override;
	/** Switches the beam VFX off and clears the per-activation hit set. */
	virtual void EndPattern(bool bForceStop = false) override;
	/** Adds the beam length so the telegraph cue can size itself. */
	virtual FGameplayCueParameters FillCueParam(FAbilityPayload const& Payload) override;

	/** Beam yaw at SpentTime: the payload yaw, offset by however much of the sweep arc has been travelled. */
	float GetBeamYaw(float SpentTime) const;

	/** Full arc swept over BeamDuration, in degrees, centered on the payload yaw. 0 keeps the beam static. */
	float SweepAngle;

	/** How long the beam stays on, in seconds. */
	UPROPERTY(EditDefaultsOnly, Category = "Beam", meta = (ClampMin = "0.01"))
	float BeamDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Beam", meta = (ClampMin = "0.0"))
	float BeamRange = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Beam", meta = (ClampMin = "0.0"))
	float BeamHalfWidth = 60.f;

	/** Destroys the furthest arena tile the beam reaches, at the moment it fires. */
	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	bool bDestroyLastTileHit = false;

	// NOT DETERMINISTIC !!
	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	bool FollowBossOrientation = false;

	// NOT DETERMINISTIC !!
	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	bool FollowBossLocation = false;

	/** Beam visual, authored local-space pointing +X like the systems UGeoBeamVFXComponent drives. Optional. */
	UPROPERTY(EditDefaultsOnly, Category = "Beam|GameFeel")
	TObjectPtr<UNiagaraSystem> BeamVfxSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Beam|GameFeel")
	FColor BeamColor;

	/** Niagara user param names, matching UGeoBeamVFXComponent's User.Beam_Length/Width/Color. */
	UPROPERTY(EditDefaultsOnly, Category = "Beam|GameFeel")
	FName BeamLengthParamName = "Beam_Length";

	UPROPERTY(EditDefaultsOnly, Category = "Beam|GameFeel")
	FName BeamWidthParamName = "Beam_Width";

	UPROPERTY(EditDefaultsOnly, Category = "Beam|GameFeel")
	FName BeamColorParamName = "Color";

private:
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> BeamVfxComponent;
};
