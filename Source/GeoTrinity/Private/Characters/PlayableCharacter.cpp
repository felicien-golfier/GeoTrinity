#include "Characters/PlayableCharacter.h"

#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "Components/WidgetComponent.h"
#include "GeoPlayerState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/GeoDeployChargeGaugeWidget.h"
#include "Input/GeoInputComponent.h"

APlayableCharacter::APlayableCharacter(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	DeployChargeGaugeComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("DeployChargeGaugeComponent"));
	DeployChargeGaugeComponent->SetupAttachment(GetRootComponent());
	DeployChargeGaugeComponent->SetWidgetSpace(EWidgetSpace::Screen);
	DeployChargeGaugeComponent->SetHiddenInGame(true);
}

void APlayableCharacter::ShowDeployChargeGauge(UGeoDeployAbility* Ability)
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

void APlayableCharacter::HideDeployChargeGauge()
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

void APlayableCharacter::UpdateAimRotation(float DeltaSeconds)
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

	GeoInputComponent->BindAbilityActions(this, &ThisClass::AbilityInputTagPressed, &ThisClass::AbilityInputTagReleased,
										  &ThisClass::AbilityInputTagHeld);
}

void APlayableCharacter::InitGAS()
{
	AGeoPlayerState* GeoPlayerState = GetPlayerState<AGeoPlayerState>();
	if (!GeoPlayerState)
	{
		UE_LOG(LogTemp, Warning, TEXT("No player state in %s"), *GetName());
		return;
	}

	AbilitySystemComponent = Cast<UGeoAbilitySystemComponent>(GeoPlayerState->GetAbilitySystemComponent());
	AbilitySystemComponent->InitAbilityActorInfo(GeoPlayerState, this);
	AttributeSetBase = GeoPlayerState->GetCharacterAttributeSet();

	Super::InitGAS();
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

void APlayableCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Set the ASC on the Server. Clients do this in OnRep_PlayerState()
	InitGAS();
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
