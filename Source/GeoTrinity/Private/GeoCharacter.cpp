#include "GeoCharacter.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/GeoAttributeSetBase.h"
#include "Components/CapsuleComponent.h"
#include "GeoInputComponent.h"
#include "GeoMovementComponent.h"
#include "GeoPlayerController.h"
#include "GeoPlayerState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/GeoHUD.h"

// Sets default values
AGeoCharacter::AGeoCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(
		  ObjectInitializer.SetDefaultSubobjectClass<UGeoMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh Component"));
	MeshComponent->SetIsReplicated(true);
	MeshComponent->SetupAttachment(GetCapsuleComponent());
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GeoInputComponent = CreateDefaultSubobject<UGeoInputComponent>(TEXT("Geo Input Component"));
	GeoInputComponent->SetIsReplicated(true);

	// Use the Character's movement component, which we've overridden to our class above
	GeoMovementComponent = Cast<UGeoMovementComponent>(GetCharacterMovement());

	// Disable orient-to-movement; we will rotate manually toward aim
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement())
	{
		CharacterMovementComponent->bOrientRotationToMovement = false;
	}
}

void AGeoCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateAimRotation(DeltaSeconds);
}

void AGeoCharacter::UpdateAimRotation(float DeltaSeconds)
{
	// Compute desired aim yaw
	float DesiredYaw = GetActorRotation().Yaw;
	bool bHasAim = false;

	// Prior Stick over mouse orientation.
	if (IsValid(GeoInputComponent))
	{
		FVector2D Stick;
		if (GeoInputComponent->GetLookVector(Stick))
		{
			DesiredYaw = FMath::Atan2(Stick.Y, Stick.X) * (180.f / PI);
			bHasAim = true;
		}
	}

	if (!bHasAim)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			FVector WorldOrigin, WorldDir;
			if (PlayerController->DeprojectMousePositionToWorld(WorldOrigin, WorldDir))
			{
				// Intersect ray with plane Z = Actor.Z
				const float Z = GetActorLocation().Z;
				if (!FMath::IsNearlyZero(WorldDir.Z))
				{
					const float T = (Z - WorldOrigin.Z) / WorldDir.Z;
					if (T > 0.f)
					{
						const FVector Hit = WorldOrigin + WorldDir * T;
						const FVector ToHit = (Hit - GetActorLocation());
						const FRotator AimRot = ToHit.Rotation();
						DesiredYaw = AimRot.Yaw;
						bHasAim = true;
					}
				}
			}
		}
	}

	if (bHasAim)
	{
		// Apply rotation locally
		FRotator R = GetActorRotation();
		R.Yaw = DesiredYaw;
		SetActorRotation(R);

		if (!HasAuthority())
		{
			TimeSinceLastAimSend += DeltaSeconds;
			if (TimeSinceLastAimSend > 0.03f && FMath::Abs(DesiredYaw - LastSentAimYaw) > 0.5f)
			{
				if (AGeoPlayerController* GC = Cast<AGeoPlayerController>(GetController()))
				{
					GC->ServerSetAimYaw(DesiredYaw);
					LastSentAimYaw = DesiredYaw;
					TimeSinceLastAimSend = 0.f;
				}
			}
		}
	}
}

void AGeoCharacter::BP_ApplyEffectToSelfDefaultLvl(TSubclassOf<UGameplayEffect> gameplayEffectClass)
{
	ApplyEffectToSelf(gameplayEffectClass, 1.0f);
}

void AGeoCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	GeoInputComponent->BindInput(PlayerInputComponent);

	GeoInputComponent->BindAbilityActions(this, &ThisClass::AbilityInputTagPressed, &ThisClass::AbilityInputTagReleased,
		&ThisClass::AbilityInputTagHeld);
}

// Server Only
void AGeoCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitAbilityActorInfo();
	InitializeDefaultAttributes();
}

void AGeoCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Set the ASC for clients. Server does this in PossessedBy.
	InitAbilityActorInfo();
}

void AGeoCharacter::InitAbilityActorInfo()
{
	AGeoPlayerState* PS = GetPlayerState<AGeoPlayerState>();
	if (!PS)
	{
		return;
	}

	AbilitySystemComponent = Cast<UGeoAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	AbilitySystemComponent->InitAbilityActorInfo(PS, this);
	AttributeSet = PS->GetGeoAttributeSetBase();

	if (AGeoPlayerController* GeoPlayerController = Cast<AGeoPlayerController>(GetController()))
	{
		// Hud only present locally
		if (AGeoHUD* Hud = Cast<AGeoHUD>(GeoPlayerController->GetHUD()))
		{
			Hud->InitOverlay(GeoPlayerController, PS, AbilitySystemComponent, AttributeSet);
		}
	}
}

void AGeoCharacter::InitializeDefaultAttributes()
{
	check(IsValid(AbilitySystemComponent));

	if (!IsValid(DefaultAttributes))
	{
		UE_LOG(LogGeoTrinity, Error,
			TEXT("%s() Missing DefaultAttributes for %s. Please fill in the pawn's Blueprint."), *FString(__FUNCTION__),
			*GetName());
		return;
	}

	ApplyEffectToSelf(DefaultAttributes, 1.0f);
}

void AGeoCharacter::AbilityInputTagPressed(FGameplayTag inputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s pressed"), *inputTag.ToString());
}

void AGeoCharacter::AbilityInputTagReleased(FGameplayTag inputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s released"), *inputTag.ToString());
}

void AGeoCharacter::AbilityInputTagHeld(FGameplayTag inputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s heeeeeld"), *inputTag.ToString());
}

void AGeoCharacter::ApplyEffectToSelf_Implementation(TSubclassOf<UGameplayEffect> gameplayEffectClass, float level)
{
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	FGameplayEffectContextHandle EffectContextHandle = AbilitySystemComponent->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(gameplayEffectClass, level, EffectContextHandle);

	if (SpecHandle.IsValid())
	{
		FPredictionKey PredictionKey;
		AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), AbilitySystemComponent,
			PredictionKey);
		// AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), AbilitySystemComponent);
	}
}

FColor AGeoCharacter::GetColorForCharacter(const AGeoCharacter* Character)
{
	if (!IsValid(Character))
	{
		return FColor::White;
	}

	static const FColor Palette[] = {FColor::Red, FColor::Green, FColor::Blue, FColor::Yellow, FColor::Cyan,
		FColor::Magenta, FColor(255, 165, 0),   // Orange
		FColor(128, 0, 128),   // Purple
		FColor::Turquoise, FColor::Silver};

	return Palette[Character->GetUniqueID() % std::size(Palette)];
}

// void AGeoCharacter::VLogBoxes(const FInputStep& InputStep, const FColor Color) const
// {
// 	UE_VLOG_BOX(this, LogGeoTrinity, VeryVerbose,
// 		FBox(FVector(GetBox().Min, 0.f) + GetActorLocation(), FVector(GetBox().Max, 0.f) + GetActorLocation()), Color,
// 		TEXT("LocalTime %s, delta time %.5f"), *InputStep.Time.ToString(), InputStep.DeltaTimeSeconds);
// }