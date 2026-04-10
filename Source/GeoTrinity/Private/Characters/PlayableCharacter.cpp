#include "Characters/PlayableCharacter.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GeoPlayerState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/GeoDeployChargeGaugeWidget.h"
#include "Input/GeoInputComponent.h"
#include "VectorTypes.h"
#include "World/GeoWorldSettings.h"

APlayableCharacter::APlayableCharacter(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	DeployChargeGaugeComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("DeployChargeGaugeComponent"));
	DeployChargeGaugeComponent->SetupAttachment(GetRootComponent());
	DeployChargeGaugeComponent->SetWidgetSpace(EWidgetSpace::Screen);
	DeployChargeGaugeComponent->SetHiddenInGame(true);

	DeployableManagerComponent =
		CreateDefaultSubobject<UGeoDeployableManagerComponent>(TEXT("DeployableManagerComponent"));

	TeamId = ETeam::Player;
}

void APlayableCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APlayableCharacter::ShowDeployChargeGauge(UGeoGameplayAbility* Ability) const
{
	UGeoDeployChargeGaugeWidget* Widget =
		Cast<UGeoDeployChargeGaugeWidget>(DeployChargeGaugeComponent->GetUserWidgetObject());
	ensureMsgf(Widget, TEXT("DeployChargeGaugeComponent has no widget or wrong widget class on %s"), *GetName());
	if (Widget)
	{
		Widget->DeployAbility = Ability;
	}
	DeployChargeGaugeComponent->SetHiddenInGame(false);
}

void APlayableCharacter::HideDeployChargeGauge() const
{
	DeployChargeGaugeComponent->SetHiddenInGame(true);
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
	if (HasAuthority())
	{
		AbilitySystemComponent->GiveStartupAbilities(GetPlayerClass());
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

	if (AGeoPlayerState* GeoPlayerState = GetPlayerState<AGeoPlayerState>())
	{
		GeoPlayerState->SetPlayerClass(PickStartingClass());
	}

	// Set the ASC on the Server. Clients do this in OnRep_PlayerState()
	InitGAS();
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
	DeployableManagerComponent->RecallAll();
	AbilitySystemComponent->ClearPlayerClassAbilities();
	AbilitySystemComponent->GiveStartupAbilities(NewClass);
	ApplyClassData(NewClass);
}

void APlayableCharacter::ApplyClassData(EPlayerClass NewClass)
{
	FPlayerClassData const* VisualData = ClassData.Find(NewClass);
	if (!VisualData)
	{
		ensureMsgf(VisualData, TEXT("ApplyClassData: No visual data for class on %s"), *GetName());
		return;
	}
	if (!VisualData->DefaultAttributes)
	{
		ensureMsgf(VisualData->DefaultAttributes, TEXT("ApplyClassData: No DefaultAttributes for class on %s"),
				   *GetName());
		return;
	}

	GetMesh()->SetSkeletalMesh(VisualData->Mesh);
	GetMesh()->SetAnimInstanceClass(VisualData->AnimClass);
	AbilitySystemComponent->ApplyEffectToSelf(VisualData->DefaultAttributes);
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
