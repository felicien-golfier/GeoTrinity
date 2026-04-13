// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystemComponent.h"
#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"

#include "GeoAbilitySystemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeChangedSignature, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealProvided, float, HealDone);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDamageDealt, float, DamageAmount, FGameplayTag, AbilityTag);

class UGeoGameplayAbility;
struct FGeoGameplayEffectContext;
/**
 * Ability system component tailored for the GeoTrinity 2D top-down game.
 * Adds input-tag-driven ability activation, startup ability management per player class,
 * attribute delegate binding, and the pattern replication system for enemy bullet patterns.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	virtual void InitializeComponent() override;
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	/** Creates a GeoGameplayEffectContext pre-populated with this component's owner. */
	FGeoGameplayEffectContext* MakeGeoEffectContext() const;

	/** Grants all abilities whose tags are in AbilitiesToGive, looked up from the global UAbilityInfo asset. */
	void GiveStartupAbilities(TArray<FGameplayTag> const& AbilitiesToGive, int32 Level = 1);

	/** Grants all class-agnostic startup abilities defined in the global UAbilityInfo asset. */
	void GiveStartupAbilities(int32 Level = 1);

	/** Grants all abilities defined for PlayerClass in the global UAbilityInfo asset. */
	void GiveStartupAbilities(EPlayerClass PlayerClass, int32 Level = 1);

	/** Removes all abilities that were granted for a specific player class. */
	void ClearPlayerClassAbilities();

	/** Notifies the ASC that the input mapped to inputTag was pressed this frame. */
	void AbilityInputTagPressed(FGameplayTag const& inputTag);

	/** Notifies the ASC that the input mapped to inputTag is held this frame. */
	void AbilityInputTagHeld(FGameplayTag const& inputTag);

	/** Notifies the ASC that the input mapped to inputTag was released this frame. */
	void AbilityInputTagReleased(FGameplayTag const& inputTag);

	/** Activates ability with event data containing avatar orientation. Used for projectile abilities. */
	bool TryActivateAbilityWithTargetData(FGameplayAbilitySpecHandle Handle);

	/** Applies a gameplay effect to this component's owner at the given level. */
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, int32 Level = 1);

	/** Applies the DefaultAttributes gameplay effect to initialize base attribute values. */
	void InitializeDefaultAttributes(int32 Level = 1);

	/** Binds OnHealthChanged and OnMaxHealthChanged delegates to the Health/MaxHealth attribute change callbacks. */
	void BindAttributeCallbacks();

	/**
	 * Instantiates a UPattern of PatternClass, registers it, and calls OnCreate.
	 *
	 * @param PatternClass  The UClass of the pattern to create. Must not be null.
	 * @param AbilityTag    Tag of the ability that spawned the pattern (used for montage selection).
	 * @return              The newly created pattern instance.
	 */
	UPattern* CreatePatternInstance(UClass const* PatternClass, FGameplayTag AbilityTag);

	/**
	 * Finds an active pattern instance by class.
	 *
	 * @param PatternClass  The class to search for.
	 * @param Pattern       Output — set to the found pattern, or null if not found.
	 * @return              True if a matching pattern was found.
	 */
	bool FindPatternByClass(UClass* PatternClass, UPattern*& Pattern);

	/** Ends all currently active pattern instances managed by this component. */
	void StopAllActivePatterns();

	/**
	 * Multicast RPC that instantiates PatternClass on every client and calls InitPattern with Payload.
	 * Called by UPatternAbility on the server after activation.
	 */
	UFUNCTION(NetMulticast, reliable)
	void PatternStartMulticast(FAbilityPayload Payload, UClass* PatternClass);

	/**
	 * Returns a reference to the per-ability fire section index, used to cycle animation montage sections.
	 * The index is created at zero on first access.
	 *
	 * @param AbilityTag  Tag identifying the ability whose fire section index to retrieve.
	 */
	int32& GetFireSectionIndex(FGameplayTag const& AbilityTag);

	UPROPERTY(BlueprintAssignable)
	FOnHealProvided OnHealProvided;

	UPROPERTY(BlueprintAssignable)
	FOnDamageDealt OnDamageDealt;

	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnHealthChanged;
	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnMaxHealthChanged;

private:
	TMap<FGameplayTag, int32> FireSectionIndices;
	bool bStartupAbilitiesGiven{false};

	UPROPERTY(Transient)
	TArray<UPattern*> Patterns;

	// DATA //
	UPROPERTY(EditAnywhere, Category = "Gas")
	TSubclassOf<UGameplayEffect> DefaultAttributes;

	UPROPERTY(EditAnywhere, Category = GAS, meta = (Categories = "Ability.Spell"))
	TArray<FGameplayTag> StartupAbilityTags;
};
