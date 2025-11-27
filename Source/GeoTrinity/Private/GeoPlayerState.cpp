// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoPlayerState.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/InteractableComponent.h"
#include "GeoPlayerController.h"
#include "HUD/GeoHUD.h"

AGeoPlayerState::AGeoPlayerState()
{
	// Create ASC, and set it to be explicitly replicated
	AbilitySystemComponent = CreateDefaultSubobject<UGeoAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// If replication is not sufficient in this mode, adapt. (From what I remember, the replication can anyway be
	// tailored in the Abilities/Gameplay effects)
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// Adding an attribute set as a subobject of the owning actor of an AbilitySystemComponent
	// automatically registers the AttributeSet with the AbilitySystemComponent
	AttributeSetBase = CreateDefaultSubobject<UCharacterAttributeSet>(TEXT("AttributeSetBase"));

	SetNetUpdateFrequency(100.0f);
}
void AGeoPlayerState::BeginPlay()
{
	Super::BeginPlay();
	if (IsValid(GetPawn()))
	{
		InitializeInteractableComponent();
	}
	else
	{
		OnPawnSet.AddUniqueDynamic(this, &AGeoPlayerState::OnPlayerPawnSet);
	}
}

void AGeoPlayerState::ClientInitialize(class AController* Controller)
{
	Super::ClientInitialize(Controller);
}

void AGeoPlayerState::OnPlayerPawnSet(APlayerState*, APawn* NewPawn, APawn*)
{
	if (IsValid(NewPawn))
	{
		InitializeInteractableComponent();
	}
}

void AGeoPlayerState::InitializeInteractableComponent()
{
	APawn* Pawn = GetPawn();
	checkf(Pawn, TEXT("Player pawn is invalid at init"));
	UInteractableComponent* InteractableComponent = Pawn->GetComponentByClass<UInteractableComponent>();
	if (IsValid(InteractableComponent))
	{
		if (Pawn->HasAuthority())
		{
			InteractableComponent->InitGas(AbilitySystemComponent, Pawn, AttributeSetBase);
		}
		else
		{
			InteractableComponent->InitAbilityActorInfo(AbilitySystemComponent, Pawn, AttributeSetBase);
			InitOverlay();
		}
	}
}

void AGeoPlayerState::InitOverlay()
{
	if (AGeoPlayerController* GeoPlayerController = Cast<AGeoPlayerController>(GetOwningController()))
	{
		// Hud only present locally
		if (AGeoHUD* Hud = Cast<AGeoHUD>(GeoPlayerController->GetHUD()))
		{
			Hud->InitOverlay(GeoPlayerController, this, AbilitySystemComponent, AttributeSetBase);
		}
	}
}

UAbilitySystemComponent* AGeoPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
