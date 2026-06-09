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

	InitOverlay();

	OnPawnSet.AddUniqueDynamic(this, &AGeoPlayerState::OnPlayerPawnSet);

	// On the listen-server host the pawn is possessed (and abilities granted) synchronously during PossessedBy, which
	// runs BEFORE this BeginPlay — so OnPawnSet already fired before the binding above and never will again. Drive the
	// pawn-dependent setup here for the already-set pawn. On clients the pawn replicates later, so OnPlayerPawnSet
	// (bound above) covers it and AddUnique on the inner bindings prevents a double-build if both paths run.
	if (APawn* CurrentPawn = GetPawn())
	{
		OnPlayerPawnSet(this, CurrentPawn, nullptr);
	}
}

void AGeoPlayerState::ClientInitialize(class AController* Controller)
{
	Super::ClientInitialize(Controller);
}

void AGeoPlayerState::OnPlayerPawnSet(APlayerState*, APawn* NewPawn, APawn*)
{
	// Local-player gate (not !IsServer): the listen-server host needs these HUD/visual steps too, and remote players'
	// PlayerStates living on the server must not run them. GetOwningController() returns null / a non-local controller
	// in both excluded cases.
	AGeoPlayerController* GeoPlayerController = Cast<AGeoPlayerController>(GetOwningController());
	if (!IsValid(NewPawn) || !GeoPlayerController || !GeoPlayerController->IsLocalPlayerController())
	{
		return;
	}

	InitOverlay();
	// PlayerClass and the pawn link replicate independently. If PlayerClass arrived before the pawn,
	// OnRep_PlayerClass already ran and bailed on a null pawn — apply class data now that the pawn exists.
	ApplyClassDataToPawn();

	// Bind pawn-dependent HUD callbacks now that the pawn (and its components) exist.
	if (AGeoHUD* GeoHUD = GeoPlayerController->GetHUD<AGeoHUD>())
	{
		GeoHUD->BindToPawn(Cast<APlayableCharacter>(NewPawn));
	}
}

void AGeoPlayerState::InitOverlay()
{
	// Gate on the local player controller so this runs for clients AND the listen-server host, but not for a dedicated
	// server or for other players' PlayerStates replicated onto a server.
	AGeoPlayerController* GeoPlayerController = Cast<AGeoPlayerController>(GetOwningController());
	if (GeoPlayerController && GeoPlayerController->IsLocalPlayerController())
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
	RebuildAbilityBar();
}

void AGeoPlayerState::RebuildAbilityBar()
{
	// Local-controller gate: on the listen-server host there is no OnRep_PlayerClass, so ChangeClass calls this
	// directly; on a server this PlayerState may belong to a remote player whose HUD lives elsewhere.
	AGeoPlayerController* GeoPlayerController = Cast<AGeoPlayerController>(GetOwningController());
	if (GeoPlayerController && GeoPlayerController->IsLocalPlayerController())
	{
		if (AGeoHUD* GeoHUD = GeoPlayerController->GetHUD<AGeoHUD>())
		{
			GeoHUD->BuildAbilityBar(GeoPlayerController->GetPawn<APlayableCharacter>());
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
