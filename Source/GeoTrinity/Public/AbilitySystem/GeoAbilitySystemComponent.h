// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"

#include "GeoAbilitySystemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeChangedSignature, float, NewValue);

class UGeoGameplayAbility;
struct FGeoGameplayEffectContext;
/*
 * Ability system component tailored for the Geo game (2D chara top down)
 * */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	virtual void InitializeComponent() override;
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	/** Create an EffectContext for the owner of this AbilitySystemComponent */
	FGeoGameplayEffectContext* MakeGeoEffectContext() const;

	/** Abilities **/
	void GiveStartupAbilities(TArray<TSubclassOf<UGeoGameplayAbility>>& AbilitiesToGive);
	void GiveStartupAbilities(TArray<FGameplayTag> const& AbilitiesToGive, int32 const Level = 1.f);
	void GiveStartupAbilities(int32 const Level = 1.f);

	/** Input **/
	void AbilityInputTagPressed(FGameplayTag const& inputTag);
	void AbilityInputTagHeld(FGameplayTag const& inputTag);
	void AbilityInputTagReleased(FGameplayTag const& inputTag);

	/** Activates ability with event data containing avatar orientation. Used for projectile abilities. */
	bool TryActivateAbilityWithTargetData(FGameplayAbilitySpecHandle Handle);

	/** Effects **/
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, int32 Level = 1);
	void InitializeDefaultAttributes(int32 Level = 1);

	/** Delegates **/
	void BindAttributeCallbacks(); // By doing that, we factorize, ok... but we also make the ASC not agnostic to

	UPattern* CreatePatternInstance(UClass const* PatternClass, FGameplayTag AbilityTag);
	bool FindPatternByClass(UClass* PatternClass, UPattern*& Pattern);
	UFUNCTION(NetMulticast, reliable)
	void PatternStartMulticast(FAbilityPayload Payload, UClass* PatternClass);

	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnHealthChanged;
	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnMaxHealthChanged;

private:
	bool bStartupAbilitiesGiven{false};

	UPROPERTY(Transient)
	TArray<UPattern*> Patterns;

	// DATA //
	UPROPERTY(EditAnywhere, Category = "Gas")
	TSubclassOf<UGameplayEffect> DefaultAttributes;

	UPROPERTY(EditAnywhere, Category = GAS, meta = (Categories = "Ability.Spell"))
	TArray<FGameplayTag> StartupAbilityTags;
};
