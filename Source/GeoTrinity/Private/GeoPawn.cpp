#include "GeoPawn.h"

#include "Components/DynamicMeshComponent.h"
#include "GeoInputComponent.h"
#include "GeoMovementComponent.h"
#include "GeoPlayerState.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"

// Sets default values
AGeoPawn::AGeoPawn()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);
	Box = FGeoBox(FVector2D(0, 0), FVector2D(50, 50));   // carré 50x50

	FDynamicMesh3 MyFDynamicMesh3;
	const int Vid1 = MyFDynamicMesh3.AppendVertex(FVector(Box.Size.X / 2, Box.Size.Y / 2, 0));
	const int Vid2 = MyFDynamicMesh3.AppendVertex(FVector(Box.Size.X / 2, -Box.Size.Y / 2, 0));
	const int Vid3 = MyFDynamicMesh3.AppendVertex(FVector(-Box.Size.X / 2, -Box.Size.Y / 2, 0));
	const int Vid4 = MyFDynamicMesh3.AppendVertex(FVector(-Box.Size.X / 2, Box.Size.Y / 2, 0));
	MyFDynamicMesh3.AppendTriangle(Vid1, Vid2, Vid3);
	MyFDynamicMesh3.AppendTriangle(Vid3, Vid4, Vid1);
	MeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("Dynamic Mesh Component"));
	MeshComponent->SetMesh(MoveTemp(MyFDynamicMesh3));
	SetRootComponent(MeshComponent);

	GeoInputComponent = CreateDefaultSubobject<UGeoInputComponent>(TEXT("Geo Input Component"));
	GeoMovementComponent = CreateDefaultSubobject<UGeoMovementComponent>(TEXT("Geo Movement Component"));
}

void AGeoPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	GeoInputComponent->BindInput(PlayerInputComponent);
}

void AGeoPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitAbilityActorInfo();
}

void AGeoPawn::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	// Set the ASC for clients. Server does this in PossessedBy.
	InitAbilityActorInfo();
}

void AGeoPawn::InitAbilityActorInfo()
{
	AGeoPlayerState* PS = GetPlayerState<AGeoPlayerState>();
	if (!PS)
		return;

	AbilitySystemComponent = Cast<UGeoAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	AbilitySystemComponent->InitAbilityActorInfo(PS, this);
}
