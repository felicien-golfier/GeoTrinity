#include "GeoPlayerState.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "GeoPlayerController.h"
#include "HUD/GeoHUD.h"
#include "Net/UnrealNetwork.h"

AGeoPlayerState::AGeoPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UGeoAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	CharacterAttributeSet = CreateDefaultSubobject<UCharacterAttributeSet>(TEXT("CharacterAttributeSet"));

	SetNetUpdateFrequency(100.0f);
}

void AGeoPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGeoPlayerState, PlayerClass);
}

void AGeoPlayerState::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		InitOverlay();
	}

	OnPawnSet.AddUniqueDynamic(this, &AGeoPlayerState::OnPlayerPawnSet);
}

void AGeoPlayerState::ClientInitialize(class AController* Controller)
{
	Super::ClientInitialize(Controller);
}

void AGeoPlayerState::OnPlayerPawnSet(APlayerState*, APawn* NewPawn, APawn*)
{
	if (IsValid(NewPawn) && !HasAuthority())
	{
		InitOverlay();
	}
}

void AGeoPlayerState::InitOverlay()
{
	if (AGeoPlayerController* GeoPlayerController = Cast<AGeoPlayerController>(GetOwningController()))
	{
		if (AGeoHUD* Hud = Cast<AGeoHUD>(GeoPlayerController->GetHUD()))
		{
			Hud->InitOverlay(GeoPlayerController, this, AbilitySystemComponent, CharacterAttributeSet);
		}
	}
}

UAbilitySystemComponent* AGeoPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

FGenericTeamId AGeoPlayerState::GetGenericTeamId() const
{
	if (IGenericTeamAgentInterface const* TeamAgent = Cast<IGenericTeamAgentInterface>(GetPawn()))
	{
		return TeamAgent->GetGenericTeamId();
	}
	return IGenericTeamAgentInterface::GetGenericTeamId();
}
