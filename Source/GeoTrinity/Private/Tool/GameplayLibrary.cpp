#include "Tool/GameplayLibrary.h"

#include "AbilitySystem/Abilities/AbilityPayload.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Characters/PlayableCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Settings/GameDataSettings.h"
#include "System/GeoActorPoolingSubsystem.h"
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

AGeoProjectile* GameplayLibrary::SpawnProjectile(UWorld* World, TSubclassOf<AGeoProjectile> ProjectileClass,
												 FTransform const& SpawnTransform, FAbilityPayload const& Payload,
												 TArray<TInstancedStruct<FEffectData>> const& EffectDataArray)
{
	if (!World || !ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameplayLibrary::SpawnProjectile] Invalid World or ProjectileClass"));
		return nullptr;
	}


	AGeoProjectile* Projectile = UGeoActorPoolingSubsystem::Get(World)->RequestActor(
		ProjectileClass, SpawnTransform, Payload.Owner, Cast<APawn>(Payload.Owner), false);

	if (!Projectile)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameplayLibrary::SpawnProjectile] RequestActor returned nullptr for %s"),
			   *ProjectileClass->GetName());
		return nullptr;
	}

	Projectile->Payload = Payload;
	Projectile->EffectDataArray = EffectDataArray;
	Projectile->Init();

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

float GameplayLibrary::GetYawWithNetworkDelay(AActor* const Avatar, float const NetworkDelay)
{
	float const ActualYaw = Avatar->GetActorRotation().Yaw;
	float ProjectileYaw = ActualYaw;

	if (Avatar->HasAuthority() && GetDefault<UGameDataSettings>()->bNetworkDelayCompensation)
	{
		if (APlayableCharacter const* PlayableCharacter = Cast<APlayableCharacter>(Avatar))
		{
			ProjectileYaw += PlayableCharacter->GetYawVelocity() * NetworkDelay;
		}
	}

	FVector const Start = Avatar->GetActorLocation();
	constexpr float ArrowLength = 500.f;

	FVector const ActualEnd = Start + FRotator(0.f, ActualYaw, 0.f).Vector() * ArrowLength;
	UE_VLOG_ARROW(Avatar, LogGeoASC, Verbose, Start, ActualEnd, FColor::Red, TEXT("%s ActualYaw=%.2f"),
				  Avatar->HasAuthority() ? TEXT("Server") : TEXT("Client"), ActualYaw);

	FVector const PredictedEnd = Start + FRotator(0.f, ProjectileYaw, 0.f).Vector() * ArrowLength;
	UE_VLOG_ARROW(Avatar, LogGeoASC, Verbose, Start, PredictedEnd, FColor::Blue,
				  TEXT("%s PredictedYaw=%.2f (delta=%.2f, ND=%.4f)"),
				  Avatar->HasAuthority() ? TEXT("Server") : TEXT("Client"), ProjectileYaw, ProjectileYaw - ActualYaw,
				  NetworkDelay);

	return ProjectileYaw;
}
