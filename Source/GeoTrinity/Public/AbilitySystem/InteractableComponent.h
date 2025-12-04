// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GeoGameplayAbility.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"

#include "InteractableComponent.generated.h"

class UGeoAttributeSetBase;
class UGeoAbilitySystemComponent;
class UCharacterAttributeSet;
class UAttributeSet;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGasAttributeChangedSignature, float, NewValue);

UENUM(Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ETeam : uint8
{
	Neutral = (1 << 0) UMETA(DisplayName = "Neutral"),
	Ally = (1 << 1) UMETA(DisplayName = "Ally"),
	Enemy = (1 << 2) UMETA(DisplayName = "Enemy")
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UInteractableComponent
	: public UActorComponent
	, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UInteractableComponent();

	virtual void BeginPlay() override;
	//----------------------------------------------------------------------//
	// IGenericTeamAgentInterface START
	//----------------------------------------------------------------------//
	/** Assigns Team Agent to given TeamID */
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override
	{
		TeamId = static_cast<ETeam>(NewTeamId.GetId());
	}

	/** Retrieve team identifier in form of FGenericTeamId */
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(static_cast<uint8>(TeamId)); }

	/** Retrieved owner attitude toward given Other object */
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

	//----------------------------------------------------------------------//
	// IGenericTeamAgentInterface END
	//----------------------------------------------------------------------//

	//----------------------------------------------------------------------//
	// GAS START
	//----------------------------------------------------------------------//
public:
	void BindGasCallbacks();
	virtual void InitGas(UGeoAbilitySystemComponent* GeoAbilitySystemComponent, AActor* OwnerActor,
		UGeoAttributeSetBase* NewAttributeSet);
	virtual void InitAbilityActorInfo(UGeoAbilitySystemComponent* GeoAbilitySystemComponent, AActor* OwnerActor,
		UGeoAttributeSetBase* NewAttributeSet);
	void InitializeDefaultAttributes() const;
	void AddCharacterDefaultAbilities();

	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> gameplayEffectClass, float level) const;

	// Getter for base attribute set (for HUD/UI and other systems needing UAttributeSet*)
	UFUNCTION(BlueprintCallable, Category = "GAS")
	UAttributeSet* GetAttributeSet() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gas")
	bool bInitGasAtBeginPlay = true;

	UPROPERTY(BlueprintReadOnly, Category = "Gas")
	TObjectPtr<UGeoAbilitySystemComponent> AbilitySystemComponent;

	/** Delegates **/
	UPROPERTY(BlueprintAssignable, Category = "Gas")
	FOnGasAttributeChangedSignature OnHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = "Gas")
	FOnGasAttributeChangedSignature OnMaxHealthChanged;

protected:
	UFUNCTION(BlueprintCallable, Category = "GAS")
	void BP_ApplyEffectToSelfDefaultLvl(TSubclassOf<UGameplayEffect> gameplayEffectClass);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gas")
	TSubclassOf<UGameplayEffect> DefaultAttributes;

	UPROPERTY(BlueprintReadOnly, Category = "Gas")
	TObjectPtr<UGeoAttributeSetBase> AttributeSet;

	// TODO: could be auto by tag
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GAS)
	TArray<TSubclassOf<UGeoGameplayAbility>> StartupAbilities;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GAS)
	TArray<FGameplayTag> StartupAbilityTags;
	//----------------------------------------------------------------------//
	// GAS END
	//----------------------------------------------------------------------//

private:
	UPROPERTY(Category = Team, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	ETeam TeamId;
};
