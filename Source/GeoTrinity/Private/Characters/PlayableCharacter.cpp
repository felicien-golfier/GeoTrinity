#include "Characters/PlayableCharacter.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/Component/GeoDeploySatelliteComponent.h"
#include "Characters/Component/ShieldBurstPassiveComponent.h"
#include "Components/WidgetComponent.h"
#include "GameClasses/GeoGameState.h"
#include "GameClasses/GeoPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/Interface/GeoChargeBeamGaugeWidgetInterface.h"
#include "HUD/Interface/GeoChargeGaugeWidgetInterface.h"
#include "Input/GeoInputComponent.h"
#include "Net/UnrealNetwork.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "VectorTypes.h"
#include "World/GeoWorldSettings.h"

namespace
{
	/** How long a gauge lingers after its charge ends, so the final fill is seen before the widget disappears. */
	constexpr float GaugeHideDelay = 0.15f;
}

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

	AimCursorComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("AimCursorComponent"));
	AimCursorComponent->SetupAttachment(GetRootComponent());
	AimCursorComponent->SetWidgetSpace(EWidgetSpace::Screen);
	AimCursorComponent->SetDrawAtDesiredSize(true);
	AimCursorComponent->SetHiddenInGame(true);
	if (UClass* const AimCursorWidgetClass = GetDefault<UGameDataSettings>()->AimCursorWidgetClass.LoadSynchronous())
	{
		AimCursorComponent->SetWidgetClass(AimCursorWidgetClass);
	}

	DeploySatelliteComponent = CreateDefaultSubobject<UGeoDeploySatelliteComponent>(TEXT("DeploySatelliteComponent"));
	DeploySatelliteComponent->SetupAttachment(WidgetAnchorComponent);

	TeamId = ETeam::Player;
}

void APlayableCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Not the constructor: a Blueprint-authored AimCursorDistance is only applied to the CDO after it has run.
	AimCursorComponent->SetRelativeLocation(FVector(AimCursorDistance, 0.f, 0.f));
}

void APlayableCharacter::SetChargeGaugeVisible(UWidgetComponent* Component, FTimerHandle& HideHandle,
											   UGeoGameplayAbility* Ability, bool const bVisible)
{
	IGeoChargeGaugeWidgetInterface* Widget =
		Cast<IGeoChargeGaugeWidgetInterface>(Component->GetUserWidgetObject());
	if (!ensureMsgf(Widget, TEXT("%s has no widget or wrong widget class on %s"), *Component->GetName(), *GetName()))
	{
		return;
	}

	if (bVisible)
	{
		GetWorld()->GetTimerManager().ClearTimer(HideHandle);
		Component->SetHiddenInGame(false);
		Widget->SetChargeAbility(Ability);
		return;
	}

	// Attach before the final sync: a charge too short to ever reach the visible branch still has to render its end
	// state once before the gauge detaches.
	Widget->SetChargeAbility(Ability);
	Widget->UpdateVisualChargeRatio();
	Widget->SetChargeAbility(nullptr);

	GetWorld()->GetTimerManager().SetTimer(HideHandle,
										   FTimerDelegate::CreateWeakLambda(this,
																			[Component]()
																			{
																				Component->SetHiddenInGame(true);
																			}),
										   GaugeHideDelay, false);
}

void APlayableCharacter::SetDeployChargeGaugeVisibility(UGeoGameplayAbility* Ability, bool const bVisible)
{
	SetChargeGaugeVisible(DeployChargeGaugeComponent, ChargeDeployHideTimerHandle, Ability, bVisible);
}

void APlayableCharacter::SetChargeBeamGaugeVisible(UGeoGameplayAbility* Ability, bool bVisible, float SweetSpotMinRatio,
												   float SweetSpotMaxRatio)
{
	SetChargeGaugeVisible(ChargeBeamGaugeComponent, ChargeBeamHideTimerHandle, Ability, bVisible);

	if (!bVisible)
	{
		return;
	}
	if (IGeoChargeBeamGaugeWidgetInterface* Widget =
			Cast<IGeoChargeBeamGaugeWidgetInterface>(ChargeBeamGaugeComponent->GetUserWidgetObject()))
	{
		Widget->SetSweetSpotRatios(SweetSpotMinRatio, SweetSpotMaxRatio);
	}
}

void APlayableCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled())
	{
		UpdateAimRotation(DeltaSeconds);
		AimCursorComponent->SetHiddenInGame(!GeoInputComponent->IsUsingController() || IsDead());
	}
}

void APlayableCharacter::UpdateAimRotation(float /*DeltaSeconds*/)
{
	FVector2D Look;
	if (!GeoInputComponent->GetLookVector(Look))
	{
		return;
	}

	float const DesiredYaw = FMath::Atan2(Look.Y, Look.X) * (180.f / PI);
	SetTargetYaw(DesiredYaw);
}

void APlayableCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	GeoInputComponent->BindInput(PlayerInputComponent);

	UAbilityInfo* AbilityInfo = UGeoAbilitySystemLibrary::GetAbilityInfo();
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

	BindCombattantWidgetToASC();
}

void APlayableCharacter::ResetAbilitiesAndEffects()
{
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}
	AbilitySystemComponent->CancelAllAbilities();
	AbilitySystemComponent->ResetCooldowns();

	// Effect removal is authoritative and replicates down; a client running it would drop replicated effects out of the
	// fast array without their granted tags.
	if (GeoLib::IsServer(this))
	{
		AbilitySystemComponent->RemoveActiveEffects(FGameplayEffectQuery());
	}
}

void APlayableCharacter::DeathLogic()
{
	ResetAbilitiesAndEffects();
	if (IsValid(AbilitySystemComponent) && GeoLib::IsServer(this))
	{
		// A death that never went through the health path (an arena fall) still leaves health and shield behind.
		AbilitySystemComponent->SetNumericAttributeBase(UGeoAttributeSetBase::GetHealthAttribute(), 0.f);
		AbilitySystemComponent->SetNumericAttributeBase(UGeoAttributeSetBase::GetShieldAttribute(), 0.f);
		// Death disarms the sacrifice detonation (the DetonateReady GE just went away with the purge above).
		AbilitySystemComponent->SetNumericAttributeBase(UCharacterAttributeSet::GetSacrificeValueAttribute(), 0.f);
	}
	StopCharacter();
	SetCanBeDamaged(false);

	if (GeoLib::IsServer(this))
	{
		AGeoGameState* GameState = GetWorld()->GetGameState<AGeoGameState>();
		if (!ensureMsgf(GameState, TEXT("No GameState in %s"), *GetName()))
		{
			return;
		}
		GameState->NotifyPlayerDied(*this);
	}
}

void APlayableCharacter::ReviveLogic()
{
	ResetAbilitiesAndEffects();
	StopAllSpawnedElements();
	if (IsValid(AbilitySystemComponent))
	{
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
	SetDeathVisuals(true);
}

void APlayableCharacter::RestartCharacter()
{
	EnableInput(GetGeoPlayerController());
	GetGeoMovementComponent()->GravityScale = 1.f;
	GetGeoMovementComponent()->SetMovementMode(MOVE_Walking);
	SetActorEnableCollision(true);
	SetDeathVisuals(false);
}

void APlayableCharacter::SetDeathVisuals(bool const bDead)
{
	Super::SetDeathVisuals(bDead);

	FPlayerClassData const* VisualData = GetClassData(GetPlayerClass());
	if (!VisualData)
	{
		return;
	}
	SetBodyMaterial(bDead ? VisualData->DeathMaterial : VisualData->AliveMaterial);
}

UAnimMontage* APlayableCharacter::GetDeathMontage() const
{
	FPlayerClassData const* VisualData = GetClassData(GetPlayerClass());
	if (!VisualData)
	{
		return nullptr;
	}
	return bDiedFromFall ? VisualData->FallMontage : VisualData->DeathMontage;
}

FPlayerClassData const* APlayableCharacter::GetClassData(EPlayerClass Class) const
{
	FPlayerClassData const* PlayerClassData = ClassData.Find(Class);
	ensureMsgf(PlayerClassData, TEXT("%hs: no ClassData entry for %s on %s"), __FUNCTION__,
			   *UEnum::GetValueAsString(Class), *GetName());
	return PlayerClassData;
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
	if (NewValue <= 0.f)
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
	ResetAbilitiesAndEffects();
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

	FPlayerClassData const* PlayerClassData = GetClassData(GetPlayerClass());
	if (!PlayerClassData
		|| !ensureMsgf(PlayerClassData->DefaultAttributes, TEXT("GiveLife: No DefaultAttributes for class on %s"),
					   *GetName()))
	{
		return;
	}

	AbilitySystemComponent->ApplyEffectToSelf(PlayerClassData->DefaultAttributes);
	AbilitySystemComponent->ReactivatePassiveAbilities();
}

void APlayableCharacter::ApplyClassData(EPlayerClass NewClass)
{
	FPlayerClassData const* PlayerClassData = GetClassData(NewClass);
	if (!PlayerClassData)
	{
		return;
	}

	GetMesh()->SetSkeletalMesh(PlayerClassData->Mesh);
	GetMesh()->SetAnimInstanceClass(PlayerClassData->AnimClass);
	ensureMsgf(PlayerClassData->AliveMaterial, TEXT("ApplyClassData: No AliveMaterial for class on %s"), *GetName());
	SetBodyMaterial(PlayerClassData->AliveMaterial);
	DeploySatelliteComponent->SetParams(PlayerClassData->SatelliteParams);
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
