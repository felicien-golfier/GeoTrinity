// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoGameMode.h"

#include "Characters/GeoCharacter.h"

AGeoGameMode::AGeoGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DefaultPawnClass = AGeoCharacter::StaticClass();
}
void AGeoGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}