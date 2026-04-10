#include "GeoPlayerState.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "Characters/PlayableCharacter.h"
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
	DOREPLIFETIME(AGeoPlayerState, DebugDPS);
	DOREPLIFETIME(AGeoPlayerState, DebugHPS);
	DOREPLIFETIME(AGeoPlayerState, DebugRecv);
	DOREPLIFETIME(AGeoPlayerState, TotalDamageDealt);
	DOREPLIFETIME(AGeoPlayerState, TotalHealingDealt);
	DOREPLIFETIME(AGeoPlayerState, TotalDamageReceived);
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

void AGeoPlayerState::OnRep_PlayerClass()
{
	APlayableCharacter* PlayableCharacter = Cast<APlayableCharacter>(GetPawn());
	if (!PlayableCharacter)
	{
		return;
	}
	PlayableCharacter->ApplyClassData(PlayerClass);
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
