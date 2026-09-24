#include "Characters/Component/GeoCharacterMovementComponent.h"

#include "Characters/GeoCharacter.h"
#include "Settings/GameDataSettings.h"
#include "Tool/GeoNetcodeDebug.h"
#include "Tool/UGeoGameplayLibrary.h"

void FGeoSavedMove_Character::Clear()
{
	Super::Clear();
	PerceivedServerTime = 0.f;
	bWantsToDash = false;
	DashVelocity = FVector::ZeroVector;
	DashTimeRemaining = 0.f;
}

void FGeoSavedMove_Character::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAcceleration,
										 FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAcceleration, ClientData);
	PerceivedServerTime = GeoLib::GetServerTime(Character->GetWorld(), true);

	UGeoCharacterMovementComponent const* MovementComponent =
		CastChecked<UGeoCharacterMovementComponent>(Character->GetCharacterMovement());
	bWantsToDash = MovementComponent->bWantsToDash;
	DashVelocity = MovementComponent->DashVelocity;
	DashTimeRemaining = MovementComponent->DashTimeRemaining;
}

void FGeoSavedMove_Character::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	UGeoCharacterMovementComponent* MovementComponent =
		CastChecked<UGeoCharacterMovementComponent>(Character->GetCharacterMovement());
	MovementComponent->DashVelocity = DashVelocity;
	MovementComponent->DashTimeRemaining = DashTimeRemaining;
}

bool FGeoSavedMove_Character::CanCombineWith(FSavedMovePtr const& NewMove, ACharacter* Character,
											 float const MaxDelta) const
{
	FGeoSavedMove_Character const* NewGeoMove = static_cast<FGeoSavedMove_Character const*>(NewMove.Get());
	return DashTimeRemaining == 0.f && NewGeoMove->DashTimeRemaining == 0.f
		&& Super::CanCombineWith(NewMove, Character, MaxDelta);
}

uint8 FGeoSavedMove_Character::GetCompressedFlags() const
{
	uint8 Flags = Super::GetCompressedFlags();
	if (bWantsToDash)
	{
		Flags |= FLAG_Custom_0;
	}

	return Flags;
}

FGeoNetworkPredictionData_Client_Character::FGeoNetworkPredictionData_Client_Character(
	UCharacterMovementComponent const& ClientMovement) : FNetworkPredictionData_Client_Character(ClientMovement)
{
}

FSavedMovePtr FGeoNetworkPredictionData_Client_Character::AllocateNewMove()
{
	return FSavedMovePtr(new FGeoSavedMove_Character());
}

void FGeoCharacterNetworkMoveData::ClientFillNetworkMoveData(FSavedMove_Character const& ClientMove,
															 ENetworkMoveType MoveType)
{
	Super::ClientFillNetworkMoveData(ClientMove, MoveType);
	PerceivedServerTime = static_cast<FGeoSavedMove_Character const&>(ClientMove).PerceivedServerTime;
}

bool FGeoCharacterNetworkMoveData::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Archive,
											 UPackageMap* PackageMap, ENetworkMoveType MoveType)
{
	Super::Serialize(CharacterMovement, Archive, PackageMap, MoveType);
	Archive << PerceivedServerTime;
	return !Archive.IsError();
}

FGeoCharacterNetworkMoveDataContainer::FGeoCharacterNetworkMoveDataContainer()
{
	NewMoveData = &GeoMoveData[0];
	PendingMoveData = &GeoMoveData[1];
	OldMoveData = &GeoMoveData[2];
}

UGeoCharacterMovementComponent::UGeoCharacterMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // movement driven via ProcessInput calls

	// Corrections are eased into the mesh as a world-space offset; the default 0.1s blend reads as a visible
	// wobble at 50+ ping. Blend faster and snap past capsule-sized errors instead of gliding through them.
	NetworkSimulatedSmoothLocationTime = 0.05f;
	NetworkMaxSmoothUpdateDistance = 140.f;

	SetNetworkMoveDataContainer(GeoNetworkMoveDataContainer);
}

void UGeoCharacterMovementComponent::OnRegister()
{
	Super::OnRegister();
	// Cache the designer-configured base here, not in BeginPlay: on a listen-server host the pawn is possessed and its
	// MovementSpeedMultiplier attribute is applied (firing ApplySpeedMultiplier) BEFORE BeginPlay runs. Caching in
	// BeginPlay would then snapshot the already-multiplied (or zeroed) value. OnRegister runs before possession on
	// every net mode, so the cached base is always the real default.
	BaseMaxWalkSpeed = MaxWalkSpeed;
	BaseMaxAcceleration = MaxAcceleration;
}

void UGeoCharacterMovementComponent::TickComponent(float const DeltaTime, ELevelTick const TickType,
												   FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GeoLib::IsServer(GetWorld()))
	{
		FGeoPose const Pose = GeoLib::GetCurrentPose(GetOwner());
		float const OldestTimeNeeded = Pose.ServerTime - 2.f * GetDefault<UGameDataSettings>()->MaxLatencyCompensation;
		// Keeps the last pose at or before that time: a lookup there interpolates from it.
		while (PoseHistory.Num() > 1 && PoseHistory[1].ServerTime <= OldestTimeNeeded)
		{
			PoseHistory.PopFront();
		}
		PoseHistory.Add(Pose);
		FGeoNetcodeDebug::DrawRecordedPose(GetOwner(), Pose, Pose.ServerTime - OldestTimeNeeded);
		FGeoNetcodeDebug::DrawClientPose(CharacterOwner, Pose.ServerTime - OldestTimeNeeded);
	}
}

void UGeoCharacterMovementComponent::ApplySpeedMultiplier(float Multiplier)
{
	MaxWalkSpeed = BaseMaxWalkSpeed * Multiplier;
	MaxAcceleration = BaseMaxAcceleration * Multiplier;
}

void UGeoCharacterMovementComponent::RequestDash(FVector const& RequestVelocity, float const RequestDuration)
{
	DashVelocity = RequestVelocity;
	DashDuration = RequestDuration;
	if (CharacterOwner->IsLocallyControlled())
	{
		bWantsToDash = true;
	}
	else
	{
		bDashGranted = true;
	}
}

void UGeoCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float const DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	if (bWantsToDash && (CharacterOwner->IsLocallyControlled() || bDashGranted))
	{
		DashTimeRemaining = DashDuration;
		bDashGranted = false;
	}

	bWantsToDash = false;
}

void UGeoCharacterMovementComponent::UpdateCharacterStateAfterMovement(float const DeltaSeconds)
{
	Super::UpdateCharacterStateAfterMovement(DeltaSeconds);

	bool const bWasDashing = DashTimeRemaining > 0.f;
	DashTimeRemaining = FMath::Max(DashTimeRemaining - DeltaSeconds, 0.f);
	if (bWasDashing && DashTimeRemaining == 0.f)
	{
		Velocity = Velocity.GetClampedToMaxSize(GetMaxSpeed());
	}
}

void UGeoCharacterMovementComponent::CalcVelocity(float const DeltaTime, float const Friction, bool const bFluid,
												  float const BrakingDeceleration)
{
	if (DashTimeRemaining > 0.f)
	{
		Velocity = DashVelocity;
	}
	else
	{
		Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
	}
}

void UGeoCharacterMovementComponent::StopMovementImmediately()
{
	Super::StopMovementImmediately();
	DashTimeRemaining = 0.f;
}

void UGeoCharacterMovementComponent::UpdateFromCompressedFlags(uint8 const Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	bWantsToDash = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

bool UGeoCharacterMovementComponent::ClientUpdatePositionAfterServerUpdate()
{
	bool const bRealWantsToDash = bWantsToDash;
	FVector const RealDashVelocity = DashVelocity;
	bool const bReplayed = Super::ClientUpdatePositionAfterServerUpdate();
	bWantsToDash = bRealWantsToDash;
	DashVelocity = RealDashVelocity;
	return bReplayed;
}

FNetworkPredictionData_Client* UGeoCharacterMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		UGeoCharacterMovementComponent* const MutableThis = const_cast<UGeoCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FGeoNetworkPredictionData_Client_Character(*this);
	}

	return ClientPredictionData;
}

void UGeoCharacterMovementComponent::ServerMove_PerformMovement(FCharacterNetworkMoveData const& MoveData)
{
	Super::ServerMove_PerformMovement(MoveData);
	LastMoveServerTime = FMath::Max(LastMoveServerTime,
									static_cast<FGeoCharacterNetworkMoveData const&>(MoveData).PerceivedServerTime);
}

float UGeoCharacterMovementComponent::GetPerceivedServerTime() const
{
	float const ServerTime = GeoLib::GetServerTime(GetWorld(), true);
	if (!CharacterOwner->IsPlayerControlled() || CharacterOwner->IsLocallyControlled())
	{
		return ServerTime;
	}

	return FMath::Clamp(LastMoveServerTime, ServerTime - GetDefault<UGameDataSettings>()->MaxLatencyCompensation,
						ServerTime);
}

FGeoPose UGeoCharacterMovementComponent::GetPoseAt(float const ServerTime) const
{
	FGeoPose const CurrentPose = GeoLib::GetCurrentPose(GetOwner());
	FGeoPose Pose = CurrentPose;
	FGeoPose Later = CurrentPose;
	for (int32 Index = PoseHistory.Num() - 1; Index >= 0; --Index)
	{
		FGeoPose const& Earlier = PoseHistory[Index];
		if (Earlier.ServerTime <= ServerTime)
		{
			float const Span = Later.ServerTime - Earlier.ServerTime;
			float const Alpha = Span > 0.f ? FMath::Min((ServerTime - Earlier.ServerTime) / Span, 1.f) : 1.f;
			Pose = {ServerTime, FMath::Lerp(Earlier.Location, Later.Location, Alpha),
					Earlier.Yaw + FMath::FindDeltaAngleDegrees(Earlier.Yaw, Later.Yaw) * Alpha};
			break;
		}
		Later = Earlier;
		Pose = Earlier;
	}

	FGeoNetcodeDebug::DrawRewoundPose(GetOwner(), Pose, CurrentPose);
	return Pose;
}

void UGeoCharacterMovementComponent::ServerMoveHandleClientError(float ClientTimeStamp, float DeltaTime,
																 FVector const& Accel,
																 FVector const& RelativeClientLocation,
																 UPrimitiveComponent* ClientMovementBase,
																 FName ClientBaseBoneName, uint8 ClientMovementMode)
{
	bool const bCorpseFollowingClient = IsCorpseFollowingClient();
	bIgnoreClientMovementErrorChecksAndCorrection = bCorpseFollowingClient;
	bServerAcceptClientAuthoritativePosition = bCorpseFollowingClient;

	Super::ServerMoveHandleClientError(ClientTimeStamp, DeltaTime, Accel, RelativeClientLocation, ClientMovementBase,
									   ClientBaseBoneName, ClientMovementMode);
}

bool UGeoCharacterMovementComponent::IsCorpseFollowingClient() const
{
	AGeoCharacter const* const Character = GetGeoCharacter();
	return Character->IsDead()
		&& GeoLib::GetServerTime(GetWorld())
		<= Character->GetDeathServerTime() + 2.f * GetDefault<UGameDataSettings>()->MaxLatencyCompensation;
}

AGeoCharacter* UGeoCharacterMovementComponent::GetGeoCharacter() const
{
	return Cast<AGeoCharacter>(GetOwner());
}
