#pragma once

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
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

UENUM(Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ETeam : uint8
{
	Neutral = (1 << 0) UMETA(DisplayName = "Neutral"),
	Ally = (1 << 1) UMETA(DisplayName = "Ally"),
	Enemy = (1 << 2) UMETA(DisplayName = "Enemy")
};

UCLASS()
class GEOTRINITY_API AGeoCharacter
	: public ACharacter
	, public IAbilitySystemInterface
	, public IGenericTeamAgentInterface
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

	//----------------------------------------------------------------------//
	// IAbilitySystemInterface BEGIN
	//----------------------------------------------------------------------//
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//----------------------------------------------------------------------//
	// IAbilitySystemInterface END
	//----------------------------------------------------------------------//

	//----------------------------------------------------------------------//
	// IGenericTeamAgentInterface BEGIN
	//----------------------------------------------------------------------//
	/** Assigns Team Agent to given TeamID */
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override
	{
		TeamId = static_cast<ETeam>(NewTeamId.GetId());
	}

	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(static_cast<uint8>(TeamId)); };

	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	//----------------------------------------------------------------------//
	// IGenericTeamAgentInterface END
	//----------------------------------------------------------------------//

	AGeoPlayerController* GetGeoController() const { return Cast<AGeoPlayerController>(GetController()); }

	void DrawDebugVectorFromCharacter(const FVector& Direction, const FString& DebugMessage) const;
	void DrawDebugVectorFromCharacter(const FVector& Direction, const FString& DebugMessage, FColor Color) const;

protected:
	//----------------------------------------------------------------------//
	// GAS START
	//----------------------------------------------------------------------//

	// InitGAS MUST call InitAbilityActorInfo in subclasses.
	virtual void InitGAS();

	//----------------------------------------------------------------------//
	// GAS END
	//----------------------------------------------------------------------//

	UPROPERTY(Category = Geo, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGeoInputComponent> GeoInputComponent;

	UPROPERTY(Category = Geo, EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UGeoAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(Transient)
	TObjectPtr<UGeoAttributeSetBase> AttributeSetBase;

	UPROPERTY(Category = Team, EditAnywhere, BlueprintReadOnly)
	ETeam TeamId;

#ifdef UE_EDITOR
public:
	virtual void BeginPlay() override;

private:
	ENetRole LocalRoleForDebugPurpose = ROLE_None;
#endif
};
