// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GeoGameFeelComponent.generated.h"

class AGeoProjectile;
class UGeoAbilitySystemComponent;
class UMeshComponent;
class UMaterialInterface;
class UNiagaraComponent;
class UNiagaraSystem;
struct FGeoBuffVFXEntry;

/**
 * Centralizes cosmetic game feel reactions (hit flash, recoil, buff VFX) for any actor.
 * Add to AGeoCharacter and AGeoInteractableActor subclasses; AGeoProjectile creates one natively.
 * Auto-discovers the owner's first mesh on BeginPlay.
 */
UCLASS(ClassGroup = "GeoTrinity", meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoGameFeelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Enables tick and initializes default values. */
	UGeoGameFeelComponent();

	/** Discovers and caches the owner's first mesh for hit-flash and recoil application. */
	virtual void BeginPlay() override;
	/** Springs the recoil offset back toward the mesh's resting position. */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	/** Flashes the owner's mesh with HitFlashMaterial for HitFlashDuration seconds. Uses LocalPlayerHitFlashMaterial
	 * when owner is the local player. */
	void FlashOnHit();

	/**
	 * Kicks the mesh backward opposite to Yaw by Distance cm, then springs back automatically.
	 * No-op on a pawn this machine doesn't control, whose mesh relative location belongs to network smoothing.
	 *
	 * @param Distance  How far the mesh snaps back in cm.
	 */
	void ApplyRecoil(float Distance);

	/**
	 * Returns true when enough time has passed since the last heal (bIsHeal) or damage GameplayCue to fire a new one.
	 * Records the current time on success so subsequent calls within the rate window return false.
	 * Call server-side only — the rate state is not replicated.
	 */
	bool IsCueAvailable(bool bIsHeal);

	/**
	 * Reads this owner's buff VFX off SourceASC from now on: listens to every UGameDataSettings::BuffVFX attribute so a
	 * buff appearing or expiring re-dresses the owner, then shows whatever is already boosted.
	 *
	 * SourceASC is who the buffs belong to, not who wears them — a character passes its own, a projectile passes its
	 * shooter's, since a shot has no attributes of its own. The owner then wears each entry's CharacterVFX, or its
	 * ProjectileVFX when the shot carries the effect that attribute scales.
	 *
	 * Idempotent: call it again on the same ASC (a second InitGAS, a pooled shot refired by the same character) to
	 * re-evaluate without touching the bindings. Passing a different ASC clears the previous one first.
	 */
	void BindBuffVFX(UGeoAbilitySystemComponent* SourceASC);

	/** Stops listening and destroys every buff VFX. Niagara keeps simulating through a hidden actor, so a pooled owner
	 * must be cleared on release or the next reuse renders the previous one's buff. */
	void ClearBuffVFX();

	UPROPERTY(EditDefaultsOnly, Category = "GeoGameFeel", meta = (ClampMin = "0"))
	float RecoilRecoverySpeed = 14.f;

private:
	/** Matches the VFX on this owner to the attributes currently above their base value on BuffSourceASC. Leaves
	 * already-correct systems running, so every path that can change the answer just calls it. */
	void RefreshBuffVFX();

	/**
	 * The system Entry shows on this owner. A character wears CharacterVFX for any boosted attribute. A projectile only
	 * carries the two boosts a shot can actually express — damage and applied heal — and only when it carries that
	 * effect; every other attribute shows nothing on a shot. Null when the entry has nothing to show here.
	 */
	UNiagaraSystem* GetBuffVFXSystem(FGeoBuffVFXEntry const& Entry) const;

	/** Adds or removes System, leaving an already-correct one running. */
	void SetBuffVFX(UNiagaraSystem* System, bool bShow);

	UPROPERTY()
	TObjectPtr<UMeshComponent> TargetMesh;

	/** Buff VFX currently playing, matched by asset since that is what an entry resolves to. */
	UPROPERTY()
	TArray<TObjectPtr<UNiagaraComponent>> BuffVFXComponents;

	/** Whose buffs this owner shows. Weak: a projectile outliving its shooter must not keep that ASC alive. */
	TWeakObjectPtr<UGeoAbilitySystemComponent> BuffSourceASC;

	FTimerHandle HitFlashTimerHandle;

	double LastDamageCueTime = 0.0;
	double LastHealCueTime = 0.0;

	FVector CurrentRecoilOffset = FVector::ZeroVector;
	FVector InitialMeshRelativeLocation = FVector::ZeroVector;
};
