#pragma once

#include "Pattern.generated.h"

class UEffectDataAsset;
struct FEffectData;
class UGameplayEffect;
class AGeoProjectile;
USTRUCT(BlueprintType)
struct FPatternPayload
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

	// TODO: Find out a better solution than sending this every ability.
	UPROPERTY(Transient, BlueprintReadOnly)
	int32 AbilityLevel;

	UPROPERTY(Transient, BlueprintReadOnly)
	TSubclassOf<class UPattern> PatternClass;

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
	UFUNCTION(BlueprintNativeEvent)
	void StartPattern(const FPatternPayload& Payload);
	virtual void StartPattern_Implementation(const FPatternPayload& Payload);

	UPROPERTY(EditDefaultsOnly)
	UEffectDataAsset* EffectDataAsset;
};

UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API UProjectilePattern : public UPattern
{
	GENERATED_BODY()

public:
	virtual void StartPattern_Implementation(const FPatternPayload& Payload) override;

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectile(const FPatternPayload& Payload, float Yaw);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AGeoProjectile> ProjectileClass;
};