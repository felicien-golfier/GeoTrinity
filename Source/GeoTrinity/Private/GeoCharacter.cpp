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

	// Set default collision profiles
	MeshComponent->SetCollisionProfileName(TEXT("GeoShape"));
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("GeoCapsule"));

	GeoInputComponent = CreateDefaultSubobject<UGeoInputComponent>(TEXT("Geo Input Component"));
	GeoInputComponent->SetIsReplicated(true);

	// Disable orient-to-movement; we will rotate manually toward aim
	bUseControllerRotationYaw = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AGeoCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateAimRotation(DeltaSeconds);
}

void AGeoCharacter::UpdateAimRotation(float DeltaSeconds)
{
	FVector2D Look;
	if (GeoInputComponent->GetLookVector(Look))
	{
		float DesiredYaw = FMath::Atan2(Look.Y, Look.X) * (180.f / PI);

		// Apply rotation locally
		FRotator R = GetActorRotation();
		R.Yaw = DesiredYaw;
		SetActorRotation(R);

		if (!HasAuthority())
		{
			TimeSinceLastAimSend += DeltaSeconds;
			constexpr float MaxTimeBetweenAimUpdates = 0.03f;
			constexpr float MinYawToUpdateServer = 0.5f;
			if (TimeSinceLastAimSend > MaxTimeBetweenAimUpdates
				&& FMath::Abs(DesiredYaw - LastSentAimYaw) > MinYawToUpdateServer)
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
	AddCharacterDefaultAbilities();
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

void AGeoCharacter::AddCharacterDefaultAbilities()
{
	checkf(AbilitySystemComponent, TEXT("%s() AbilitySystemComponent is null. Did we call this too soon ?"),
		*FString(__FUNCTION__))

		if (!HasAuthority())
	{
		UE_LOG(LogGeoASC, Warning,
			TEXT("This should not be the case, as only the server should be calling this method"));
	}
	AbilitySystemComponent->AddCharacterStartupAbilities(StartupAbilities);
}

void AGeoCharacter::AbilityInputTagPressed(FGameplayTag inputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s pressed"), *inputTag.ToString());
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->AbilityInputTagPressed(inputTag);
}

void AGeoCharacter::AbilityInputTagReleased(FGameplayTag inputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s released"), *inputTag.ToString());
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->AbilityInputTagReleased(inputTag);
}

void AGeoCharacter::AbilityInputTagHeld(FGameplayTag inputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s heeeeeld"), *inputTag.ToString());
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->AbilityInputTagHeld(inputTag);
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
		FColor::Magenta, FColor::Orange, FColor::Emerald, FColor::Purple, FColor::Turquoise, FColor::Silver};

	return Palette[Character->GetUniqueID() % std::size(Palette)];
}

// void AGeoCharacter::VLogBoxes(const FInputStep& InputStep, const FColor Color) const
// {
// 	UE_VLOG_BOX(this, LogGeoTrinity, VeryVerbose,
// 		FBox(FVector(GetBox().Min, 0.f) + GetActorLocation(), FVector(GetBox().Max, 0.f) + GetActorLocation()), Color,
// 		TEXT("LocalTime %s, delta time %.5f"), *InputStep.Time.ToString(), InputStep.DeltaTimeSeconds);
// }

void AGeoCharacter::DrawDebugVectorFromCharacter(const FVector& Direction, const FString& DebugMessage) const
{
	DrawDebugVectorFromCharacter(Direction, DebugMessage, GetColorForCharacter(this));
}

void AGeoCharacter::DrawDebugVectorFromCharacter(const FVector& Direction, const FString& DebugMessage,
	FColor Color) const
{
	// Debug: draw a world-space line (arrow) from the character showing the look vector
	if (UWorld* World = GetWorld())
	{
		const FVector Start = GetActorLocation();
		const FVector Dir = Direction.GetSafeNormal();
		constexpr float Length = 500.f;   // visualized length of the vector
		const FVector End = Start + Dir * Length;

		// Single-frame arrow (non-persistent) so it updates every tick without clutter
		DrawDebugDirectionalArrow(World, Start, End, 20.f, Color,
			/*bPersistentLines*/ false,
			/*LifeTime*/ 0.f, /*DepthPriority*/ 0, /*Thickness*/ 2.f);

		UE_VLOG_ARROW(this, LogGeoTrinity, VeryVerbose, Start, End, Color, TEXT("%s"), *DebugMessage);
	}
}