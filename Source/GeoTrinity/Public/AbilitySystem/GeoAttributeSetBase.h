// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GeoAttributeSetBase.generated.h"

#define GAMEPLAYATTRIBUTE_BASEVALUE_GETTER(PropertyName) \
FORCEINLINE float Get##PropertyName##BaseValue() const \
{ \
return PropertyName.GetBaseValue(); \
}

// Uses macros from AttributeSet.h
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
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
	
	UPROPERTY(BlueprintReadOnly, Category = "Basic", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UGeoAttributeSetBase, Health)

	/** MaxHealth is its own attribute since GameplayEffects may modify it */
	UPROPERTY(BlueprintReadOnly, Category = "Basic", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UGeoAttributeSetBase, MaxHealth)

protected:
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);
	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
};
