// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "GeoEncounterObject.generated.h"

class AGeoEncounter;

/** Abstract base of trigger, condition and action. Instanced on the placed encounter, so its properties hold direct
 *  level references. */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class GEOTRINITY_API UGeoEncounterObject : public UObject
{
	GENERATED_BODY()

public:
	/** Null on the class default object, else the world of the outer encounter. */
	virtual UWorld* GetWorld() const override;

#if WITH_EDITOR
	virtual bool ImplementsGetWorld() const override { return true; }
#endif

	/** The encounter this object sits in. */
	UFUNCTION(BlueprintPure, Category = "GeoEncounter")
	AGeoEncounter* GetEncounter() const;
};
