#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "GeoCharacter.generated.h"

struct FGameplayTag;
class UGameplayEffect;
class UGeoAttributeSetBase;
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
	void UpdateAimRotation(float DeltaSeconds);
	UGeoInputComponent* GetGeoInputComponent() const { return GeoInputComponent; }
	UGeoMovementComponent* GetGeoMovementComponent() const { return GeoMovementComponent; }

	static FColor GetColorForCharacter(const AGeoCharacter* Character);

protected:
	UFUNCTION(BlueprintCallable, Category = "GAS")
	void BP_ApplyEffectToSelfDefaultLvl(TSubclassOf<UGameplayEffect> gameplayEffectClass);

private:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

public:
	virtual void Tick(float DeltaSeconds) override;

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

private:
	// Aim rotation cache to throttle RPCs
	float CachedAimYaw = 0.f;
	float LastSentAimYaw = 0.f;
	float TimeSinceLastAimSend = 0.f;
};
