// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "CoreMinimal.h"

#include "GeoReloadAbility.generated.h"

class AGeoBuffPickup;
class UEffectDataAsset;

/**
 * Triangle reload ability: after a delay equal to FireDelay, restores ammo and spawns a buff pickup.
 * PowerScale = MissingAmmo / MaxAmmo — controls pickup size and effect magnitude.
 * A random buff is chosen from BuffEffectDataAssets each time.
 */
UCLASS()
class GEOTRINITY_API UGeoReloadAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	/** Returns BuffColors[Index] wrapped by the palette size. White when no colors are configured. Read from the CDO by
	 * the spawned pickup (via its buff index) to tint its mesh. */
	FLinearColor GetColorForIndex(int32 Index) const;

	/**
	 * Maps an ammo count to a buff color the same way Fire() maps it to a buff index, so the HUD ammo number can preview
	 * the color the next reload will spawn. Static so the overlay needs only the ammo value: it resolves the reload
	 * ability CDO itself via the Ability.Type.Reload tag. White when the CDO or palette is missing.
	 */
	UFUNCTION(BlueprintPure, Category = "Ability|Reload")
	static FLinearColor GetColorForAmmo(int32 Ammo);

	/** Public so AGeoGameState::Loot() can spawn the same pickups from this CDO without duplicating the config. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Reload")
	TSubclassOf<AGeoBuffPickup> BuffPickupClass;

protected:
	/** Binds the ammo-changed callback so the reload button is grayed out in the HUD when ammo is full. */
	virtual void OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec) override;

	/** Returns false when the character already has maximum ammo (reload is a no-op in that state). */
	virtual bool CheckCost(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
						   OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	/** Restores ammo via AmmoRestoreEffect and spawns a buff pickup at a random location near the character. */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	/**
	 * Clamps DesiredOffset so the pickup lands somewhere the player can walk to: traces from Origin toward
	 * Origin + DesiredOffset on the GeoCharacter channel (what a character collides with) and, on a hit, pulls the
	 * offset back to the hit point minus PickupRadius. Returns DesiredOffset unchanged when the path is clear.
	 */
	FVector GetReachableSpawnOffset(FVector Origin, FVector DesiredOffset) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Reload")
	TSubclassOf<UGameplayEffect> AmmoRestoreEffect;

	/** Per-buff colors, indexed in parallel with the merged buff effect array (see GetEffectDataArray). The pickup is
	 * tinted with the entry matching the chosen buff; the HUD ammo number uses the same palette via GetColorForAmmo. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Reload")
	TArray<FLinearColor> BuffColors;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Reload")
	float MinSpawnRadius = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Reload")
	float MaxSpawnRadius = 400.f;

	/** Pickup half-width kept clear of a blocking wall when the spawn offset is pulled back to a reachable point. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Reload")
	float PickupRadius = 50.f;
};
