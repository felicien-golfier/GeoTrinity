#pragma once
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "Pattern.generated.h"

struct FGameplayTagContainer;
class UEffectDataAsset;
class UGameplayEffect;
class AGeoProjectile;
USTRUCT(BlueprintType)
struct FAbilityPayload
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly)
	FVector2D Origin;   // position X,Y

	UPROPERTY(Transient, BlueprintReadOnly)
	float Yaw;   // orientation

	UPROPERTY(Transient, BlueprintReadOnly)
	double ServerSpawnTime;   // server world time (seconds)

	UPROPERTY(Transient, BlueprintReadOnly)
	int32 Seed;   // seed pour variations RNG

	UPROPERTY(Transient, BlueprintReadOnly)
	int32 AbilityLevel;

	UPROPERTY(Transient, BlueprintReadOnly)
	TSubclassOf<class UPattern> PatternClass;

	// TODO: optimise AbilityTag : remove from payload and set only once on Pattern Creation.
	UPROPERTY(Transient, BlueprintReadOnly)
	FGameplayTag AbilityTag;

	UPROPERTY(Transient, BlueprintReadOnly)
	AActor* Owner;

	UPROPERTY(Transient, BlueprintReadOnly)
	AActor* Instigator;
};

UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API UPattern : public UObject
{
	GENERATED_BODY()

public:
	void OnCreate(FGameplayTag AbilityTag);

	UFUNCTION(BlueprintNativeEvent)
	void StartPattern(const FAbilityPayload& Payload);
	virtual void StartPattern_Implementation(const FAbilityPayload& Payload);

	TArray<TInstancedStruct<struct FEffectData>> EffectDataArray;
};

UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API UProjectilePattern : public UPattern
{
	GENERATED_BODY()

public:
	virtual void StartPattern_Implementation(const FAbilityPayload& Payload) override;

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectile(const FAbilityPayload& Payload, float Yaw);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AGeoProjectile> ProjectileClass;
};