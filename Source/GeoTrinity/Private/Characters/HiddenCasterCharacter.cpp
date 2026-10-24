// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/HiddenCasterCharacter.h"

#include "Tool/UGeoGameplayLibrary.h"

AHiddenCasterCharacter::AHiddenCasterCharacter(FObjectInitializer const& ObjectInitializer) :
	Super(ObjectInitializer.DoNotCreateDefaultSubobject(TEXT("CharacterWidgetComponent")))
{
}

void AHiddenCasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (GeoLib::IsServer(this))
	{
		SetInvulnerable(true);
	}
}
