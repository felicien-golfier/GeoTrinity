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
 * Deriving just to set up basic RPG stuff for now (felt awkward to put all of this in the controller)
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
	void InitOverlay();

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
