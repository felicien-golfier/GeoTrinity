// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/GeoTeleporter.h"

#include "Characters/PlayableCharacter.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "World/GeoGameCamera.h"

AGeoTeleporter::AGeoTeleporter()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	MeshComponent->CastShadow = false;
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Overlap);

	TextComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextComponent"));
	TextComponent->SetupAttachment(MeshComponent);
	TextComponent->SetRelativeRotation(FRotator(0, 90.0, 0.0));
	TextComponent->SetHorizontalAlignment(EHTA_Center);
	TextComponent->CastShadow = false;
}

void AGeoTeleporter::OnConstruction(FTransform const& Transform)
{
	Super::OnConstruction(Transform);
	TextComponent->SetText(DisplayText);
}

void AGeoTeleporter::BeginPlay()
{
	Super::BeginPlay();
	MeshComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnBeginOverlap);
	MeshComponent->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnEndOverlap);

	if (AGeoTeleporter const* NextTeleporter = FindNextTeleporter())
	{
		FVector const Direction = (NextTeleporter->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (!Direction.IsNearlyZero())
		{
			SetActorRotation(Direction.Rotation());
		}
	}
}

void AGeoTeleporter::OnBeginOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
									UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/,
									FHitResult const& /*SweepResult*/)
{

	APlayableCharacter* PlayableCharacter = Cast<APlayableCharacter>(OtherActor);
	if (!PlayableCharacter
		|| !ensureMsgf(TeleportTag.IsValid(), TEXT("AGeoTeleporter %s has no TeleportTag."), *GetName()))
	{
		return;
	}

	if (PendingArrivals.Contains(PlayableCharacter))
	{
		return;
	}

	if (PlayableCharacter->IsLocallyControlled() && CameraTag.IsValid())
	{
		AGeoGameCamera* Camera =
			Cast<AGeoGameCamera>(UGameplayStatics::GetActorOfClass(GetWorld(), AGeoGameCamera::StaticClass()));
		if (ensureMsgf(Camera, TEXT("AGeoTeleporter %s: no AGeoGameCamera in the level."), *GetName()))
		{
			Camera->SetBoundsTag(CameraTag);
		}
	}

	if (!GeoLib::IsServer(GetWorld()))
	{
		return;
	}

	AGeoTeleporter* NextTeleporter = FindNextTeleporter();
	if (!NextTeleporter)
	{
		UE_LOG(LogTemp, Warning,
			   TEXT("AGeoTeleporter %s: no other teleporter with tag %s in the level — nowhere to send %s."),
			   *GetName(), *TeleportTag.ToString(), *PlayableCharacter->GetName());
		return;
	}

	NextTeleporter->PendingArrivals.Add(PlayableCharacter);
	FVector const Destination = NextTeleporter->GetActorLocation();
	PlayableCharacter->SetActorLocation(FVector(Destination.X, Destination.Y, PlayableCharacter->GetActorLocation().Z),
										false, nullptr, ETeleportType::TeleportPhysics);
}

void AGeoTeleporter::OnEndOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
								  UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	PendingArrivals.Remove(OtherActor);
}

AGeoTeleporter* AGeoTeleporter::FindNextTeleporter() const
{
	TArray<AActor*> AllTeleporters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), StaticClass(), AllTeleporters);
	TArray<AActor*> Teleporters = AllTeleporters.FilterByPredicate(
		[this](AActor const* Actor)
		{
			return CastChecked<AGeoTeleporter>(Actor)->TeleportTag == TeleportTag;
		});
	if (Teleporters.Num() < 2)
	{
		return nullptr;
	}

	Teleporters.Sort(
		[](AActor const& Left, AActor const& Right)
		{
			return Left.GetName() < Right.GetName();
		});
	int32 const MyIndex = Teleporters.IndexOfByKey(this);
	return CastChecked<AGeoTeleporter>(Teleporters[(MyIndex + 1) % Teleporters.Num()]);
}
