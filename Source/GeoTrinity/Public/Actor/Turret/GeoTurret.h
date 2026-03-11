// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/EffectData.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "Actor/GeoInteractableActor.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GeoTurret.generated.h"

class AGeoProjectile;
class UCapsuleComponent;

USTRUCT()
struct FTurretData : public FDeployableData
{
	GENERATED_BODY()

	UPROPERTY(NotReplicated)
	TArray<TInstancedStruct<FEffectData>> EffectDataArray;
};

UCLASS(Blueprintable, ClassGroup = (Custom))
class GEOTRINITY_API AGeoTurret : public AGeoDeployableBase
{
	GENERATED_BODY()

public:
	virtual void InitInteractableData(FInteractableActorData* Data) override;
	virtual void OnRecalled() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool IsBlinking() const;
	bool WasBlinkingOnRecall() const { return bWasBlinkingOnRecall; }

protected:
	virtual FTurretData const* GetData() const override { return &Data; }

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	AActor* FindBestTarget() const;
	void ScheduleFire();
	void TryFire();

	UPROPERTY(Replicated)
	FTurretData Data;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AGeoProjectile> TurretProjectileClass;

	UPROPERTY(EditDefaultsOnly)
	float FireInterval = 1.f;

	UPROPERTY(EditDefaultsOnly)
	float BlinkThreshold = 0.2f;

private:
	FTimerHandle FireTimerHandle;
	bool bWasBlinkingOnRecall = false;
};
