#pragma once

#include "CoreMinimal.h"
#include "GeoShapes.h"

#include "GeoPawn.generated.h"

class UGeoInputComponent;
class UDynamicMeshComponent;
UCLASS()
class GEOTRINITY_API AGeoPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGeoPawn();

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

public:
	FGeoBox GetBox() const { return Box; }
	UGeoInputComponent* GetGeoInputComponent() const { return GeoInputComponent; }

protected:
	FGeoBox Box;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDynamicMeshComponent> MeshComponent;

	UPROPERTY(Category = Geo, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGeoInputComponent> GeoInputComponent;

public:
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (RowType = CharacterStats))
	FDataTableRowHandle StatsDTHandle;
};
