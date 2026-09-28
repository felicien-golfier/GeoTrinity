// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "ZonePattern.generated.h"

class UNiagaraComponent;

/**
 * Telegraphs a circle at the payload origin for the whole wind-up, in the zone's own colours, then delivers the
 * ability's effects there. A ZoneParams.LifeDrainMaxDuration above zero leaves a zone behind that carries the effects
 * for that long; zero lands them all at once on whoever TeamAttitude matches inside the circle, with StartCue as the
 * burst. The zone class comes from UGameDataSettings::DefaultZoneClass unless ZoneClass overrides it.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API UZonePattern : public UPattern
{
	GENERATED_BODY()

protected:
	/** Turns the hazard on for a burst, off for a lingering zone — the zone judges its own. */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;
	/** Draws the telegraph for what is left of the wind-up. */
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;
	/** Drops the telegraph; for a lingering zone the server spawns it, and every machine ends the pattern. */
	virtual void StartPattern() override;
	/** True when Location stands in the circle. */
	virtual bool IsInHazard(AActor const* Target, FVector2D Location, float SpentTime) const override;
	/** Drops the telegraph if a force-stop comes before StartPattern. */
	virtual void EndPattern(bool bForceStop = false) override;
	/** Adds the circle's radius so StartCue can size itself. */
	virtual FGameplayCueParameters FillCueParam(FGeoCueParam const& Cue, FAbilityPayload const& Payload) override;

private:
	/** ZoneClass, or the project-wide zone from UGameDataSettings when this pattern names none. */
	TSubclassOf<AGeoDeployableBase> GetZoneClass() const;

	/** Destroys the wind-up telegraph, if still drawn. */
	void RemoveIndicator();

	/** Zone left behind. Unset uses UGameDataSettings::DefaultZoneClass. Unused by a burst. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoZone", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AGeoDeployableBase> ZoneClass;

	/** The circle: Size is its radius, Color and SecondaryColors its telegraph's (and the zone's) colours. A zero
	 * LifeDrainMaxDuration makes it a burst. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoZone", meta = (AllowPrivateAccess = "true"))
	FDeployableDataParams ZoneParams;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> IndicatorComponent;
};
