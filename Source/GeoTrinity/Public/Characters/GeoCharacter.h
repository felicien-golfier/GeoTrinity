#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GeoMovementComponent.h"
#include "GeoPlayerController.h"

#include "GeoCharacter.generated.h"

class UCharacterAttributeSet;
class UGeoGameplayAbility;
struct FGameplayTag;
class UGameplayEffect;
class UGeoAbilitySystemComponent;
class UGeoInputComponent;
class UDynamicMeshComponent;
class UGeoMovementComponent;
class UStaticMeshComponent;
UCLASS()
class GEOTRINITY_API AGeoCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGeoCharacter(const FObjectInitializer& ObjectInitializer);
	UGeoInputComponent* GetGeoInputComponent() const { return GeoInputComponent; }
	UGeoMovementComponent* GetGeoMovementComponent() const
	{
		return Cast<UGeoMovementComponent>(GetMovementComponent());
	}

	AGeoPlayerController* GetGeoController() const { return Cast<AGeoPlayerController>(GetController()); }

	static FColor GetColorForCharacter(const AGeoCharacter* Character);
	void DrawDebugVectorFromCharacter(const FVector& Direction, const FString& DebugMessage) const;
	void DrawDebugVectorFromCharacter(const FVector& Direction, const FString& DebugMessage, FColor Color) const;

protected:
	UFUNCTION(BlueprintCallable, Category = "GAS")
	void BP_ApplyEffectToSelfDefaultLvl(TSubclassOf<UGameplayEffect> gameplayEffectClass);

private:
	virtual void PossessedBy(AController* NewController) override;

public:
	// GAS
	virtual void InitAbilityActorInfo();
	void InitializeDefaultAttributes();
	void AddCharacterDefaultAbilities();
	UFUNCTION(Server, Reliable)
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> gameplayEffectClass, float level);

protected:
	UPROPERTY(Category = Geo, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(Category = Geo, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGeoInputComponent> GeoInputComponent;

	UPROPERTY(BlueprintReadOnly, Category = Character)
	TObjectPtr<UGeoAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = GAS)
	TObjectPtr<UCharacterAttributeSet> AttributeSet;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = GAS)
	TSubclassOf<UGameplayEffect> DefaultAttributes;

	// TODO: create the data asset that ID'es each Ability by a unique tag
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GAS)
	// TArray<FGameplayTag> StartupAbilityInputTags;

	// In the mean time, for ease of use:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GAS)
	TArray<TSubclassOf<UGeoGameplayAbility>> StartupAbilities;

public:
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (RowType = CharacterStats))
	FDataTableRowHandle StatsDTHandle;
};
