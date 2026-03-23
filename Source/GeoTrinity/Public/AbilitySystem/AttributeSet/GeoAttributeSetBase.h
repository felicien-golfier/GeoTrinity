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
	virtual void PostGameplayEffectExecute(FGameplayEffectModCallbackData const& Data) override;

	UPROPERTY(BlueprintReadOnly, Category = "Basic", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UGeoAttributeSetBase, Health)

	/** MaxHealth is its own attribute since GameplayEffects may modify it */
	UPROPERTY(BlueprintReadOnly, Category = "Basic", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UGeoAttributeSetBase, MaxHealth)

	// Shield
	UPROPERTY(BlueprintReadOnly, Category = "Basic", ReplicatedUsing = OnRep_Shield)
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS(UGeoAttributeSetBase, Shield)

	UPROPERTY(BlueprintReadOnly, Category = "Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UGeoAttributeSetBase, IncomingDamage)

	UPROPERTY(BlueprintReadOnly, Category = "Meta")
	FGameplayAttributeData IncomingHeal;
	ATTRIBUTE_ACCESSORS(UGeoAttributeSetBase, IncomingHeal)


	UFUNCTION(BlueprintCallable, Category = "Attribute")
	float GetHealthRatio() const;

protected:
	UFUNCTION()
	virtual void OnRep_Health(FGameplayAttributeData const& OldHealth);
	UFUNCTION()
	virtual void OnRep_MaxHealth(FGameplayAttributeData const& OldMaxHealth);
	UFUNCTION()
	virtual void OnRep_Shield(FGameplayAttributeData const& OldShield);
};
