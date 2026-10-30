// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Arena/GeoDummyArena.h"
#include "Actor/Deployable/Zones/GeoEffectZone.h"
#include "Characters/GeoSummonedDummy.h"
#include "Characters/HiddenCasterCharacter.h"
#include "CoreMinimal.h"
#include "Encounter/Action/GeoStartEncounterAction.h"
#include "Encounter/GeoCompareCondition.h"
#include "Encounter/GeoEncounter.h"
#include "Encounter/GeoEncounterAction.h"
#include "Encounter/GeoEncounterTrigger.h"
#include "GameFramework/HUD.h"
#include "HUD/Interface/GeoHUDInterface.h"

#include "GeoTestEncounter.generated.h"

/** Trigger a test fires by hand (FireNow) or from OnArm; counts its arming and disarming. */
UCLASS(NotPlaceable, Transient)
class UGeoTestTrigger : public UGeoEncounterTrigger
{
	GENERATED_BODY()

public:
	virtual void OnArm_Implementation() override
	{
		++ArmCount;
		if (bFireOnArm)
		{
			Fire(FGeoEncounterPayload());
		}
	}

	virtual void OnDisarm_Implementation() override { ++DisarmCount; }

	/** Fires the trigger directly with an optional Count, without waiting for an event. */
	void FireNow(int32 const Count = 0) { Fire(MakePayload(Count)); }

	/** Returns a payload with Count set; used by FireNow and tests that need a payload to pass to a condition. */
	static FGeoEncounterPayload MakePayload(int32 const Count)
	{
		FGeoEncounterPayload Payload;
		Payload.Count = Count;
		return Payload;
	}

	bool bFireOnArm = false;
	int32 ArmCount = 0;
	int32 DisarmCount = 0;
};

/** Action that counts its runs and skips and notes its Id in the shared Order list on both. */
UCLASS(NotPlaceable, Transient)
class UGeoTestAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override
	{
		++Runs;
		LastPayload = Payload;
		if (Order)
		{
			Order->Add(Id);
		}
	}

	virtual void OnCancel_Implementation() override { ++Cancels; }

	virtual void OnSkipped_Implementation() override
	{
		++Skips;
		if (Order)
		{
			Order->Add(Id);
		}
	}

	FName Id;
	TArray<FName>* Order = nullptr;
	int32 Runs = 0;
	int32 Cancels = 0;
	int32 Skips = 0;
	FGeoEncounterPayload LastPayload;
};

/** Action that finishes only when the test calls Finish. */
UCLASS(NotPlaceable, Transient)
class UGeoTestLatentAction : public UGeoTestAction
{
	GENERATED_BODY()

public:
	UGeoTestLatentAction() { bLatent = true; }
};

/** Action that asks its encounter to go to a step from inside Execute. */
UCLASS(NotPlaceable, Transient)
class UGeoTestGoToStepAction : public UGeoTestAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override
	{
		Super::Execute_Implementation(Payload);
		GetEncounter()->GoToStep(StepName);
	}

	FName StepName;
};

/** Arena that spawns a hidden caster as its boss, the way a room with no boss hosts one. Its setups are open to a test
 * between SpawnActorDeferred and FinishSpawning. */
UCLASS(NotPlaceable, Transient)
class AGeoTestCasterArena : public AGeoDummyArena
{
	GENERATED_BODY()

public:
	AGeoTestCasterArena() { BossClass = AHiddenCasterCharacter::StaticClass(); }

	/** Mutable boss setup for test configuration before FinishSpawning. */
	FGeoEnemySetup& EditBossSetup() { return BossSetup; }
	/** Mutable start-add list for test configuration before FinishSpawning. */
	TArray<FGeoArenaAdd>& EditStartAdds() { return StartAdds; }
};

/** Counts the times an encounter completed. */
UCLASS(NotPlaceable, Transient)
class UGeoTestCompletionListener : public UObject
{
	GENERATED_BODY()

public:
	/** Counts each OnCompleted broadcast so a test can assert how many times the encounter finished. */
	UFUNCTION()
	void OnCompleted() { ++Completions; }

	int32 Completions = 0;
};

/** Encounter whose steps a test builds in code. Spawn it, add steps, then rules, then Start. */
UCLASS(NotPlaceable, Transient)
class AGeoTestEncounter : public AGeoEncounter
{
	GENERATED_BODY()

public:
	/** Mutable step list so a test can push steps and rules before calling Start. */
	TArray<FGeoEncounterStep>& EditSteps() { return Steps; }

	/** Controls whether the encounter auto-starts on BeginPlay, for tests that call Start manually. */
	void SetStartOnBeginPlay(bool const bStart) { bStartOnBeginPlay = bStart; }
	/** Sets the player start this encounter lands the pawn on during a jump. */
	void SetJumpStart(APlayerStart* Start) { JumpStart = Start; }

	/** A GeoStartEncounterAction of this encounter starting Next: the link of a chain. */
	UGeoStartEncounterAction* NewStartAction(AGeoEncounter* Next)
	{
		UGeoStartEncounterAction* const Action = NewObject<UGeoStartEncounterAction>(this);
		Action->Target = Next;
		return Action;
	}

	/** An action of class T noting Id in Order. */
	template <class T = UGeoTestAction>
	T* NewAction(FName const Id, TArray<FName>* Order = nullptr)
	{
		T* const Action = NewObject<T>(this);
		Action->Id = Id;
		Action->Order = Order;
		return Action;
	}

	/** Adds a rule to step StepIndex and returns its trigger. */
	UGeoTestTrigger* AddRule(int32 const StepIndex, TArray<TObjectPtr<UGeoEncounterAction>> Actions,
							 bool const bRepeat = false, bool const bCompletesStep = false,
							 FName const StepAfter = NAME_None, UGeoEncounterCondition* Condition = nullptr)
	{
		FGeoEncounterRule& Rule = Steps[StepIndex].Rules.AddDefaulted_GetRef();
		UGeoTestTrigger* const Trigger = NewObject<UGeoTestTrigger>(this);
		Rule.Trigger = Trigger;
		Rule.Condition = Condition;
		Rule.Actions = MoveTemp(Actions);
		Rule.bRepeat = bRepeat;
		Rule.bCompletesStep = bCompletesStep;
		Rule.StepAfter = StepAfter;
		return Trigger;
	}

	/** Adds a rule driven by Trigger, which the caller made. */
	void AddRuleWithTrigger(int32 const StepIndex, UGeoEncounterTrigger* Trigger,
							TArray<TObjectPtr<UGeoEncounterAction>> Actions, bool const bRepeat = false,
							UGeoEncounterCondition* Condition = nullptr)
	{
		FGeoEncounterRule& Rule = Steps[StepIndex].Rules.AddDefaulted_GetRef();
		Rule.Trigger = Trigger;
		Rule.Condition = Condition;
		Rule.Actions = MoveTemp(Actions);
		Rule.bRepeat = bRepeat;
	}

	/** Creates a GeoCompareCondition owned by this encounter; CounterName required for the Counter source. */
	UGeoCompareCondition* NewCompare(EGeoCompareSource const Source, EGeoCompareOperator const Comparison,
									 float const Operand, FName const CounterName = NAME_None)
	{
		UGeoCompareCondition* const Condition = NewObject<UGeoCompareCondition>(this);
		Condition->Source = Source;
		Condition->Comparison = Comparison;
		Condition->Operand = Operand;
		Condition->CounterName = CounterName;
		return Condition;
	}

	/** Adds Count steps named A, B, C... */
	void AddSteps(int32 const Count)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Steps.AddDefaulted_GetRef().Name = FName(*FString(1, TEXT("ABCDEFGH") + Index));
		}
	}
};

/** HUD that records the Cat lines asked of it and holds the last line's end callback for the test to call. */
UCLASS(NotPlaceable, Transient)
class AGeoTestHUD
	: public AHUD
	, public IGeoHUDInterface
{
	GENERATED_BODY()

public:
	/** No-op stub; tests do not need a real overlay. */
	virtual void InitOverlay(APlayerController* /*PC*/, APlayerState* /*PS*/, UAbilitySystemComponent* /*ASC*/,
							 UAttributeSet* /*AS*/) override
	{
	}
	/** No-op stub. */
	virtual void BindToPawn(APlayableCharacter* /*PlayableCharacter*/) override {}
	/** No-op stub. */
	virtual void BuildAbilityBar(APlayableCharacter* /*PlayableCharacter*/) override {}
	/** No-op stub. */
	virtual void ShowBossHealthBar(AEnemyCharacter* /*Boss*/) override {}
	/** No-op stub. */
	virtual void HideBossHealthBar() override {}

	/** Records the line text and holds OnEnded so a test can trigger the end manually via HideCatLine. */
	virtual void ShowCatLine(FText const& Line, FSimpleDelegate OnEnded) override
	{
		Lines.Add(Line.ToString());
		LineEnded = MoveTemp(OnEnded);
	}

	/** Fires the held OnEnded callback, advancing the encounter past the Cat line action. */
	virtual void HideCatLine() override
	{
		++HiddenLines;
		FSimpleDelegate const Waiter = MoveTemp(LineEnded);
		Waiter.ExecuteIfBound();
	}

	/** Holds OnClosed so a test can dismiss the page by calling PageClosed.ExecuteIfBound(). */
	virtual void ShowAbilityPage(FGameplayTag /*AbilityTag*/, FSimpleDelegate OnClosed) override
	{
		PageClosed = MoveTemp(OnClosed);
	}
	/** Tracks that the quest card is visible. */
	virtual void ShowQuestCard(FText const& /*Title*/, int32 /*Max*/, float /*TimeLimitSeconds*/) override
	{
		bQuestCardShown = true;
	}
	/** No-op stub. */
	virtual void SetQuestCount(int32 /*Count*/) override {}
	/** Clears the quest-card visibility flag. */
	virtual void HideQuestCard() override { bQuestCardShown = false; }
	/** No-op stub. */
	virtual void OpenGemsPage(bool /*bForge*/) override {}

	int32 HiddenLines = 0;
	bool bQuestCardShown = false;
	TArray<FString> Lines;
	FSimpleDelegate LineEnded;
	FSimpleDelegate PageClosed;
};

/** Effect zone with its radius and spawn size readable by a test. */
UCLASS(NotPlaceable, Transient)
class AGeoTestReadableZone : public AGeoEffectZone
{
	GENERATED_BODY()

public:
	/** The zone's current collision radius, for asserting growth in tests. */
	float GetTestRadius() const { return GetRadius(); }
	/** The spawn size from the zone's data params, for asserting the initial size. */
	float GetSpawnSize() const { return Data.Params.Size; }
	/** The server time at which this zone began growing, for testing late-join replication. */
	float GetGrowthStartServerTime() const { return GrowthStartServerTime; }
	/** What a client that receives the zone late gets: the server's growth start, not its own BeginPlay. */
	void ReceiveGrowthStartServerTime(float const ServerTime) { GrowthStartServerTime = ServerTime; }
};

/** Summoned dummy with its zone and ally set, as its Blueprint would: the zone grows, the ally is a plain enemy tagged
 * "Ally". Its zone class is the Game Data Settings' DefaultZoneClass, which the test sets. */
UCLASS(NotPlaceable, Transient)
class AGeoTestSummonedDummy : public AGeoSummonedDummy
{
	GENERATED_BODY()

public:
	AGeoTestSummonedDummy()
	{
		ZoneParams.Size = 100.f;
		ZoneParams.GrowToSize = FScalableFloat(600.f);
		ZoneParams.GrowSeconds = FScalableFloat(10.f);
		AllyClass = AEnemyCharacter::StaticClass();
		AllySetup.Team = ETeam::Player;
		AllySetup.Tag = TEXT("Ally");
	}
};
