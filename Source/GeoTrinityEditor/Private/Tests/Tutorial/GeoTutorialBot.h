// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

struct FGameplayAbilitySpec;
class AEnemyCharacter;
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
	explicit FGeoRestoreGemProfileCommand(UGeoGemProfileSave* InProfile);

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

	explicit FGeoTutorialBot(FAutomationTestBase& InTest);
	virtual ~FGeoTutorialBot() override;

	virtual bool Update() override;

	/** Loads the tutorial map, starts the play session, runs Bot, ends the session and writes the gem profile back. The
	 * test returns right after; the work is latent. */
	static void QueueSession(FAutomationTestBase& Test, TSharedRef<FGeoTutorialBot> Bot);

	// --- Stages ---------------------------------------------------------------------------------------------------

	void AddStage(FString Name, FStageFunction Run, double TimeoutSeconds = GeoTutorial::DefaultStageTimeoutSeconds);
	/** Waits for the play world, its pawn and its HUD; subscribes to the Cat lines; the time runs at TimeDilation. When
	 * StartRoom is a room, jumps to it, else (FullRun) restarts the Landing room, which began before the player joined.
	 */
	void AddStartStage(int32 StartRoom);
	/** Waits for a Cat line holding Fragment, shown after the one the last line stage matched. */
	void AddLineStage(FString Name, FString Fragment);
	/** Waits Seconds of real time (the world may be paused or dilated). */
	void AddWaitStage(FString Name, double Seconds);
	/** Exactly Count living enemies tagged Tag exist: a spawn that never happens (or happens twice) stalls or fails. */
	void AddSpawnStage(int32 Room, FName Tag, int32 Count);
	/** The pad of Room opens (the proof its encounter completed). */
	void AddExitPadStage(int32 Room);
	/** The pawn steps on the open exit pad of Room and lands in the next room; its arrival closes the exit. */
	void AddTeleportStages(int32 Room);

	// --- The world ------------------------------------------------------------------------------------------------

	UWorld* GetWorld() const;
	AGeoPlayerController* GetController() const;
	APlayableCharacter* GetPawn() const;
	AGeoTeleporter* FindPad(FString const& Name) const;
	bool IsPadOpen(FString const& Name) const;
	AGeoEncounter* FindEncounter(int32 Room) const;
	/** The step counter of Room's encounter. */
	int32 GetCounter(int32 Room, FName Counter) const;
	AActor* FindByLabel(FString const& Label) const;
	/** The living enemies carrying Tag, as spawned by the room's spawn action. */
	TArray<AEnemyCharacter*> FindLiving(FName Tag) const;
	/** The active zone standing at Location, null when none. */
	AGeoEffectZone* FindZoneAt(FVector2D Location) const;
	void MovePawn(FVector2D Location) const;
	/** The player dies, once: the invincibility is off for the call only, so nothing else kills it meanwhile. */
	void KillPlayer() const;

	static float HealthRatio(AActor* Actor);
	/** Sets the health through its attribute: how the bot hurts or heals what the room does not. */
	static void SetHealthRatio(AActor* Actor, float Ratio);

	// --- Abilities ------------------------------------------------------------------------------------------------

	static FGameplayAbilitySpec const* FindSpec(AActor* Actor, FName AbilityTagName);
	bool IsGranted(FName AbilityTagName) const;
	static bool IsAbilityActive(AActor* Actor, FName AbilityTagName);
	/** The player uses the ability as the input does: its ASC with the aim target data. */
	bool ActivatePlayerAbility(FName AbilityTagName) const;
	/** Counts the activations of AbilityTagName on the ASC of what GetActor returns, followed when it is respawned.
	 * Returns the watch to read with GetWatchCount. */
	int32 WatchActivations(TFunction<AActor*()> GetActor, FName AbilityTagName);
	int32 GetWatchCount(int32 Watch) const { return Watches[Watch].Count; }

	void SetInvincible(bool bInvincible) const;

	// --- The Cat lines and the pages ------------------------------------------------------------------------------

	/** How many lines hold Fragment. */
	int32 CountLines(FString const& Fragment) const;
	/** Index of the first line holding Fragment from index From on, INDEX_NONE when none. */
	int32 FindLine(FString const& Fragment, int32 From = 0) const;
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

	void ServeUI();
	void CheckEntryPadsClosed();
	void KeepPlayerHealthy() const;
	void UpdateWatches();
	void Unbind(FWatch& Watch);
	/** The first Cat line is logged from the moment the world exists. */
	void SubscribeToLines();
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
	TWeakObjectPtr<UWorld> LineWorld;
	FDelegateHandle LineHandle;
};

#endif
