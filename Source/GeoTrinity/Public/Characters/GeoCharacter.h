#pragma once

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "GeoMovementComponent.h"
#include "GeoPlayerController.h"

#include "GeoCharacter.generated.h"

UENUM(Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ETeam : uint8
{
	Neutral = (1 << 0) UMETA(DisplayName = "Neutral"),
	Ally = (1 << 1) UMETA(DisplayName = "Ally"),
	Enemy = (1 << 2) UMETA(DisplayName = "Enemy")
};

ENUM_CLASS_FLAGS(ETeam);

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
class GEOTRINITY_API AGeoCharacter
	: public ACharacter
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

	AGeoPlayerController* GetGeoController() const { return Cast<AGeoPlayerController>(GetController()); }

	static FColor GetColorForCharacter(const AGeoCharacter* Character);
	void DrawDebugVectorFromCharacter(const FVector& Direction, const FString& DebugMessage) const;
	void DrawDebugVectorFromCharacter(const FVector& Direction, const FString& DebugMessage, FColor Color) const;

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

protected:
	UFUNCTION(BlueprintCallable, Category = "GAS")
	void BP_ApplyEffectToSelfDefaultLvl(TSubclassOf<UGameplayEffect> gameplayEffectClass);

private:
	virtual void PossessedBy(AController* NewController) override;

public:
	// GAS
	virtual void InitAbilityActorInfo();
	virtual void InitAbilityActorInfo(UGeoAbilitySystemComponent* GeoAbilitySystemComponent, AActor* OwnerActor,
		UCharacterAttributeSet* GeoAttributeSetBase);
	void InitializeDefaultAttributes();
	void AddCharacterDefaultAbilities();
	UFUNCTION(Server, Reliable)
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> gameplayEffectClass, float level);

private:
	UPROPERTY(Category = Team, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	ETeam TeamId;

protected:
	UPROPERTY(Category = Geo, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(Category = Geo, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGeoInputComponent> GeoInputComponent;

	UPROPERTY(BlueprintReadOnly, Category = Character)
	TObjectPtr<UGeoAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(BlueprintReadOnly, Category = GAS)
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
