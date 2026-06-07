#include "GameClasses/GeoPlayerState.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "Characters/PlayableCharacter.h"
#include "GameClasses/GeoPlayerController.h"
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
		// PlayerClass and the pawn link replicate independently. If PlayerClass arrived before the pawn,
		// OnRep_PlayerClass already ran and bailed on a null pawn — apply class data now that the pawn exists.
		ApplyClassDataToPawn();

		// Bind pawn-dependent HUD callbacks now that the pawn (and its components) exist.
		if (AGeoPlayerController* GeoPlayerController = Cast<AGeoPlayerController>(GetOwningController()))
		{
			if (AGeoHUD* GeoHUD = GeoPlayerController->GetHUD<AGeoHUD>())
			{
				GeoHUD->BindToPawn(Cast<APlayableCharacter>(NewPawn));
			}
		}
	}
}

void AGeoPlayerState::InitOverlay()
{
	if (AGeoPlayerController* GeoPlayerController = Cast<AGeoPlayerController>(GetOwningController()))
	{
		if (AGeoHUD* GeoHUD = GeoPlayerController->GetHUD<AGeoHUD>())
		{
			GeoHUD->InitOverlay(GeoPlayerController, this, AbilitySystemComponent, CharacterAttributeSet);
		}
	}
}

void AGeoPlayerState::OnRep_PlayerClass()
{
	ApplyClassDataToPawn();

	// The class change re-grants abilities; rebuild the ability bar so it reflects the new class's ability set.
	if (AGeoPlayerController* GeoPlayerController = Cast<AGeoPlayerController>(GetOwningController()))
	{
		if (AGeoHUD* GeoHUD = GeoPlayerController->GetHUD<AGeoHUD>())
		{
			GeoHUD->BuildAbilityBar();
		}
	}
}

void AGeoPlayerState::ApplyClassDataToPawn()
{
	if (APlayableCharacter* PlayableCharacter = Cast<APlayableCharacter>(GetPawn()))
	{
		PlayableCharacter->ApplyClassData(PlayerClass);
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
