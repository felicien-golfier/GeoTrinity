#pragma once

// Obstacle2D.h
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GeoShapes.h"
#include "GeoObstacle.generated.h"

UCLASS()
class AGeoObstacle : public AActor
{
  GENERATED_BODY()

public:
  FGeoBox Box;

  AGeoObstacle()
  {
    PrimaryActorTick.bCanEverTick = false;
  }
};