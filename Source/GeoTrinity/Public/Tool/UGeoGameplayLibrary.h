#pragma once
#include "GameplayPrediction.h"
#include "GenericTeamAgentInterface.h"
#include "StructUtils/InstancedStruct.h"

#include "UGeoGameplayLibrary.generated.h"

// Should be used as default value to spawn projectiles / characters etc... Also should be Playable Character's half
// capsule height.
constexpr float ArbitraryCharacterZ = 50.0f;


class AGeoProjectile;
class UCameraShakeBase;
struct FAbilityPayload;
struct FEffectData;

UCLASS()

class UGeoGameplayLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary", meta = (DefaultToSelf = "WorldContextObject"))
	static FColor GetColorForObject(UObject const* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary", meta = (DefaultToSelf = "WorldContextObject"))
	static bool IsServer(UObject const* WorldContextObject);

	/** Triggers a camera shake on the local player controller. Safe to call from any machine — no-op on dedicated
	 * server. */
	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary", meta = (DefaultToSelf = "WorldContextObject"))
	static void TriggerCameraShake(UObject const* WorldContextObject, TSubclassOf<UCameraShakeBase> ShakeClass,
								   float Scale = 1.f);
	static bool IsServer(UWorld const* World);

	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary", meta = (DefaultToSelf = "WorldContextObject"))
	static float GetServerTime(UObject const* WorldContextObject, bool bUpdatedWithPing = false);
	static float GetServerTime(UWorld const* World, bool bUpdatedWithPing = false);
};

using GeoLib = UGeoGameplayLibrary;
