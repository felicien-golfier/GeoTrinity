#include "Characters/Component/GeoCharacterMovementComponent.h"

#include "Characters/GeoCharacter.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

void FGeoSavedMove_Character::Clear()
{
	Super::Clear();
	PerceivedServerTime = 0.f;
}

void FGeoSavedMove_Character::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAcceleration,
										 FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAcceleration, ClientData);
	PerceivedServerTime = GeoLib::GetServerTime(Character->GetWorld(), true);
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

void UGeoCharacterMovementComponent::ApplySpeedMultiplier(float Multiplier)
{
	MaxWalkSpeed = BaseMaxWalkSpeed * Multiplier;
	MaxAcceleration = BaseMaxAcceleration * Multiplier;
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

void UGeoCharacterMovementComponent::ServerMoveHandleClientError(float ClientTimeStamp, float DeltaTime,
																 FVector const& Accel,
																 FVector const& RelativeClientLocation,
																 UPrimitiveComponent* ClientMovementBase,
																 FName ClientBaseBoneName, uint8 ClientMovementMode)
{
	if (IsCorpseFollowingClient())
	{
		UpdatedComponent->SetWorldLocation(RelativeClientLocation);
	}

	Super::ServerMoveHandleClientError(ClientTimeStamp, DeltaTime, Accel, RelativeClientLocation, ClientMovementBase,
									   ClientBaseBoneName, ClientMovementMode);
}

bool UGeoCharacterMovementComponent::ServerCheckClientError(float ClientTimeStamp, float DeltaTime,
															FVector const& Accel, FVector const& ClientWorldLocation,
															FVector const& RelativeClientLocation,
															UPrimitiveComponent* ClientMovementBase,
															FName ClientBaseBoneName, uint8 ClientMovementMode)
{
	return !IsCorpseFollowingClient()
		&& Super::ServerCheckClientError(ClientTimeStamp, DeltaTime, Accel, ClientWorldLocation, RelativeClientLocation,
										 ClientMovementBase, ClientBaseBoneName, ClientMovementMode);
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
