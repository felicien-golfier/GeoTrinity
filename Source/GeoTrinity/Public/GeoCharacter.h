// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GeoShapes.h"
#include "GeoCharacter.generated.h"


class UDynamicMesh;UCLASS()
class GEOTRINITY_API AGeoCharacter : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGeoCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void Move(FVector2D InputDir, float DeltaTime);
	void ApplyCollision(const FGeoBox& Obstacle);
private:
	FGeoBox Box;
	
	/** The main skeletal mesh associated with this Character (optional sub-object). */
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UDynamicMesh> Mesh;
	
};
