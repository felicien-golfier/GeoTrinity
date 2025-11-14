#pragma once

#include "CoreMinimal.h"

#include "GeoPawn.generated.h"

struct FGameplayTag;
class UGameplayEffect;
class UGeoAttributeSetBase;
class UGeoAbilitySystemComponent;
class UGeoInputComponent;
class UDynamicMeshComponent;
class UGeoMovementComponent;
UCLASS()
class GEOTRINITY_API AGeoPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGeoPawn();
	UGeoInputComponent* GetGeoInputComponent() const { return GeoInputComponent; }
	UGeoMovementComponent* GetGeoMovementComponent() const { return GeoMovementComponent; }

	static FColor GetColorForPawn(const AGeoPawn* Pawn);

protected:
	UFUNCTION(BlueprintCallable, Category = "GAS")
	void BP_ApplyEffectToSelfDefaultLvl(TSubclassOf<UGameplayEffect> gameplayEffectClass);

private:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	// GAS
	void InitAbilityActorInfo();
	void InitializeDefaultAttributes();
	UFUNCTION(Server, Reliable)
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> gameplayEffectClass, float level);

	// GAS - Input callbacks
	void AbilityInputTagPressed(FGameplayTag inputTag);
	void AbilityInputTagReleased(FGameplayTag inputTag);
	void AbilityInputTagHeld(FGameplayTag inputTag);

private:
	UPROPERTY(Category = Geo, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(Category = Geo, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGeoInputComponent> GeoInputComponent;

	UPROPERTY(Category = Geo, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGeoMovementComponent> GeoMovementComponent;

protected:
	UPROPERTY(BlueprintReadOnly, Category = Character)
	TObjectPtr<UGeoAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = GAS)
	TObjectPtr<UGeoAttributeSetBase> AttributeSet;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = GAS)
	TSubclassOf<UGameplayEffect> DefaultAttributes;

public:
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (RowType = CharacterStats))
	FDataTableRowHandle StatsDTHandle;
};
