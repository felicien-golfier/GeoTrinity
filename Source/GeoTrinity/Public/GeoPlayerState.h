#pragma once

#include "AbilitySystemInterface.h"
#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GenericTeamAgentInterface.h"

class APlayableCharacter;

#include "GeoPlayerState.generated.h"

class UCharacterAttributeSet;
class UGeoAbilitySystemComponent;
/**
 * Player state for GeoTrinity. Owns the ASC and UCharacterAttributeSet for playable characters
 * (ASC lives here so it survives pawn respawns). Also tracks the player's current class,
 * replicated combat debug stats (DPS, HPS, damage received), and triggers HUD overlay initialization.
 */
UCLASS()
class GEOTRINITY_API AGeoPlayerState
	: public APlayerState
	, public IAbilitySystemInterface
	, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AGeoPlayerState();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void ClientInitialize(AController* Controller) override;
	/** Creates the HUD overlay widget on the owning client. Called from ClientInitialize once the controller is valid. */
	void InitOverlay();

	/** Callback for APlayerState::PawnSetDelegate. Triggers InitGAS on the pawn when it is first assigned. */
	UFUNCTION()
	void OnPlayerPawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn);

	/** Implement IAbilitySystemInterface */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	/** END Implement IAbilitySystemInterface */

	// IGenericTeamAgentInterface BEGIN
	virtual FGenericTeamId GetGenericTeamId() const override;
	// IGameplayTaskOwnerInterface END

	UCharacterAttributeSet* GetCharacterAttributeSet() const { return CharacterAttributeSet; }
	UGeoAbilitySystemComponent* GetGeoAbilitySystemComponent() const { return AbilitySystemComponent; }

	EPlayerClass GetPlayerClass() const { return PlayerClass; }
	void SetPlayerClass(EPlayerClass NewClass) { PlayerClass = NewClass; }

	float GetDebugDPS() const { return DebugDPS; }
	float GetDebugHPS() const { return DebugHPS; }
	float GetDebugRecv() const { return DebugRecv; }
	float GetTotalDamageDealt() const { return TotalDamageDealt; }
	float GetTotalHealingDealt() const { return TotalHealingDealt; }
	float GetTotalDamageReceived() const { return TotalDamageReceived; }
	void SetDebugCombatStats(float DPS, float HPS, float Recv, float TotDmg, float TotHeal, float TotRecv)
	{
		DebugDPS = DPS;
		DebugHPS = HPS;
		DebugRecv = Recv;
		TotalDamageDealt = TotDmg;
		TotalHealingDealt = TotHeal;
		TotalDamageReceived = TotRecv;
	}

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGeoAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UCharacterAttributeSet> CharacterAttributeSet;

	UPROPERTY(ReplicatedUsing = OnRep_PlayerClass, EditAnywhere, BlueprintReadOnly, Category = "Class")
	EPlayerClass PlayerClass = EPlayerClass::None;

	UFUNCTION()
	void OnRep_PlayerClass();

	UPROPERTY(Replicated)
	float DebugDPS = 0.f;
	UPROPERTY(Replicated)
	float DebugHPS = 0.f;
	UPROPERTY(Replicated)
	float DebugRecv = 0.f;
	UPROPERTY(Replicated)
	float TotalDamageDealt = 0.f;
	UPROPERTY(Replicated)
	float TotalHealingDealt = 0.f;
	UPROPERTY(Replicated)
	float TotalDamageReceived = 0.f;
};
