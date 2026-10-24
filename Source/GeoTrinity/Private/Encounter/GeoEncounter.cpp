// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/GeoEncounter.h"

#include "Components/SceneComponent.h"
#include "Encounter/Action/GeoStartEncounterAction.h"
#include "Encounter/GeoCompareCondition.h"
#include "Encounter/GeoEncounterAction.h"
#include "Encounter/GeoEncounterCondition.h"
#include "Encounter/GeoEncounterTrigger.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Misc/DataValidation.h"
#include "Tool/UGeoGameplayLibrary.h"
#if WITH_EDITOR
#include "World/GeoWorldSettings.h"
#endif
AGeoEncounter::AGeoEncounter()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AGeoEncounter::BeginPlay()
{
	Super::BeginPlay();
	AGeoEncounter const* const JumpTarget = FindJumpTarget();
	if (JumpTarget == this)
	{
		MovePlayerToJumpStart();
		StartAfterSkippingChain();
	}
	else if (!JumpTarget && bStartOnBeginPlay)
	{
		Start();
	}
}

void AGeoEncounter::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	Pending = FPendingChange();
	StopCurrentStep();
	State = EState::Idle;
	Super::EndPlay(EndPlayReason);
}

void AGeoEncounter::Start()
{
	if (State == EState::Idle)
	{
		RequestChange({.Target = 0, .bRunExit = false});
	}
	else
	{
		UE_LOG(LogGeoEncounter, Warning, TEXT("%s: Start while running, use Restart"), *GetName());
	}
}

void AGeoEncounter::Abort()
{
	if (GeoLib::IsServer(this))
	{
		TGuardValue<bool> const Aborting(bAborting, true);
		Pending = FPendingChange();
		StopCurrentStep();
		State = EState::Idle;
		CurrentStep = INDEX_NONE;

		auto const CancelAll = [](TArray<TObjectPtr<UGeoEncounterAction>> const& Actions)
		{
			for (UGeoEncounterAction* const Action : Actions)
			{
				if (Action)
				{
					Action->OnCancel();
				}
			}
		};
		for (FGeoEncounterStep const& Step : Steps)
		{
			CancelAll(Step.OnEnter);
			for (FGeoEncounterRule const& Rule : Step.Rules)
			{
				CancelAll(Rule.Actions);
			}

			CancelAll(Step.OnExit);
		}
	}
}

void AGeoEncounter::StartAfterSkippingChain()
{
	if (GeoLib::IsServer(this))
	{
		for (TActorIterator<AGeoEncounter> It(GetWorld()); It; ++It)
		{
			if (*It != this)
			{
				It->Abort();
			}
		}

		TArray<AGeoEncounter const*> Skipped;
		AGeoEncounter const* Link = FindChainStart();
		while (Link && Link != this && !Skipped.Contains(Link))
		{
			Skipped.Add(Link);
			Link = Link->FindNextInChain();
		}

		if (Link == this)
		{
			for (AGeoEncounter const* const Encounter : Skipped)
			{
				Encounter->SkipActions();
			}
		}
		else
		{
			UE_LOG(LogGeoEncounter, Warning, TEXT("%s: not on the chain of the start encounter, nothing skipped"),
				   *GetName());
		}
	}

	Restart();
}

AGeoEncounter* AGeoEncounter::FindJumpTarget() const
{
	FString Portal = GetWorld()->URL.Portal;

#if WITH_EDITOR
	if (Portal.IsEmpty()) // Let the previous map launch the step it needs.
	{
		if (AGeoWorldSettings const* GeoWorldSettings = Cast<AGeoWorldSettings>(GetWorld()->GetWorldSettings()))
		{
			Portal = GeoWorldSettings->StartingTutoRoomInEditor.ToString();
		}
	}
#endif

	AGeoEncounter* Target = nullptr;
	if (Portal.IsNumeric())
	{
		Target = FindChainStart();
		for (int32 LinksLeft = FCString::Atoi(*Portal); Target && LinksLeft > 0; --LinksLeft)
		{
			Target = Target->FindNextInChain();
		}
	}
	else if (!Portal.IsEmpty())
	{
		for (TActorIterator<AGeoEncounter> It(GetWorld()); It && !Target; ++It)
		{
			if (It->JumpStart && It->JumpStart->PlayerStartTag == FName(Portal))
			{
				Target = *It;
			}
		}
	}

	return Target;
}

AGeoEncounter* AGeoEncounter::FindChainStart() const
{
	for (TActorIterator<AGeoEncounter> It(GetWorld()); It; ++It)
	{
		if (It->bStartOnBeginPlay)
		{
			return *It;
		}
	}

	return nullptr;
}

void AGeoEncounter::MovePlayerToJumpStart() const
{
	APlayerController const* const PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* const Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (JumpStart && Pawn)
	{
		Pawn->TeleportTo(JumpStart->GetActorLocation(), JumpStart->GetActorRotation());
	}
	else if (JumpStart)
	{
		UE_LOG(LogGeoEncounter, Warning, TEXT("%s: no local pawn to move to the jump start"), *GetName());
	}
}

AGeoEncounter* AGeoEncounter::FindNextInChain() const
{
	auto const FindIn = [](TArray<TObjectPtr<UGeoEncounterAction>> const& Actions) -> AGeoEncounter*
	{
		for (UGeoEncounterAction const* const Action : Actions)
		{
			UGeoStartEncounterAction const* const StartAction = Cast<UGeoStartEncounterAction>(Action);
			if (StartAction && StartAction->Target)
			{
				return StartAction->Target;
			}
		}

		return nullptr;
	};

	for (FGeoEncounterStep const& Step : Steps)
	{
		AGeoEncounter* Next = FindIn(Step.OnEnter);
		for (FGeoEncounterRule const& Rule : Step.Rules)
		{
			Next = Next ? Next : FindIn(Rule.Actions);
		}

		Next = Next ? Next : FindIn(Step.OnExit);
		if (Next)
		{
			return Next;
		}
	}

	return nullptr;
}

void AGeoEncounter::SkipActions() const
{
	for (FGeoEncounterStep const& Step : Steps)
	{
		for (UGeoEncounterAction* const Action : Step.OnEnter)
		{
			if (Action)
			{
				Action->OnSkipped();
			}
		}

		for (UGeoEncounterAction* const Action : Step.OnExit)
		{
			if (Action)
			{
				Action->OnSkipped();
			}
		}
	}
}

void AGeoEncounter::Restart()
{
	RequestChange({.Target = 0, .bRunExit = false});
}

void AGeoEncounter::GoToStep(FName const StepName)
{
	int32 const Target = FindStep(StepName);
	if (Target != INDEX_NONE)
	{
		RequestChange({.Target = Target, .bRunExit = true});
	}
	else
	{
		UE_LOG(LogGeoEncounter, Error, TEXT("%s: GoToStep: no step named %s"), *GetName(), *StepName.ToString());
	}
}

void AGeoEncounter::AddToEncounterCounter(FName const Counter, int32 const Delta)
{
	int32 const NewValue = Counters.FindOrAdd(Counter) += Delta;
	OnCounterChanged.Broadcast(Counter, NewValue);
}

int32 AGeoEncounter::FindStep(FName const StepName) const
{
	return Steps.IndexOfByPredicate(
		[StepName](FGeoEncounterStep const& Step)
		{
			return Step.Name == StepName;
		});
}

void AGeoEncounter::RequestChange(FPendingChange const Change)
{
	bool const bInPlay = HasActorBegunPlay() || IsActorBeginningPlay();
	bool const bAccepted =
		bInPlay && GeoLib::IsServer(this) && !Pending.IsSet() && (!Change.bRunExit || State == EState::Running);
	if (bAccepted)
	{
		Pending = Change;
		if (CallDepth == 0)
		{
			ApplyPending();
		}
	}
	else
	{
		UE_LOG(LogGeoEncounter, Verbose, TEXT("%s: step change to %d dropped"), *GetName(), Change.Target);
	}
}

void AGeoEncounter::ApplyPending()
{
	++CallDepth;
	while (Pending.IsSet())
	{
		FPendingChange const Change = Pending;
		Pending = FPendingChange();

		StopCurrentStep();
		if (Change.bRunExit && !Steps[CurrentStep].OnExit.IsEmpty())
		{
			State = EState::Exiting;
			ExitTarget = Change.Target;
			StartRun(ExitRun, INDEX_NONE, Steps[CurrentStep].OnExit, FGeoEncounterPayload());
		}
		else
		{
			EnterStep(Change.Target);
		}
	}
	--CallDepth;
}

void AGeoEncounter::StopCurrentStep()
{
	for (int32 RuleIndex = 0; RuleIndex < RuleArmed.Num(); ++RuleIndex)
	{
		DisarmRule(RuleIndex);
	}

	CancelRun(EnterRun);
	CancelRun(ExitRun);
	for (FGeoEncounterActionRun& Run : RuleRuns)
	{
		CancelRun(Run);
	}

	TArray<TObjectPtr<UGeoEncounterAction>> const Held = MoveTemp(HeldActions);
	HeldActions.Reset();
	for (UGeoEncounterAction* const Action : Held)
	{
		if (Action)
		{
			Action->OnCancel();
		}
	}
}

void AGeoEncounter::HoldUntilStepLeft(UGeoEncounterAction& Action)
{
	HeldActions.AddUnique(&Action);
}

void AGeoEncounter::EnterStep(int32 const StepIndex)
{
	Counters.Reset();
	RuleArmed.Reset();
	RuleRuns.Reset();

	if (Steps.IsValidIndex(StepIndex))
	{
		FGeoEncounterStep const& Step = Steps[StepIndex];
		UE_LOG(LogGeoEncounter, Verbose, TEXT("%s: enter step %s"), *GetName(), *Step.Name.ToString());
		CurrentStep = StepIndex;
		State = EState::Running;
		RuleArmed.Init(true, Step.Rules.Num());
		RuleRuns.SetNum(Step.Rules.Num());
		for (FGeoEncounterRule const& Rule : Step.Rules)
		{
			if (Rule.Trigger)
			{
				Rule.Trigger->OnArm();
			}
		}

		if (!Pending.IsSet())
		{
			StartRun(EnterRun, INDEX_NONE, Step.OnEnter, FGeoEncounterPayload());
		}
	}
	else
	{
		UE_LOG(LogGeoEncounter, Verbose, TEXT("%s: completed"), *GetName());
		CurrentStep = INDEX_NONE;
		State = EState::Idle;
		OnCompleted.Broadcast();
	}
}

void AGeoEncounter::DisarmRule(int32 const RuleIndex)
{
	if (RuleArmed[RuleIndex])
	{
		RuleArmed[RuleIndex] = false;
		UGeoEncounterTrigger* const Trigger = Steps[CurrentStep].Rules[RuleIndex].Trigger;
		if (Trigger)
		{
			Trigger->OnDisarm();
		}
	}
}

bool AGeoEncounter::IsCompletionUnderway() const
{
	TArray<FGeoEncounterRule> const& Rules = Steps[CurrentStep].Rules;
	for (int32 RuleIndex = 0; RuleIndex < Rules.Num(); ++RuleIndex)
	{
		if (Rules[RuleIndex].bCompletesStep && RuleRuns[RuleIndex].bActive)
		{
			return true;
		}
	}

	return false;
}

void AGeoEncounter::OnTriggerFired(UGeoEncounterTrigger const& Trigger, FGeoEncounterPayload const& Payload)
{
	FCallScope const Scope(*this);
	int32 RuleIndex = INDEX_NONE;
	if (State == EState::Running && !Pending.IsSet() && !IsCompletionUnderway())
	{
		RuleIndex = Steps[CurrentStep].Rules.IndexOfByPredicate(
			[&Trigger](FGeoEncounterRule const& Rule)
			{
				return Rule.Trigger.Get() == &Trigger;
			});
	}

	if (RuleIndex != INDEX_NONE && RuleArmed[RuleIndex] && !RuleRuns[RuleIndex].bActive)
	{
		FGeoEncounterRule const& Rule = Steps[CurrentStep].Rules[RuleIndex];
		if (!Rule.Condition || Rule.Condition->Evaluate(Payload))
		{
			UE_LOG(LogGeoEncounter, Verbose, TEXT("%s: rule %d fired"), *GetName(), RuleIndex);
			if (!Rule.bRepeat)
			{
				DisarmRule(RuleIndex);
			}

			StartRun(RuleRuns[RuleIndex], RuleIndex, Rule.Actions, Payload);
		}
	}
}

void AGeoEncounter::OnActionFinished(UGeoEncounterAction const& Action)
{
	FCallScope const Scope(*this);
	FGeoEncounterActionRun* AwaitingRun = nullptr;
	auto const Check = [&Action, &AwaitingRun](FGeoEncounterActionRun& Run)
	{
		if (Run.bActive && Run.Awaiting.Get() == &Action)
		{
			AwaitingRun = &Run;
		}
	};
	Check(EnterRun);
	Check(ExitRun);
	for (FGeoEncounterActionRun& Run : RuleRuns)
	{
		Check(Run);
	}

	if (AwaitingRun)
	{
		AwaitingRun->Awaiting = nullptr;
		Advance(*AwaitingRun);
	}
	else
	{
		UE_LOG(LogGeoEncounter, Verbose, TEXT("%s: Finish from %s ignored, not awaited"), *GetName(),
			   *Action.GetName());
	}
}

void AGeoEncounter::StartRun(FGeoEncounterActionRun& Run, int32 const RuleIndex,
							 TArray<TObjectPtr<UGeoEncounterAction>> const& Actions,
							 FGeoEncounterPayload const& Payload)
{
	Run.RuleIndex = RuleIndex;
	Run.Actions = Actions;
	Run.Next = 0;
	Run.Payload = Payload;
	Run.Awaiting = nullptr;
	Run.bActive = true;
	Run.bAdvancing = false;
	Advance(Run);
}

void AGeoEncounter::Advance(FGeoEncounterActionRun& Run)
{
	if (!Run.bAdvancing)
	{
		Run.bAdvancing = true;
		while (Run.bActive && !Pending.IsSet() && !Run.Awaiting && Run.Next < Run.Actions.Num())
		{
			UGeoEncounterAction* const Action = Run.Actions[Run.Next++];
			if (Action)
			{
				Run.Awaiting = Action;
				Action->Begin(Run.Payload);
			}
			else
			{
				UE_LOG(LogGeoEncounter, Error, TEXT("%s: null action skipped"), *GetName());
			}
		}

		Run.bAdvancing = false;

		if (Run.bActive && !Pending.IsSet() && !Run.Awaiting && Run.Next >= Run.Actions.Num())
		{
			Run.bActive = false;
			OnRunFinished(Run);
		}
	}
}

void AGeoEncounter::CancelRun(FGeoEncounterActionRun& Run)
{
	if (Run.bActive)
	{
		Run.bActive = false;
		UGeoEncounterAction* const Action = Run.Awaiting;
		Run.Awaiting = nullptr;
		if (Action)
		{
			Action->OnCancel();
		}
	}
}

void AGeoEncounter::OnRunFinished(FGeoEncounterActionRun const& Run)
{
	if (&Run == &ExitRun)
	{
		EnterStep(ExitTarget);
	}
	else if (Run.RuleIndex != INDEX_NONE)
	{
		FGeoEncounterRule const& Rule = Steps[CurrentStep].Rules[Run.RuleIndex];
		if (Rule.bCompletesStep)
		{
			int32 const Target = Rule.StepAfter.IsNone() ? CurrentStep + 1 : FindStep(Rule.StepAfter);
			if (Target != INDEX_NONE)
			{
				RequestChange({.Target = Target, .bRunExit = true});
			}
			else
			{
				UE_LOG(LogGeoEncounter, Error, TEXT("%s: StepAfter %s does not exist"), *GetName(),
					   *Rule.StepAfter.ToString());
			}
		}
	}
}

#if WITH_EDITOR
EDataValidationResult AGeoEncounter::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	TSet<FName> SeenNames;
	for (FGeoEncounterStep const& Step : Steps)
	{
		if (Step.Name.IsNone() || SeenNames.Contains(Step.Name))
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("Step name \"%s\" is empty or used twice"), *Step.Name.ToString())));
		}

		SeenNames.Add(Step.Name);

		if (Step.OnEnter.Contains(nullptr) || Step.OnExit.Contains(nullptr))
		{
			Context.AddError(
				FText::FromString(FString::Printf(TEXT("Step %s has a null action"), *Step.Name.ToString())));
		}

		auto const HasActionAfterStart = [](TArray<TObjectPtr<UGeoEncounterAction>> const& Actions)
		{
			int32 const StartIndex = Actions.IndexOfByPredicate(
				[](UGeoEncounterAction const* const Action)
				{
					return Cast<UGeoStartEncounterAction>(Action) != nullptr;
				});
			return StartIndex != INDEX_NONE && StartIndex < Actions.Num() - 1;
		};
		bool bActionAfterStart = HasActionAfterStart(Step.OnEnter) || HasActionAfterStart(Step.OnExit);
		for (FGeoEncounterRule const& Rule : Step.Rules)
		{
			bActionAfterStart |= HasActionAfterStart(Rule.Actions);
		}

		if (bActionAfterStart)
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("Step %s: an action follows a start encounter action, which aborts this encounter first"),
				*Step.Name.ToString())));
		}

		for (FGeoEncounterRule const& Rule : Step.Rules)
		{
			UGeoCompareCondition const* const Compare = Cast<UGeoCompareCondition>(Rule.Condition);
			if (!Rule.Trigger || Rule.Actions.Contains(nullptr))
			{
				Context.AddError(FText::FromString(
					FString::Printf(TEXT("Step %s has a rule with a null trigger or action"), *Step.Name.ToString())));
			}

			if (Rule.Actions.IsEmpty() && !Rule.bCompletesStep)
			{
				Context.AddError(FText::FromString(
					FString::Printf(TEXT("Step %s has a rule that does nothing"), *Step.Name.ToString())));
			}

			if (!Rule.StepAfter.IsNone() && FindStep(Rule.StepAfter) == INDEX_NONE)
			{
				Context.AddError(FText::FromString(FString::Printf(TEXT("Step %s: StepAfter %s does not exist"),
																   *Step.Name.ToString(), *Rule.StepAfter.ToString())));
			}

			if (Compare && Compare->IsCounterNameMissing())
			{
				Context.AddError(FText::FromString(
					FString::Printf(TEXT("Step %s: a counter compare has no counter name"), *Step.Name.ToString())));
			}
		}
	}

	if (Context.GetNumErrors() > 0)
	{
		return EDataValidationResult::Invalid;
	}
	return EDataValidationResult::Valid;
}
#endif
