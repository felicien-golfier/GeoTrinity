#include "GeoPawn.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/GeoAttributeSetBase.h"
#include "Components/DynamicMeshComponent.h"
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
	Box = FBox2D(FVector2D(-25.f, -25.f), FVector2D(25.f, 25.f));   // carré 50x50

	FDynamicMesh3 MyFDynamicMesh3;
	const int Vid1 = MyFDynamicMesh3.AppendVertex(FVector(Box.Max, 0));
	const int Vid2 = MyFDynamicMesh3.AppendVertex(FVector(Box.Max.X, Box.Min.Y, 0));
	const int Vid3 = MyFDynamicMesh3.AppendVertex(FVector(Box.Min, 0.f));
	const int Vid4 = MyFDynamicMesh3.AppendVertex(FVector(Box.Min.X, Box.Max.Y, 0));

	MyFDynamicMesh3.AppendTriangle(Vid1, Vid2, Vid3);
	MyFDynamicMesh3.AppendTriangle(Vid3, Vid4, Vid1);
	MeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("Dynamic Mesh Component"));
	MeshComponent->SetMesh(MoveTemp(MyFDynamicMesh3));
	SetRootComponent(MeshComponent);

	GeoInputComponent = CreateDefaultSubobject<UGeoInputComponent>(TEXT("Geo Input Component"));
	GeoMovementComponent = CreateDefaultSubobject<UGeoMovementComponent>(TEXT("Geo Movement Component"));
}

void AGeoPawn::GetActorBounds(bool bOnlyCollidingComponents, FVector& Origin, FVector& BoxExtent,
	bool bIncludeFromChildActors) const
{
	Origin = GetActorLocation();
	BoxExtent = FVector(Box.GetExtent(), 0.f);
}

void AGeoPawn::BP_ApplyEffectToSelfDefaultLvl(TSubclassOf<UGameplayEffect> gameplayEffectClass)
{
	ApplyEffectToSelf(gameplayEffectClass, 1.0f);
}

void AGeoPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	GeoInputComponent->BindInput(PlayerInputComponent);
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
	check(IsValid(AbilitySystemComponent))

		if (!IsValid(DefaultAttributes))
	{
		UE_LOG(LogGeoTrinity, Error,
			TEXT("%s() Missing DefaultAttributes for %s. Please fill in the pawn's Blueprint."), *FString(__FUNCTION__),
			*GetName());
		return;
	}

	ApplyEffectToSelf(DefaultAttributes, 1.0f);
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
