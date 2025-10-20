#pragma once

#include "CoreMinimal.h"
#include "GeoShapes.h"

#include "GeoCharacter.generated.h"

class UDynamicMeshComponent;
UCLASS()
class GEOTRINITY_API AGeoCharacter : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGeoCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void UpdateCharacterLocation( float DeltaTime );
	virtual void SetupPlayerInputComponent( UInputComponent* PlayerInputComponent ) override;
	UFUNCTION()
	void Move( const FInputActionInstance& Instance );

public:
	// Called every frame
	virtual void Tick( float DeltaTime ) override;
	void ApplyCollision( const FGeoBox& Obstacle );

protected:
	FGeoBox Box;

	UPROPERTY( Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = ( AllowPrivateAccess = "true" ) )
	TObjectPtr< UDynamicMeshComponent > MeshComponent;

	FVector2D MovementInput;

public:
	UPROPERTY( Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = ( RowType = CharacterStats ) )
	FDataTableRowHandle StatsDTHandle;
};
