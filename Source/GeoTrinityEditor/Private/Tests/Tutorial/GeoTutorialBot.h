// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Misc/AutomationTest.h"
#include "Tool/CatLine/GeoCatLineCategories.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

struct FGameplayAbilitySpec;
struct FGameplayEffectSpec;
struct FOnAttributeChangeData;
class AEnemyCharacter;
class AGeoCharacter;
class AGeoEffectZone;
class AGeoEncounter;
class AGeoPlayerController;
class AGeoTeleporter;
class APlayableCharacter;
class UGeoAbilitySystemComponent;
class UGeoGemProfileSave;

constexpr EAutomationTestFlags GeoTutorialRunFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

namespace GeoTutorial
{
	/** The rooms along +X, in the order of tutorial_encounters.py ROOMS. */
	enum ERoom : int32
	{
		Landing,
		Dash,
		Shoot,
		Deploy,
		Special,
		Dummies,
		Zones,
		Miniboss,
		RoomCount
	};

	constexpr float RoomPitch = 2900.f;
	constexpr float RoomHalfX = 700.f;
	constexpr float TimeDilation = 8.f;
	constexpr double DefaultStageTimeoutSeconds = 90.;
	constexpr float PawnZ = 60.f;
	/** Health log lines a failure carries (FGeoTutorialBot::GetHealthLog). */
	constexpr int32 HealthLogTailCount = 12;

	inline float RoomCentreX(int32 const Room)
	{
		return Room * RoomPitch;
	}

	FString RoomName(int32 Room);
	FString ExitPadName(int32 Room);
	FString EntryPadName(int32 Room);
} // namespace GeoTutorial

/** Writes back the gem profile read before the run: the Miniboss fight grants XP and loot to the local player's
 * profile, which is the user's own file. */
class FGeoRestoreGemProfileCommand : public IAutomationLatentCommand
{
public:
	/** Captures InProfile to write back to disk when Update runs. */
	explicit FGeoRestoreGemProfileCommand(UGeoGemProfileSave* InProfile);

	/** Writes the held profile back once the PIE session has ended; returns true when done. */
	virtual bool Update() override;

private:
	TStrongObjectPtr<UGeoGemProfileSave> Profile;
};

/** Starts a listen-server play session on the map loaded in the editor, in the first level viewport. */
DEFINE_LATENT_AUTOMATION_COMMAND(FGeoStartListenPlayCommand);

/**
 * The player of the tutorial: a list of stages run one per frame in the play-in-editor world until each says it is
 * done. A room's stages are added by GeoTutorialRoomStages.h. Every frame it closes the pause menus (noting which
 * opened), ends the Cat lines (as a player mashing the skip key would), fails if an entry pad opens, and follows the
 * watched abilities. The Cat lines shown are logged in order (UGeoEncounterSubsystem::OnCatLine): a branch is proved by
 * its line.
 */
class FGeoTutorialBot : public IAutomationLatentCommand
{
public:
	/** A pause menu the bot closed: how many class abilities the player held when it opened, whether the world paused.
	 */
	struct FPage
	{
		int32 GrantedAbilities = 0;
		bool bWorldPaused = false;
	};

	using FStageFunction = TFunction<bool(bool /*bFirstTick*/)>;

	/** Binds InTest as the failure and log target for all stages. */
	explicit FGeoTutorialBot(FAutomationTestBase& InTest);
	/** Unbinds all delegates so no stage fires after the command is destroyed. */
	virtual ~FGeoTutorialBot() override;

	/** Runs the current stage one tick; returns true when every stage has completed or one timed out. */
	virtual bool Update() override;

	/** Loads the tutorial map, starts the play session, runs Bot, ends the session and writes the gem profile back. The
	 * test returns right after; the work is latent. */
	static void QueueSession(FAutomationTestBase& Test, TSharedRef<FGeoTutorialBot> Bot);

	// --- Stages ---------------------------------------------------------------------------------------------------

	/** Appends a stage: Run is called every frame until it returns true or TimeoutSeconds of real time elapse. */
	void AddStage(FString Name, FStageFunction Run, double TimeoutSeconds = GeoTutorial::DefaultStageTimeoutSeconds);
	/** Waits for the play world, its pawn and its HUD; subscribes to the Cat lines; the time runs at TimeDilation. When
	 * StartRoom is a room, jumps to it, else (FullRun) restarts the Landing room, which began before the player joined.
	 */
	void AddStartStage(int32 StartRoom);
	/** Waits for a Cat line holding Fragment, shown after the one the last line stage matched. */
	void AddLineStage(FString Name, FString Fragment);
	/** Waits for a Cat line that is one of the lines of Categories (Data/CatLines.txt), shown after the one the last
	 * line stage matched. */
	void AddFileLineStage(FString Name, FGeoCatLineCategories Categories);
	/** Waits Seconds of real time (the world may be paused or dilated). */
	void AddWaitStage(FString Name, double Seconds);
	/** Exactly Count living enemies tagged Tag exist: a spawn that never happens (or happens twice) stalls or fails. */
	void AddSpawnStage(int32 Room, FName Tag, int32 Count);
	/** The pad of Room opens (the proof its encounter completed). */
	void AddExitPadStage(int32 Room);
	/** The pawn steps on the open exit pad of Room and lands in the next room; its arrival closes the exit. */
	void AddTeleportStages(int32 Room);

	// --- The world ------------------------------------------------------------------------------------------------

	/** The play-in-editor world, null before the session starts. */
	UWorld* GetWorld() const;
	/** The first local player controller in the play world. */
	AGeoPlayerController* GetController() const;
	/** The local playable character in the play world. */
	APlayableCharacter* GetPawn() const;
	/** The AGeoTeleporter labeled Name in the play world, null when absent. */
	AGeoTeleporter* FindPad(FString const& Name) const;
	/** True when the pad labeled Name is currently open. */
	bool IsPadOpen(FString const& Name) const;
	/** The AGeoEncounter for Room in the play world, null when absent. */
	AGeoEncounter* FindEncounter(int32 Room) const;
	/** The step counter of Room's encounter. */
	int32 GetCounter(int32 Room, FName Counter) const;
	/** The actor whose editor label is Label in the play world, null when absent. */
	AActor* FindByLabel(FString const& Label) const;
	/** The living enemies carrying Tag, as spawned by the room's spawn action. */
	TArray<AEnemyCharacter*> FindLiving(FName Tag) const;
	/** The active zone standing at Location, null when none. */
	AGeoEffectZone* FindZoneAt(FVector2D Location) const;
	/** Teleports the pawn to (Location.X, Location.Y, PawnZ). */
	void MovePawn(FVector2D Location) const;
	/** The player dies, once: the invincibility is off for the call only, so nothing else kills it meanwhile. */
	void KillPlayer() const;

	/** Current health divided by max health for Actor; reads the GAS attribute. */
	static float HealthRatio(AActor* Actor);
	/** Sets the health through its attribute: how the bot hurts or heals what the room does not. */
	static void SetHealthRatio(AActor* Actor, float Ratio);

	// --- Abilities ------------------------------------------------------------------------------------------------

	/** The granted spec on Actor whose ability tag matches AbilityTagName, null when absent. */
	static FGameplayAbilitySpec const* FindSpec(AActor* Actor, FName AbilityTagName);
	/** True when the player pawn holds a granted spec tagged AbilityTagName. */
	bool IsGranted(FName AbilityTagName) const;
	/** True when the ability tagged AbilityTagName is currently active on Actor. */
	static bool IsAbilityActive(AActor* Actor, FName AbilityTagName);
	/** The player uses the ability as the input does: its ASC with the aim target data. */
	bool ActivatePlayerAbility(FName AbilityTagName) const;
	/** Counts the activations of AbilityTagName on the ASC of what GetActor returns, followed when it is respawned.
	 * Returns the watch to read with GetWatchCount. */
	int32 WatchActivations(TFunction<AActor*()> GetActor, FName AbilityTagName);
	/** Activations counted so far for the watch index returned by WatchActivations. */
	int32 GetWatchCount(int32 Watch) const { return Watches[Watch].Count; }

	/** Turns the player pawn's invincibility on (true) or off (false). */
	void SetInvincible(bool bInvincible) const;

	// --- Health log -----------------------------------------------------------------------------------------------

	/**
	 * From the next frame on, logs every change of health value of each enemy tagged Tag and the change that kills the
	 * player's pawn (its health is refilled every frame), with its cause: effect, instigator, causer, ability, or set
	 * directly (the bot, or code outside an effect). Each effect applied or executed on those enemies has its own line
	 * at the same time: damage through IncomingDamage reaches Health without its effect's data. Also each death,
	 * revive, and removal while alive (an arena wipe).
	 * Written to run.log as it happens and kept for GetHealthLog; a stage timeout carries its tail.
	 */
	void WatchHealth(FName Tag);
	/** The last Count lines of the health log, oldest first, one per line; empty when nothing was logged. */
	FString GetHealthLog(int32 Count) const;

	// --- The Cat lines and the pages ------------------------------------------------------------------------------

	/** How many lines hold Fragment. */
	int32 CountLines(FString const& Fragment) const;
	/** Index of the first line holding Fragment from index From on, INDEX_NONE when none. */
	int32 FindLine(FString const& Fragment, int32 From = 0) const;
	/** True when any logged Cat line contains Fragment. */
	bool SawLine(FString const& Fragment) const { return FindLine(Fragment) != INDEX_NONE; }
	/** Fails Test when a line holding Fragment was shown: the branch it belongs to did not run. */
	void ExpectNoLine(FString const& Fragment) const;

	FAutomationTestBase& Test;
	TArray<FString> Lines;
	TArray<FPage> Pages;
	/** True: the pause menu stays open (a test holds it); false: the bot closes it the frame it opens. */
	bool bHoldPauseMenu = false;
	/** A shown line holding this stays until it ends by itself; every other line is ended by the bot the frame it
	 * shows. */
	FString KeepLineFragment;
	/** True: the player's health is refilled every frame, so a stray bullet never kills it; only what the test asks
	 * for. */
	bool bKeepPlayerHealthy = false;
	/** Called every frame, before the stage: what a test watches whatever the stage. */
	TArray<TFunction<void()>> Observers;

private:
	struct FStage
	{
		FString Name;
		FStageFunction Run;
		double TimeoutSeconds = 0.;
	};

	struct FWatch
	{
		TFunction<AActor*()> GetActor;
		FName AbilityTagName;
		TWeakObjectPtr<UGeoAbilitySystemComponent> BoundASC;
		FDelegateHandle Handle;
		int32 Count = 0;
	};

	struct FHealthWatch
	{
		TWeakObjectPtr<AGeoCharacter> Character;
		TWeakObjectPtr<UGeoAbilitySystemComponent> BoundASC;
		FDelegateHandle Handle;
		/** Effects applied and periodic effects executed on it: bEveryChange only. */
		FDelegateHandle AppliedHandle;
		FDelegateHandle PeriodicHandle;
		FString Name;
		/** False: only the change that empties its health is logged. */
		bool bEveryChange = true;
		bool bDead = false;
		/** Destroyed: unbound, nothing more to log. */
		bool bGone = false;
		float Health = 0.f;
		/** Where it last stood alive. */
		FVector Location = FVector::ZeroVector;
		/** Last logged movement mode (bEveryChange only), so a change to or from Walking is logged once. */
		int32 MovementMode = -1;
	};

	void ServeUI();
	void CheckEntryPadsClosed();
	void KeepPlayerHealthy() const;
	void UpdateWatches();
	void Unbind(FWatch& Watch);
	/** Logs the deaths, revives and removals of the watched characters, then binds the newly spawned ones. */
	void UpdateHealthWatches();
	void BindHealth(AGeoCharacter* Character, bool bEveryChange);
	void UnbindHealth(FHealthWatch& Watch);
	/** Writes Line to run.log and the health log, stamped with the play world's time. */
	void AddHealthLog(FString const& Line);
	/** DescribeEffect of the effect behind Data, or that it carries none. */
	static FString DescribeHealthChange(FOnAttributeChangeData const& Data);
	/** Spec's effect, instigator, causer and ability. */
	static FString DescribeEffect(FGameplayEffectSpec const& Spec);
	/** Logs the Cat lines of World, the play world, and stops logging those of the one before. */
	void SubscribeToLines(UWorld* World);
	/** The world-URL jump without the travel: the pawn onto Room's player start, then its encounter's arrival reset. */
	void JumpTo(int32 Room) const;
	int32 CountGrantedAbilities() const;

	TArray<FStage> Stages;
	int32 StageIndex = 0;
	bool bStageEntered = false;
	double StageStartSeconds = 0.;
	bool bEntryPadWasOpen = false;
	int32 LineCursor = 0;
	TArray<FWatch> Watches;
	TArray<FName> HealthTags;
	TArray<FHealthWatch> HealthWatches;
	TArray<FString> HealthLog;
	TWeakObjectPtr<UWorld> LineWorld;
	FDelegateHandle LineHandle;
	/** Subscribes a play world as soon as it exists: a travel's first line shows before the next frame. */
	FDelegateHandle WorldInitializedHandle;
};

#endif
