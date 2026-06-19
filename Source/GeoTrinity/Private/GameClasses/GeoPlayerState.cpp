#include "GameClasses/GeoPlayerState.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "Characters/PlayableCharacter.h"
#include "GameClasses/GeoPlayerController.h"
#include "GameFramework/HUD.h"
#include "HUD/Interface/GeoHUDInterface.h"
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
	DOREPLIFETIME(AGeoPlayerState, BestDPS);
	DOREPLIFETIME(AGeoPlayerState, BestHPS);
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
	if (!IsValid(NewPawn))
	{
		return;
	}

	// PlayerClass and the pawn link replicate independently. If PlayerClass arrived before the pawn,
	// OnRep_PlayerClass already ran and bailed on a null pawn — apply class data now that the pawn exists.
	// Must run for remote players' pawns too: on a dedicated server clients only see other players' class
	// visuals through this path + OnRep_PlayerClass.
	ApplyClassDataToPawn();

	// Local-player gate (not !IsServer): the listen-server host needs these HUD steps too, and remote players'
	// PlayerStates living on the server must not run them. GetOwningController() returns null / a non-local controller
	// in both excluded cases.
	AGeoPlayerController* GeoPlayerController = Cast<AGeoPlayerController>(GetOwningController());
	if (!GeoPlayerController || !GeoPlayerController->IsLocalPlayerController())
	{
		return;
	}

	InitOverlay();

	// Bind pawn-dependent HUD callbacks now that the pawn (and its components) exist.
	if (IGeoHUDInterface* GeoHUD = Cast<IGeoHUDInterface>(GeoPlayerController->GetHUD()))
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
		if (IGeoHUDInterface* GeoHUD = Cast<IGeoHUDInterface>(GeoPlayerController->GetHUD()))
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
		if (IGeoHUDInterface* GeoHUD = Cast<IGeoHUDInterface>(GeoPlayerController->GetHUD()))
		{
			GeoHUD->BuildAbilityBar(GeoPlayerController->GetPawn<APlayableCharacter>());
		}
	}
}

void AGeoPlayerState::ApplyClassDataToPawn()
{
	// None is legitimate here: on the server OnPawnSet fires during PossessedBy, before ChangeClass picks the class.
	if (PlayerClass == EPlayerClass::None)
	{
		return;
	}
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
	return FGenericTeamId(static_cast<uint8>(TeamId));
}
