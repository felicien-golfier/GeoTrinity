#include "Characters/GeoCharacter.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GeoMovementComponent.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Input/GeoInputComponent.h"
#include "Tool/UGameplayLibrary.h"
#include "VisualLogger/VisualLogger.h"

// Sets default values
AGeoCharacter::AGeoCharacter(FObjectInitializer const& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<UGeoMovementComponent>(CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);

	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetIsReplicated(true);
	MeshComp->SetCastShadow(false);
	// Set default collision profiles
	MeshComp->SetCollisionProfileName(TEXT("GeoShape"));

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("GeoCapsule"));

	GeoInputComponent = CreateDefaultSubobject<UGeoInputComponent>(TEXT("Geo Input Component"));
	GeoInputComponent->SetIsReplicated(true);

	bUseControllerRotationYaw = true;

	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AGeoCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UE_VLOG_LOCATION(this, LogGeoTrinity, Verbose, GetActorLocation(), 30.f, UGameplayLibrary::GetColorForObject(this),
					 TEXT("%s [%s]"), *GetName(), *UEnum::GetValueAsString(GetLocalRole()));
}

UAbilitySystemComponent* AGeoCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

ETeamAttitude::Type AGeoCharacter::GetTeamAttitudeTowards(AActor const& Other) const
{
	IGenericTeamAgentInterface const* OtherTeamAgent = Cast<IGenericTeamAgentInterface const>(&Other);
	if (!OtherTeamAgent)
	{
		return ETeamAttitude::Neutral;
	}

	return OtherTeamAgent->GetGenericTeamId() == GetGenericTeamId() ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
}

void AGeoCharacter::DrawDebugVectorFromCharacter(FVector const& Direction, FString const& DebugMessage) const
{
	DrawDebugVectorFromCharacter(Direction, DebugMessage, UGameplayLibrary::GetColorForObject(this));
}

void AGeoCharacter::DrawDebugVectorFromCharacter(FVector const& Direction, FString const& DebugMessage,
												 FColor Color) const
{
	// Debug: draw a world-space line (arrow) from the character showing the look vector
	if (UWorld* World = GetWorld())
	{
		FVector const Start = GetActorLocation();
		FVector const Dir = Direction.GetSafeNormal();
		constexpr float Length = 500.f; // visualized length of the vector
		FVector const End = Start + Dir * Length;

		DrawDebugDirectionalArrow(World, Start, End, 20.f, Color, false, 0.f, 0, 2.f);

		UE_VLOG_ARROW(this, LogGeoTrinity, VeryVerbose, Start, End, Color, TEXT("%s"), *DebugMessage);
	}
}

void AGeoCharacter::InitGAS()
{
	AbilitySystemComponent->InitializeDefaultAttributes();
	if (HasAuthority())
	{
		AbilitySystemComponent->GiveStartupAbilities();
	}
}

#ifdef UE_EDITOR
void AGeoCharacter::BeginPlay()
{
	Super::BeginPlay();
	LocalRoleForDebugPurpose = GetLocalRole();
}
#endif
