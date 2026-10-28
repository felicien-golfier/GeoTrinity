// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/HiddenCasterCharacter.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Tool/UGeoGameplayLibrary.h"

AHiddenCasterCharacter::AHiddenCasterCharacter(FObjectInitializer const& ObjectInitializer) :
	Super(ObjectInitializer.DoNotCreateDefaultSubobject(TEXT("CharacterWidgetComponent")))
{
	AbilitySystemComponent->DefaultAttributes = UGameplayEffect::StaticClass();
}

void AHiddenCasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (GeoLib::IsServer(this))
	{
		SetInvulnerable(true);
	}
}
