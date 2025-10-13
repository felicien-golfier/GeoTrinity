// Fill out your copyright notice in the Description page of Project Settings.


#include "GeoCharacter.h"
#include "CharacterStats.h"

// Sets default values
AGeoCharacter::AGeoCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Box = FGeoBox(FVector2D(0,0), FVector2D(50,50)); // carré 50x50
}

// Called when the game starts or when spawned
void AGeoCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGeoCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	FVector NewPos = FVector(Box.Position, 0);
	SetActorLocation(NewPos);
}

void AGeoCharacter::Move(FVector2D InputDir, float DeltaTime)
{
	Box.Position += InputDir * CharacterStats.Speed * DeltaTime;
}

void AGeoCharacter::ApplyCollision(const FGeoBox& Obstacle)
{
	if (!Box.Overlaps(Obstacle)) return;

	// Correction X
	if (Box.Position.X < Obstacle.Position.X)
		Box.Position.X = Obstacle.Position.X - Obstacle.Size.X - Box.Size.X;
	else
		Box.Position.X = Obstacle.Position.X + Obstacle.Size.X + Box.Size.X;

	// Correction Y
	if (Box.Position.Y < Obstacle.Position.Y)
		Box.Position.Y = Obstacle.Position.Y - Obstacle.Size.Y - Box.Size.Y;
	else
		Box.Position.Y = Obstacle.Position.Y + Obstacle.Size.Y + Box.Size.Y;
}