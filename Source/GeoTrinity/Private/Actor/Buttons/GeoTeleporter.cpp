// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Buttons/GeoTeleporter.h"

#include "Characters/PlayableCharacter.h"
#include "Components/TextRenderComponent.h"
#include "Engine/ChildConnection.h"
#include "GameClasses/Match/GeoGameState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "Tool/Visual/GeoNiagaraParams.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The connection of the machine an actor's player sits on: a couch-coop player owns a UChildConnection of its
	 * machine's connection, and a locally hosted player owns none at all (nullptr, one "machine"). */
	UNetConnection* GetMachineConnection(AActor const* Actor)
	{
		UNetConnection* const Connection = Actor->GetNetConnection();
		UChildConnection* const ChildConnection = Connection ? Connection->GetUChildConnection() : nullptr;
		return ChildConnection ? ChildConnection->GetParentConnection() : Connection;
	}
} // namespace

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

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> PortalSystem(
		TEXT("/Game/Art/VFX/Teleporter/NS_TeleportPortal.NS_TeleportPortal"));
	PortalComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalComponent"));
	PortalComponent->SetupAttachment(MeshComponent);
	PortalComponent->SetAsset(PortalSystem.Object);
	PortalComponent->SetAutoActivate(false);

	bReplicates = true;
}

void AGeoTeleporter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGeoTeleporter, bOpen);
}

void AGeoTeleporter::SetOpen(bool bNewOpen)
{
	if (HasAuthority() && bOpen != bNewOpen)
	{
		bOpen = bNewOpen;
		ApplyOpen();
		PlayPortalTransition();
	}
}

void AGeoTeleporter::OnRep_Open()
{
	ApplyOpen();
	PlayPortalTransition();
}

void AGeoTeleporter::ApplyOpen()
{
	MeshComponent->SetCollisionEnabled(bOpen ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	if (!bOpen)
	{
		PendingArrivals.Empty();
	}

	TextComponent->SetVisibility(bOpen);
	OnOpenChanged(bOpen);
}

void AGeoTeleporter::PlayPortalTransition()
{
	if (bOpen)
	{
		PortalComponent->Activate(true);
	}
	else
	{
		PortalComponent->Deactivate();
	}
}

void AGeoTeleporter::SetPortalColor(FLinearColor const NewColor)
{
	PortalColor = NewColor;
	PortalComponent->SetVariableLinearColor(GeoNiagaraParams::Color, PortalColor);
}

void AGeoTeleporter::OnConstruction(FTransform const& Transform)
{
	Super::OnConstruction(Transform);
	TextComponent->SetText(DisplayText);

	// The mesh only shapes the overlap; the portal is the visual, and the label rides above it so they never z-fight.
	MeshComponent->SetVisibility(false);
	float const PortalHeight = 2.0f;
	float const LabelHeight = 5.0f;
	PortalComponent->SetRelativeLocation(FVector(0.0f, 0.0f, PortalHeight));
	FVector LabelLocation = TextComponent->GetRelativeLocation();
	LabelLocation.Z = LabelHeight;
	TextComponent->SetRelativeLocation(LabelLocation);

	SetPortalColor(PortalColor);
}

void AGeoTeleporter::BeginPlay()
{
	Super::BeginPlay();
	SetPortalColor(PortalColor);
	MeshComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnBeginOverlap);
	MeshComponent->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnEndOverlap);
	ApplyOpen();
	if (bOpen)
	{
		PortalComponent->Activate(true);
	}

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
	if (!PlayableCharacter || GetWorld()->GetGameStateChecked<AGeoGameState>()->IsMatchInProgress()
		|| !ensureMsgf(TeleportTag.IsValid(), TEXT("AGeoTeleporter %s has no TeleportTag."), *GetName()))
	{
		return;
	}

	if (PendingArrivals.Contains(PlayableCharacter))
	{
		LockArrivalMovement(*PlayableCharacter);
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

	UNetConnection const* const MachineConnection = GetMachineConnection(PlayableCharacter);
	FVector const Destination = NextTeleporter->GetActorLocation();
	int32 PlayerIndex = 0;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayableCharacter* const Traveller =
			Iterator->IsValid() ? Cast<APlayableCharacter>((*Iterator)->GetPawn()) : nullptr;
		if (!Traveller || GetMachineConnection(Traveller) != MachineConnection)
		{
			continue;
		}

		NextTeleporter->PendingArrivals.Add(Traveller);
		if (GeoLib::IsServer(GetWorld()))
		{
			Traveller->GetCharacterMovement()->CurrentRootMotion.Clear();

			FVector const Offset =
				NextTeleporter->GetActorRightVector() * (PlayerIndex * Traveller->GetSimpleCollisionRadius() * 2.0f);
			Traveller->TeleportTo(
				FVector(Destination.X + Offset.X, Destination.Y + Offset.Y, Traveller->GetActorLocation().Z),
				Traveller->GetActorRotation());
		}
		++PlayerIndex;
	}
}

void AGeoTeleporter::OnEndOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
								  UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	PendingArrivals.Remove(OtherActor);
}

void AGeoTeleporter::LockArrivalMovement(APlayableCharacter& Traveller) const
{
	AController* const Controller = Traveller.GetController();
	if (!Controller)
	{
		return;
	}

	Controller->SetIgnoreMoveInput(true);
	Traveller.GetCharacterMovement()->StopMovementImmediately();
	FTimerHandle MoveLockTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(MoveLockTimerHandle,
										   FTimerDelegate::CreateWeakLambda(Controller,
																			[Controller]()
																			{
																				Controller->SetIgnoreMoveInput(false);
																			}),
										   ArrivalMoveLockDuration, false);
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
