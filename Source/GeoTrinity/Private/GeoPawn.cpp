#include "GeoPawn.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/GeoAttributeSetBase.h"
#include "GeoInputComponent.h"
#include "GeoMovementComponent.h"
#include "GeoPlayerController.h"
#include "GeoPlayerState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/GeoHUD.h"

// Sets default values
AGeoPawn::AGeoPawn()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need
	// it.
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);
	SetReplicateMovement(true);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh Component"));
	MeshComponent->SetIsReplicated(true);

	SetRootComponent(MeshComponent);

	GeoInputComponent = CreateDefaultSubobject<UGeoInputComponent>(TEXT("Geo Input Component"));
	GeoInputComponent->SetIsReplicated(true);

	GeoMovementComponent = CreateDefaultSubobject<UGeoMovementComponent>(TEXT("Geo Movement Component"));
}

void AGeoPawn::BP_ApplyEffectToSelfDefaultLvl(TSubclassOf<UGameplayEffect> gameplayEffectClass)
{
	ApplyEffectToSelf(gameplayEffectClass, 1.0f);
}

void AGeoPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	GeoInputComponent->BindInput(PlayerInputComponent);

	GeoInputComponent->BindAbilityActions(this, &ThisClass::AbilityInputTagPressed, &ThisClass::AbilityInputTagReleased,
		&ThisClass::AbilityInputTagHeld);
}

// Server Only
void AGeoPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitAbilityActorInfo();
	InitializeDefaultAttributes();
}

void AGeoPawn::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Set the ASC for clients. Server does this in PossessedBy.
	InitAbilityActorInfo();
}

void AGeoPawn::InitAbilityActorInfo()
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

void AGeoPawn::InitializeDefaultAttributes()
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

void AGeoPawn::AbilityInputTagPressed(FGameplayTag inputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s pressed"), *inputTag.ToString());
}

void AGeoPawn::AbilityInputTagReleased(FGameplayTag inputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s released"), *inputTag.ToString());
}

void AGeoPawn::AbilityInputTagHeld(FGameplayTag inputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s heeeeeld"), *inputTag.ToString());
}

void AGeoPawn::ApplyEffectToSelf_Implementation(TSubclassOf<UGameplayEffect> gameplayEffectClass, float level)
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

FColor AGeoPawn::GetColorForPawn(const AGeoPawn* Pawn)
{
	if (!IsValid(Pawn))
	{
		return FColor::White;
	}

	static const FColor Palette[] = {FColor::Red, FColor::Green, FColor::Blue, FColor::Yellow, FColor::Cyan,
		FColor::Magenta, FColor(255, 165, 0),   // Orange
		FColor(128, 0, 128),   // Purple
		FColor::Turquoise, FColor::Silver};

	return Palette[Pawn->GetUniqueID() % std::size(Palette)];
}

// void AGeoPawn::VLogBoxes(const FInputStep& InputStep, const FColor Color) const
// {
// 	UE_VLOG_BOX(this, LogGeoTrinity, VeryVerbose,
// 		FBox(FVector(GetBox().Min, 0.f) + GetActorLocation(), FVector(GetBox().Max, 0.f) + GetActorLocation()), Color,
// 		TEXT("LocalTime %s, delta time %.5f"), *InputStep.Time.ToString(), InputStep.DeltaTimeSeconds);
// }