#pragma once

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/InteractableComponent.h"
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
	UInteractableComponent* GetInteractableComponent() const { return InteractableComponent; }
	UGeoMovementComponent* GetGeoMovementComponent() const
	{
		return Cast<UGeoMovementComponent>(GetMovementComponent());
	}

	AGeoPlayerController* GetGeoController() const { return Cast<AGeoPlayerController>(GetController()); }

	static FColor GetColorForCharacter(const AGeoCharacter* Character);
	void DrawDebugVectorFromCharacter(const FVector& Direction, const FString& DebugMessage) const;
	void DrawDebugVectorFromCharacter(const FVector& Direction, const FString& DebugMessage, FColor Color) const;

protected:
	UPROPERTY(Category = Geo, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(Category = Geo, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGeoInputComponent> GeoInputComponent;

	UPROPERTY(Category = Geo, EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UInteractableComponent> InteractableComponent;

public:
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (RowType = CharacterStats))
	FDataTableRowHandle StatsDTHandle;
};
