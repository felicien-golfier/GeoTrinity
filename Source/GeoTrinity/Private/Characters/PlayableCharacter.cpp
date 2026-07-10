#include "Characters/PlayableCharacter.h"

#include "Characters/Component/ShieldBurstPassiveComponent.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/WidgetComponent.h"
#include "GameClasses/GeoGameState.h"
#include "GameClasses/GeoPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/Interface/GeoChargeBeamGaugeWidgetInterface.h"
#include "HUD/Interface/GeoCombattantWidgetHost.h"
#include "HUD/Interface/GeoDeployGaugeWidgetInterface.h"
#include "Input/GeoInputComponent.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "VectorTypes.h"
#include "World/GeoWorldSettings.h"

static TAutoConsoleVariable<bool> CVarPlayerInvincible(TEXT("Geo.PlayerInvincible"), false,
													   TEXT("When true, players never die from health reaching zero."),
													   ECVF_Cheat);

APlayableCharacter::APlayableCharacter(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	DeployChargeGaugeComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("DeployChargeGaugeComponent"));
	DeployChargeGaugeComponent->SetupAttachment(WidgetAnchorComponent);
	DeployChargeGaugeComponent->SetWidgetSpace(EWidgetSpace::Screen);
	DeployChargeGaugeComponent->SetRelativeLocation(FVector(0.f, 100.f, 0.f));
	DeployChargeGaugeComponent->SetHiddenInGame(true);

	ChargeBeamGaugeComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("ChargeBeamGaugeComponent"));
	ChargeBeamGaugeComponent->SetupAttachment(WidgetAnchorComponent);
	ChargeBeamGaugeComponent->SetWidgetSpace(EWidgetSpace::Screen);
	ChargeBeamGaugeComponent->SetRelativeLocation(FVector(0.f, -100.f, 0.f));
	ChargeBeamGaugeComponent->SetHiddenInGame(true);

	TeamId = ETeam::Player;
}

void APlayableCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APlayableCharacter::SetDeployChargeGaugeVisibility(UGeoGameplayAbility* Ability, bool const bVisible)
{
	DeployChargeGaugeComponent->SetHiddenInGame(false);
	GetWorld()->GetTimerManager().ClearTimer(ChargeDeployHideTimerHandle);

	IGeoDeployGaugeWidgetInterface* Widget =
		Cast<IGeoDeployGaugeWidgetInterface>(DeployChargeGaugeComponent->GetUserWidgetObject());
	ensureMsgf(Widget, TEXT("DeployChargeGaugeComponent has no widget or wrong widget class on %s"), *GetName());
	if (Widget)
	{
		Widget->SetDeployAbility(Ability);
	}

	if (bVisible)
	{
		DeployChargeGaugeComponent->SetHiddenInGame(false);
	}
	else
	{
		// Weak-bound: the timer is dropped if the character is destroyed first.
		GetWorld()->GetTimerManager().SetTimer(
			ChargeDeployHideTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]() { DeployChargeGaugeComponent->SetHiddenInGame(true); }),
			0.15f, false);
	}
}

void APlayableCharacter::SetChargeBeamGaugeVisible(UGeoGameplayAbility* Ability, bool bVisible, float SweetSpotMinRatio,
												   float SweetSpotMaxRatio)
{
	IGeoChargeBeamGaugeWidgetInterface* Widget =
		Cast<IGeoChargeBeamGaugeWidgetInterface>(ChargeBeamGaugeComponent->GetUserWidgetObject());

	if (bVisible)
	{
		ChargeBeamGaugeComponent->SetHiddenInGame(false);
		GetWorld()->GetTimerManager().ClearTimer(ChargeBeamHideTimerHandle);
		ensureMsgf(Widget, TEXT("ChargeBeamGaugeComponent has no widget or wrong widget class on %s"), *GetName());
		if (Widget)
		{
			Widget->SetChargeBeamAbility(Ability);
			Widget->SetSweetSpotRatios(SweetSpotMinRatio, SweetSpotMaxRatio);
		}
	}
	else
	{
		if (Widget)
		{
			Widget->SetChargeBeamAbility(Ability); // In case we haven't got time to enter visibility.
			Widget->UpdateVisualChargeRatio();
			Widget->SetChargeBeamAbility(nullptr);
		}
		// Weak-bound: the timer is dropped if the character is destroyed first.
		GetWorld()->GetTimerManager().SetTimer(
			ChargeBeamHideTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]() { ChargeBeamGaugeComponent->SetHiddenInGame(true); }),
			0.15f, false);
	}
}

void APlayableCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled())
	{
		UpdateAimRotation(DeltaSeconds);
	}
}

void APlayableCharacter::UpdateAimRotation(float DeltaSeconds) const
{
	FVector2D Look;
	if (!GeoInputComponent->GetLookVector(Look))
	{
		return;
	}

	float const DesiredYaw = FMath::Atan2(Look.Y, Look.X) * (180.f / PI);
	float const CurrentYaw = Controller->GetControlRotation().Yaw;
	float const DeltaAngle = FMath::FindDeltaAngleDegrees(CurrentYaw, DesiredYaw);
	float const MaxDelta = MaxRotationSpeed * DeltaSeconds;
	float const ClampedDelta = FMath::Clamp(DeltaAngle, -MaxDelta, MaxDelta);

	Controller->SetControlRotation(FRotator(0.f, CurrentYaw + ClampedDelta, 0.f));
}

void APlayableCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	GeoInputComponent->BindInput(PlayerInputComponent);

	UAbilityInfo* AbilityInfo = UGeoAbilitySystemLibrary::GetAbilityInfo(this);
	GeoInputComponent->BindAbilityActions(this, &ThisClass::AbilityInputTagPressed, &ThisClass::AbilityInputTagReleased,
										  &ThisClass::AbilityInputTagHeld, AbilityInfo);
}

void APlayableCharacter::InitGAS()
{
	AGeoPlayerState* GeoPlayerState = GetPlayerState<AGeoPlayerState>();
	ensureMsgf(GeoPlayerState, TEXT("No player state in %s"), *GetName());
	if (!GeoPlayerState)
	{
		return;
	}

	AbilitySystemComponent = Cast<UGeoAbilitySystemComponent>(GeoPlayerState->GetAbilitySystemComponent());
	AbilitySystemComponent->InitAbilityActorInfo(GeoPlayerState, this);
	AttributeSetBase = GeoPlayerState->GetCharacterAttributeSet();

	AbilitySystemComponent->InitializeDefaultAttributes();
	if (GeoLib::IsServer(this))
	{
		AbilitySystemComponent->GiveStartupAbilities(GetPlayerClass());
		AbilitySystemComponent->OnHealthChanged.AddDynamic(this, &APlayableCharacter::OnHealthChanged);
	}

	// On a remote proxy the ASC arrives via OnRep_PlayerState, after the bar's first bind; re-bind now that it exists.
	if (IGeoCombattantWidgetHost* WidgetHost = Cast<IGeoCombattantWidgetHost>(CharacterWidgetComponent))
	{
		WidgetHost->BindToOwnerASC();
	}
}

void APlayableCharacter::DeathLogic()
{
	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->CancelAllAbilities();
		// RemoveActiveEffects only runs on the authoritative ASC; on clients the removal replicates down.
		// Calling it client-side is a no-op that leaves locally-predicted effects (e.g. dash cooldown) lingering.
		if (GeoLib::IsServer(this))
		{
			AbilitySystemComponent->RemoveActiveEffects(FGameplayEffectQuery());
			// Death disarms the sacrifice detonation (the DetonateReady GE just went away with the purge above).
			AbilitySystemComponent->SetNumericAttributeBase(UCharacterAttributeSet::GetSacrificeValueAttribute(), 0.f);
		}
	}
	StopAllSpawnedElements();
	StopCharacter();
	SetCanBeDamaged(false);

	if (GeoLib::IsServer(this))
	{
		AGeoGameState* GameState = GetWorld()->GetGameState<AGeoGameState>();
		if (!ensureMsgf(GameState, TEXT("No GameState in %s"), *GetName()))
		{
			return;
		}
		GameState->NotifyPlayerDied(this);
	}
}

void APlayableCharacter::ReviveLogic()
{
	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->CancelAllAbilities();
		// RemoveActiveEffects only runs on the authoritative ASC; on clients the removal replicates down.
		if (GeoLib::IsServer(this))
		{
			AbilitySystemComponent->RemoveActiveEffects(FGameplayEffectQuery());
		}
		ApplyClassData(GetPlayerClass());
	}
	GiveLife();
	RestartCharacter();
	SetCanBeDamaged(true);
}

void APlayableCharacter::StopCharacter()
{
	DisableInput(GetGeoPlayerController());
	GetGeoMovementComponent()->StopMovementImmediately();
	GetGeoMovementComponent()->DisableMovement();
	// Collision must go off on clients too (other characters' movement is predicted), but with collision
	// off there is no floor to rest on, so disable gravity to keep the corpse where it died.
	GetGeoMovementComponent()->GravityScale = 0.f;
	SetActorEnableCollision(false);
	SetDeathMaterial(true);
}

void APlayableCharacter::RestartCharacter()
{
	EnableInput(GetGeoPlayerController());
	GetGeoMovementComponent()->GravityScale = 1.f;
	GetGeoMovementComponent()->SetMovementMode(MOVE_Walking);
	SetActorEnableCollision(true);
	SetDeathMaterial(false);
}

void APlayableCharacter::SetDeathMaterial(bool const bDead)
{
	FPlayerClassData const* VisualData = ClassData.Find(GetPlayerClass());
	if (!ensureMsgf(VisualData, TEXT("SetDeathMaterial: No visual data for class on %s"), *GetName()))
	{
		return;
	}
	SetBodyMaterial(bDead ? VisualData->DeathMaterial : VisualData->AliveMaterial);
}

void APlayableCharacter::SetBodyMaterial(UMaterialInterface* Material)
{
	GetMesh()->SetMaterial(0, Material);

	// Setting a raw material on slot 0 orphans the shield-burst gauge MID; recreate it so the gauge visual
	// survives death/revive/class swaps regardless of the order the material swap and the replicated
	// ShieldBurstPassiveComponent arrive in.
	UShieldBurstPassiveComponent* ShieldBurstComponent = FindComponentByClass<UShieldBurstPassiveComponent>();
	if (ShieldBurstComponent && !GeoLib::IsDedicatedServer(GetWorld()))
	{
		ShieldBurstComponent->InitializeMaterialInstances();
	}
}

void APlayableCharacter::OnHealthChanged(float const NewValue)
{
	if (NewValue <= 0.f && !CVarPlayerInvincible.GetValueOnGameThread())
	{
		Death();
	}
}

void APlayableCharacter::AbilityInputTagPressed(FGameplayTag InputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s pressed"), *InputTag.ToString());
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->AbilityInputTagPressed(InputTag);
}

void APlayableCharacter::AbilityInputTagReleased(FGameplayTag InputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s released"), *InputTag.ToString());
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->AbilityInputTagReleased(InputTag);
}

void APlayableCharacter::AbilityInputTagHeld(FGameplayTag InputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s heeeeeld"), *InputTag.ToString());
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->AbilityInputTagHeld(InputTag);
}

EPlayerClass APlayableCharacter::PickStartingClass() const
{
	if (AGeoWorldSettings const* GeoWorldSettings = Cast<AGeoWorldSettings>(GetWorld()->GetWorldSettings()))
	{
		if (GeoWorldSettings->StartingClassOverride != EPlayerClass::None
			&& GeoWorldSettings->StartingClassOverride != EPlayerClass::All)
		{
			return GeoWorldSettings->StartingClassOverride;
		}
	}

	AGameStateBase const* GameState = GetWorld()->GetGameState();
	if (!GameState)
	{
		ensureMsgf(GameState, TEXT("PickStartingClass: No GameState on %s"), *GetName());
		return EPlayerClass::Triangle;
	}

	TSet<EPlayerClass> UsedClasses;
	for (APlayerState* Player : GameState->PlayerArray)
	{
		AGeoPlayerState const* GeoPlayerState = Cast<AGeoPlayerState>(Player);
		if (GeoPlayerState && GeoPlayerState != GetPlayerState<AGeoPlayerState>())
		{
			UsedClasses.Add(GeoPlayerState->GetPlayerClass());
		}
	}

	for (uint8 i = static_cast<uint8>(EPlayerClass::None) + 1; i < static_cast<uint8>(EPlayerClass::All); i++)
	{
		EPlayerClass Class = static_cast<EPlayerClass>(i);
		if (!UsedClasses.Contains(Class))
		{
			return Class;
		}
	}

	return EPlayerClass::Triangle;
}

void APlayableCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Set the ASC on the Server. Clients do this in OnRep_PlayerState()
	InitGAS();
	ChangeClass(PickStartingClass());
}

void APlayableCharacter::ChangeClass(EPlayerClass NewClass)
{
	AGeoPlayerState* GeoPlayerState = GetPlayerState<AGeoPlayerState>();
	if (!GeoPlayerState)
	{
		ensureMsgf(GeoPlayerState, TEXT("ChangeClass: No player state on %s"), *GetName());
		return;
	}

	GeoPlayerState->SetPlayerClass(NewClass);
	StopAllSpawnedElements();
	AbilitySystemComponent->ClearPlayerClassAbilities();
	AbilitySystemComponent->GiveStartupAbilities(NewClass);
	ApplyClassData(NewClass);
	GiveLife();

	// Clients rebuild the bar from OnRep_PlayerClass, but the listen-server host has no OnRep on its own PlayerState.
	// Rebuild here now that abilities for NewClass are granted (no-op when this controller isn't local).
	GeoPlayerState->RebuildAbilityBar();
}

void APlayableCharacter::GiveLife()
{
	if (!GeoLib::IsServer(this) || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	FPlayerClassData const* PlayerClassData = ClassData.Find(GetPlayerClass());
	if (!ensureMsgf(PlayerClassData && PlayerClassData->DefaultAttributes,
					TEXT("GiveLife: No DefaultAttributes for class on %s"), *GetName()))
	{
		return;
	}

	AbilitySystemComponent->ApplyEffectToSelf(PlayerClassData->DefaultAttributes);
	AbilitySystemComponent->ReactivatePassiveAbilities();
}

void APlayableCharacter::ApplyClassData(EPlayerClass NewClass)
{
	FPlayerClassData const* PlayerClassData = ClassData.Find(NewClass);
	if (!PlayerClassData)
	{
		ensureMsgf(PlayerClassData, TEXT("ApplyClassData: No visual data for class on %s"), *GetName());
		return;
	}

	GetMesh()->SetSkeletalMesh(PlayerClassData->Mesh);
	GetMesh()->SetAnimInstanceClass(PlayerClassData->AnimClass);
	ensureMsgf(PlayerClassData->AliveMaterial, TEXT("ApplyClassData: No AliveMaterial for class on %s"), *GetName());
	SetBodyMaterial(PlayerClassData->AliveMaterial);
}

EPlayerClass APlayableCharacter::GetPlayerClass() const
{
	if (AGeoPlayerState const* GeoPlayerState = GetPlayerState<AGeoPlayerState>())
	{
		return GeoPlayerState->GetPlayerClass();
	}
	return EPlayerClass::None;
}

void APlayableCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitGAS();
}
