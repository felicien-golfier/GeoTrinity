// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayEffectTypes.h"
#include "GeoAscTypes.generated.h"

/**
 * A file to gather different structures essential for our ASC
 */

class UGameplayEffect;

USTRUCT(BlueprintType)
struct FDamageEffectParams
{
	GENERATED_BODY()
	FDamageEffectParams(){}

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UObject> WorldContextObject = nullptr;
	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UGameplayEffect>DamageGameplayEffectClass = nullptr;
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> SourceASC = nullptr;
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> TargetASC = nullptr;
	UPROPERTY(BlueprintReadWrite)
	float BaseDamage = 0.f;
	UPROPERTY(BlueprintReadWrite)
	float AbilityLevel = 1.f;
	UPROPERTY(BlueprintReadWrite)
	uint8 StatusChance = 0;
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag StatusTag{};
	
	UPROPERTY(BlueprintReadWrite)
	float DeathImpulseMagnitude = 0.f;
	UPROPERTY(BlueprintReadWrite)
	FVector DeathImpulseVector{FVector::ZeroVector};
	
	UPROPERTY(BlueprintReadWrite)
	uint8 KnockbackChancePercent = 0;
	UPROPERTY(BlueprintReadWrite)
	float KnockbackMagnitude = 0.f;
	UPROPERTY(BlueprintReadWrite)
	FVector KnockbackVector{FVector::ZeroVector};
	
	UPROPERTY(BlueprintReadWrite)
	bool bIsRadialDamage {false};
	UPROPERTY(BlueprintReadWrite)
	float RadialDamageInnerRadius {0.f};
	UPROPERTY(BlueprintReadWrite)
	float RadialDamageOuterRadius {0.f};
	UPROPERTY(BlueprintReadWrite)
	FVector RadialDamageOrigin {FVector::ZeroVector};
};


// ---------------------------------------------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FGeoGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	bool IsCriticalHit() const{return bIsCriticalHit;}
	bool IsBlockedHit() const{return bIsBlockedHit;}
	FGameplayTag GetStatusTag() const{return StatusTag;}
	bool GetIsSuccessfulDebuff() const{return StatusTag.IsValid();}
	float GetDebuffDamage() const{return DebuffDamage;}
	float GetDebuffDuration() const{return DebuffDuration;}
	float GetDebuffFrequency() const{return DebuffFrequency;}
	FVector const& GetDeathImpulseVector() const {return DeathImpulseVector;}
	FVector const& GetKnockbackVector() const {return KnockbackVector;}
	bool GetIsRadialDamage() const {return bIsRadialDamage;}
	float GetRadialDamageInnerRadius() const {return RadialDamageInnerRadius;}
	float GetRadialDamageOuterRadius() const {return RadialDamageOuterRadius;}
	FVector const& GetRadialDamageOrigin() const {return RadialDamageOrigin;}
	
	void SetIsBlockedHit(bool isBlockedHit)	{bIsBlockedHit = isBlockedHit;	}
	void SetIsCriticalHit(bool isCriticalHit)	{bIsCriticalHit = isCriticalHit;	}
	void SetStatusTag(FGameplayTag statusTag)	{StatusTag = statusTag;	}
	void SetDebuffDamage(float value)	{DebuffDamage = value;	}
	void SetDebuffDuration(float value)	{ DebuffDuration = value;	}
	void SetDebuffFrequency(float value)	{ DebuffFrequency = value;	}
	void SetDamageType(FGameplayTag const& damageType, bool bMakeNew = false);
	void SetDeathImpulseVector(FVector const& inVector) { DeathImpulseVector = inVector;	}
	void SetKnockbackVector(FVector const& inVector) { KnockbackVector = inVector;	}
	void SetIsRadialDamage(bool value)	{ bIsRadialDamage = value;	}
	void SetRadialDamageInnerRadius(float value) { RadialDamageInnerRadius = value;	}
	void SetRadialDamageOuterRadius(float value) { RadialDamageOuterRadius = value;	}
	void SetRadialDamageOrigin(FVector const& inVector) { RadialDamageOrigin = inVector;	}
	
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FGeoGameplayEffectContext::StaticStruct();
	}
	virtual FGeoGameplayEffectContext* Duplicate() const override;

	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;
	
protected:
	UPROPERTY()
	bool bIsBlockedHit{false};
	UPROPERTY()
	bool bIsCriticalHit{false};
	UPROPERTY()
	FGameplayTag StatusTag {};
	UPROPERTY()
	float DebuffDamage{0.f};
	UPROPERTY()
	float DebuffDuration{0.f};
	UPROPERTY()
	float DebuffFrequency{0.f};
	UPROPERTY()
	FVector DeathImpulseVector{FVector::ZeroVector};
	UPROPERTY()
	FVector KnockbackVector{FVector::ZeroVector};
	
	UPROPERTY()
	bool bIsRadialDamage {false};
	UPROPERTY()
	float RadialDamageInnerRadius {0.f};
	UPROPERTY()
	float RadialDamageOuterRadius {0.f};
	UPROPERTY()
	FVector RadialDamageOrigin {FVector::ZeroVector};
};

// ---------------------------------------------------------------------------------------------------------------------
template<>
struct TStructOpsTypeTraits< FGeoGameplayEffectContext > : public TStructOpsTypeTraitsBase2< FGeoGameplayEffectContext >
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true		// Necessary so that TSharedPtr<FHitResult> Data is copied around
	};
};

