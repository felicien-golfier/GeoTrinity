#include "Tool/GameplayLibrary.h"

#include "AbilitySystem/Abilities/AbilityPayload.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Kismet/GameplayStatics.h"
#include "System/GeoActorPoolingSubsystem.h"
#include "System/GeoPoolableInterface.h"
#include "VisualLogger/VisualLogger.h"

bool GameplayLibrary::GetTeamInterface(AActor const* Actor, IGenericTeamAgentInterface const*& OutInterface)
{
	OutInterface = Cast<IGenericTeamAgentInterface const>(Actor);
	return OutInterface != nullptr;
}

FColor GameplayLibrary::GetColorForObject(UObject const* Object)
{
	if (!IsValid(Object))
	{
		return FColor::White;
	}

	static FColor const Palette[] = {FColor::Black,	  FColor::Red,	  FColor::Green,	 FColor::Blue,
									 FColor::Yellow,  FColor::Cyan,	  FColor::Magenta,	 FColor::Orange,
									 FColor::Emerald, FColor::Purple, FColor::Turquoise, FColor::Silver};

	return Palette[Object->GetUniqueID() % std::size(Palette)];
}

float GameplayLibrary::IsServer(UWorld const* World)
{
	return World->IsNetMode(NM_DedicatedServer) || World->IsNetMode(NM_ListenServer);
}

float GameplayLibrary::GetServerTime(UWorld const* World, bool const bUpdatedWithPing)
{
	if (IsServer(World))
	{
		return World->GetTimeSeconds();
	}

	float ServerTimeSeconds = World->GetGameState()->GetServerWorldTimeSeconds();

	if (bUpdatedWithPing)
	{
		APlayerController const* LocalPlayerController = World->GetFirstPlayerController();
		if (!IsValid(LocalPlayerController))
		{
			UE_LOG(LogTemp, Error, TEXT("No local player controller found"));
			return ServerTimeSeconds;
		}

		APlayerState const* PlayerState = LocalPlayerController->GetPlayerState<APlayerState>();
		if (!IsValid(PlayerState))
		{
			UE_LOG(LogTemp, Error, TEXT("No local player state found"));
			return ServerTimeSeconds;
		}
		float const OnWayPingSec =
			LocalPlayerController->GetPlayerState<APlayerState>()->GetPingInMilliseconds() * 0.0005f;
		ServerTimeSeconds += OnWayPingSec;
	}

	return ServerTimeSeconds;
}

int GameplayLibrary::GetAndCheckSection(UAnimMontage const* AnimMontage, FName const Section)
{
	int const SectionIndex = AnimMontage->GetSectionIndex(Section);
	if (SectionIndex == INDEX_NONE)
	{
		ensureMsgf(true, TEXT("Section %s not found in AnimMontage %s"), *Section.ToString(), *AnimMontage->GetName());
		UE_LOG(LogPattern, Error, TEXT("Section %s not found in AnimMontage %s"), *Section.ToString(),
			   *AnimMontage->GetName());
	}
	return SectionIndex;
}

UAnimInstance* GameplayLibrary::GetAnimInstance(FAbilityPayload const& Payload)
{
	ACharacter* InstigatorCharacter = Cast<ACharacter>(Payload.Instigator);
	if (!IsValid(InstigatorCharacter))
	{
		UE_LOG(LogPattern, Error, TEXT("We support only animation montage for character in pattern for now !"));
		return nullptr;
	}

	UAnimInstance* AnimInstance =
		InstigatorCharacter->GetMesh() ? InstigatorCharacter->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogPattern, Error, TEXT("Please set an anim instance (With the Default Slot filled in anim graph;)"));
		return nullptr;
	}

	return AnimInstance;
}

AGeoProjectile* GameplayLibrary::SpawnProjectile(UWorld* const World, TSubclassOf<AGeoProjectile> const ProjectileClass,
												 FTransform const& SpawnTransform, FAbilityPayload const& Payload,
												 TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
												 float const SpawnDelayFromPayloadTime, FPredictionKey PredictionKey)
{
	if (!World || !ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameplayLibrary::SpawnProjectile] Invalid World or ProjectileClass"));
		return nullptr;
	}

	AGeoProjectile* Projectile;
	bool const bIsPoolable = ProjectileClass->ImplementsInterface(UGeoPoolableInterface::StaticClass());
	if (bIsPoolable)
	{
		Projectile = UGeoActorPoolingSubsystem::Get(World)->RequestActor(ProjectileClass, SpawnTransform, Payload.Owner,
																		 Cast<APawn>(Payload.Owner), false);
		if (!Projectile)
		{
			UE_LOG(LogTemp, Error, TEXT("[GameplayLibrary::SpawnProjectile] RequestActor returned nullptr for %s"),
				   *ProjectileClass->GetName());
			return nullptr;
		}
	}
	else
	{
		Projectile = World->SpawnActorDeferred<AGeoProjectile>(ProjectileClass, SpawnTransform, Payload.Owner,
															   Cast<APawn>(Payload.Instigator),
															   ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (!Projectile)
		{
			UE_LOG(LogTemp, Error, TEXT("[GameplayLibrary::SpawnProjectile] SpawnActor returned nullptr for %s"),
				   *ProjectileClass->GetName());
			return nullptr;
		}
	}

	Projectile->Payload = Payload;
	Projectile->EffectDataArray = EffectDataArray;
	Projectile->PredictionKeyId = PredictionKey.Current;

	if (bIsPoolable)
	{
		Cast<IGeoPoolableInterface>(Projectile)->Init();
	}
	else
	{
		UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
		Projectile->InitProjectileLife();

		// Register prediction key delegates as fast-path cleanup for when the ability
		// is confirmed AFTER the projectile spawns (no FireRate delay).
		// For the FireRate-delayed case, BeginPlay proximity matching handles it instead.
		if (!IsServer(World) && PredictionKey.IsLocalClientKey())
		{
			TWeakObjectPtr<AGeoProjectile> WeakProjectile(Projectile);
			auto DestroyFake = [WeakProjectile]()
			{
				if (WeakProjectile.IsValid())
				{
					WeakProjectile->Destroy();
				}
			};
			PredictionKey.NewCaughtUpDelegate().BindLambda(DestroyFake);
			PredictionKey.NewRejectedDelegate().BindLambda(DestroyFake);
		}
	}

	if (IsServer(World))
	{
		float const TimeDelta = GetServerTime(World) - Payload.ServerSpawnTime - SpawnDelayFromPayloadTime;
		if (TimeDelta > 0.f)
		{
			Projectile->AdvanceProjectile(TimeDelta);
		}
	}

	return Projectile;
}

TArray<FVector> GameplayLibrary::GetTargetDirections(UWorld const* World, EProjectileTarget const Target,
													 float const Yaw, FVector const& Origin)
{
	switch (Target)
	{
	case EProjectileTarget::Forward:
		{
			return {FRotator(0.f, Yaw, 0.f).Vector()};
		}

	case EProjectileTarget::AllPlayers:
		{
			TArray<FVector> Directions;
			for (auto PlayerControllerIt = World->GetPlayerControllerIterator(); PlayerControllerIt;
				 ++PlayerControllerIt)
			{
				if (APlayerController const* PlayerController = PlayerControllerIt->Get();
					PlayerController && PlayerController->GetPawn())
				{
					Directions.Add((PlayerController->GetPawn()->GetActorLocation() - Origin).GetSafeNormal());
				}
			}
			return Directions;
		}

	default:
		{
			return {};
		}
	}
}
