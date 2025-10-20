#include "GeoCharacter.h"

#include "Components/DynamicMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GeoPlayerController.h"

// Sets default values
AGeoCharacter::AGeoCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Box = FGeoBox( FVector2D( 0, 0 ), FVector2D( 50, 50 ) );   // carré 50x50

	FDynamicMesh3 MyFDynamicMesh3;
	const int Vid1 = MyFDynamicMesh3.AppendVertex( FVector( Box.Size.X / 2, Box.Size.Y / 2, 0 ) );
	const int Vid2 = MyFDynamicMesh3.AppendVertex( FVector( Box.Size.X / 2, -Box.Size.Y / 2, 0 ) );
	const int Vid3 = MyFDynamicMesh3.AppendVertex( FVector( -Box.Size.X / 2, -Box.Size.Y / 2, 0 ) );
	const int Vid4 = MyFDynamicMesh3.AppendVertex( FVector( -Box.Size.X / 2, Box.Size.Y / 2, 0 ) );
	MyFDynamicMesh3.AppendTriangle( Vid1, Vid2, Vid3 );
	MyFDynamicMesh3.AppendTriangle( Vid3, Vid4, Vid1 );
	MeshComponent = CreateDefaultSubobject< UDynamicMeshComponent >( TEXT( "Dynamic Mesh Component" ) );
	MeshComponent->SetMesh( MoveTemp( MyFDynamicMesh3 ) );
}

// Called when the game starts or when spawned
void AGeoCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AGeoCharacter::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );
	UpdateCharacterLocation( DeltaTime );
}

void AGeoCharacter::UpdateCharacterLocation( float DeltaTime )
{
	if ( MovementInput.IsZero() ) {
		return;
	}

	// Scale our movement input axis values by 100 units per second
	MovementInput = MovementInput.GetSafeNormal() * 100.0f;
	FVector NewLocation = GetActorLocation();
	NewLocation += GetActorForwardVector() * MovementInput.Y * DeltaTime;
	NewLocation += GetActorRightVector() * MovementInput.X * DeltaTime;
	Box.Position = FVector2D( NewLocation );
	SetActorLocation( NewLocation );
}

void AGeoCharacter::ApplyCollision( const FGeoBox& Obstacle )
{
	if ( !Box.Overlaps( Obstacle ) ) {
		return;
	}

	// Correction X
	if ( Box.Position.X < Obstacle.Position.X ) {
		Box.Position.X = Obstacle.Position.X - Obstacle.Size.X - Box.Size.X;
	}
	else {
		Box.Position.X = Obstacle.Position.X + Obstacle.Size.X + Box.Size.X;
	}

	// Correction Y
	if ( Box.Position.Y < Obstacle.Position.Y ) {
		Box.Position.Y = Obstacle.Position.Y - Obstacle.Size.Y - Box.Size.Y;
	}
	else {
		Box.Position.Y = Obstacle.Position.Y + Obstacle.Size.Y + Box.Size.Y;
	}
}

void AGeoCharacter::SetupPlayerInputComponent( UInputComponent* PlayerInputComponent )
{
	Super::SetupPlayerInputComponent( PlayerInputComponent );

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked< UEnhancedInputComponent >( PlayerInputComponent );
	AGeoPlayerController* GeoPlayerController = Cast< AGeoPlayerController >( Controller );
	EnhancedInputComponent->BindAction( GeoPlayerController->MoveAction, ETriggerEvent::Triggered, this, &AGeoCharacter::Move );
	EnhancedInputComponent->BindAction( GeoPlayerController->MoveAction, ETriggerEvent::Completed, this, &AGeoCharacter::Move );
}

void AGeoCharacter::Move( const FInputActionInstance& Instance )
{
	FVector2D AxisValues = Instance.GetValue().Get< FVector2D >();
	MovementInput.X = AxisValues.X;
	MovementInput.Y = AxisValues.Y;
}
