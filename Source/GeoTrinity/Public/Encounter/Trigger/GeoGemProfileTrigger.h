// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterTrigger.h"

#include "GeoGemProfileTrigger.generated.h"

class UGeoGemProfileSave;
enum class EPlayerClass : uint8;

UENUM(BlueprintType)
enum class EGeoGemProfileEvent : uint8
{
	Equipped,
	Crafted,
};

/**
 * Fires when the first local player's gem profile does Event: Equipped (a gem slotted; what was slotted before
 * arming never counts) or Crafted (a gem crafted at the Forge). Count = occurrences so far, Value = the quantity
 * crafted (Crafted).
 */
UCLASS()
class GEOTRINITY_API UGeoGemProfileTrigger : public UGeoEncounterTrigger
{
	GENERATED_BODY()

public:
	virtual void OnArm_Implementation() override;
	virtual void OnDisarm_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	EGeoGemProfileEvent Event = EGeoGemProfileEvent::Equipped;

private:
	void OnGemEquipped(EPlayerClass PlayerClass, int32 SocketIndex, FName GemId);

	void OnGemCrafted(FName GemId, int32 Quantity);

	TWeakObjectPtr<UGeoGemProfileSave> Profile;
	int32 Occurrences = 0;
};
