// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CoreMinimal.h"

#include "GeoAttributeSetBase.generated.h"

#define GAMEPLAYATTRIBUTE_BASEVALUE_GETTER(PropertyName)   \
	FORCEINLINE float Get##PropertyName##BaseValue() const \
	{                                                      \
		return PropertyName.GetBaseValue();                \
	}

// Uses macros from AttributeSet.h
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)   \
	ATTRIBUTE_ACCESSORS_BASIC(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_BASEVALUE_GETTER(PropertyName)

/**
 * Attribute set that holds RPG stats for a pawn
 */
UCLASS()
class GEOTRINITY_API UGeoAttributeSetBase : public UAttributeSet
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Clamps Health and Shield to [invulnerability floor, MaxHealth] before any modification lands. */
	virtual void PreAttributeChange(FGameplayAttribute const& Attribute, float& NewValue) override;

	/** The same floor on the base value, which the damage and heal paths write directly: without it the base drifts
	 * below the clamped current value and the character snaps down at the next aggregator re-evaluation. */
	virtual void PreAttributeBaseChange(FGameplayAttribute const& Attribute, float& NewValue) const override;

	/**
	 * Applies IncomingDamage and IncomingHeal meta attributes to Health, clamps to [0, MaxHealth],
	 * and reports damage/healing to UGeoCombatStatsSubsystem. Ends the owning actor's life at zero health.
	 */
	virtual void PostGameplayEffectExecute(FGameplayEffectModCallbackData const& Data) override;

	UPROPERTY(BlueprintReadOnly, Category = "GeoBasic", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UGeoAttributeSetBase, Health)

	/** MaxHealth is its own attribute since GameplayEffects may modify it */
	UPROPERTY(BlueprintReadOnly, Category = "GeoBasic", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UGeoAttributeSetBase, MaxHealth)

	// Shield
	UPROPERTY(BlueprintReadOnly, Category = "GeoBasic", ReplicatedUsing = OnRep_Shield)
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS(UGeoAttributeSetBase, Shield)

	UPROPERTY(BlueprintReadOnly, Category = "GeoMeta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UGeoAttributeSetBase, IncomingDamage)

	UPROPERTY(BlueprintReadOnly, Category = "GeoMeta")
	FGameplayAttributeData IncomingHeal;
	ATTRIBUTE_ACCESSORS(UGeoAttributeSetBase, IncomingHeal)


	/** Returns Health / MaxHealth. Returns 0 when MaxHealth is zero. */
	UFUNCTION(BlueprintCallable, Category = "GeoAttribute")
	float GetHealthRatio() const;

protected:
	/**
	 * Lowest value Attribute may be brought to: zero normally, its own value while the avatar is invulnerable — an
	 * invulnerable character loses no health or shield, whichever route the modification took. bBaseValue selects
	 * which value that is, so the base is floored against the base and never ratchets up to a buffed current value.
	 */
	float GetInvulnerabilityFloor(FGameplayAttribute const& Attribute, bool bBaseValue) const;

	UFUNCTION()
	virtual void OnRep_Health(FGameplayAttributeData const& OldHealth);
	UFUNCTION()
	virtual void OnRep_MaxHealth(FGameplayAttributeData const& OldMaxHealth);
	UFUNCTION()
	virtual void OnRep_Shield(FGameplayAttributeData const& OldShield);
};
