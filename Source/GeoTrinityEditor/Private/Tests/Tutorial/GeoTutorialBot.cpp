// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GeoTutorialBot.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Buttons/GeoTeleporter.h"
#include "Actor/Deployable/Zones/GeoEffectZone.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "Encounter/GeoEncounter.h"
#include "Encounter/GeoEncounterSubsystem.h"
#include "EngineUtils.h"
#include "GameClasses/Player/GeoPlayerController.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "Gem/GeoGemProfileSave.h"
#include "HUD/Interface/GeoHUDInterface.h"
#include "Tool/CatLine/GeoCatLineBank.h"
#include "UObject/StrongObjectPtr.h"
#include "World/GeoWorldSettings.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Editor.h"
#include "Editor/UnrealEdEngine.h"
#include "FileHelpers.h"
#include "ILevelEditor.h"
#include "LevelEditor.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "UnrealEdGlobals.h"

FString GeoTutorial::RoomName(int32 const Room)
{
	static TCHAR const* const Names[] = {TEXT("Landing"), TEXT("Dash"),	   TEXT("Shoot"), TEXT("Deploy"),
										 TEXT("Special"), TEXT("Dummies"), TEXT("Zones"), TEXT("Miniboss")};
	static_assert(UE_ARRAY_COUNT(Names) == RoomCount);
	return Names[Room];
}

FString GeoTutorial::ExitPadName(int32 const Room)
{
	return FString::Printf(TEXT("Tut_Pad_%02d_Exit"), Room);
}

FString GeoTutorial::EntryPadName(int32 const Room)
{
	return FString::Printf(TEXT("Tut_Pad_%02d_Entry"), Room);
}

// ---------------------------------------------------------------------------------------------------------------------
FGeoRestoreGemProfileCommand::FGeoRestoreGemProfileCommand(UGeoGemProfileSave* const InProfile) : Profile(InProfile)
{
}

bool FGeoRestoreGemProfileCommand::Update()
{
	Profile->Save(0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
bool FGeoStartListenPlayCommand::Update()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	ULevelEditorPlaySettings* const Settings =
		DuplicateObject(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
	Settings->SetPlayNetMode(EPlayNetMode::PIE_ListenServer);
	Settings->SetPlayNumberOfClients(1);
	Settings->SetRunUnderOneProcess(true);

	FRequestPlaySessionParams Params;
	Params.WorldType = EPlaySessionWorldType::PlayInEditor;
	Params.EditorPlaySettings = Settings;
	Params.DestinationSlateViewport = LevelEditorModule.GetFirstActiveViewport();
	GUnrealEd->RequestPlaySession(Params);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
FGeoTutorialBot::FGeoTutorialBot(FAutomationTestBase& InTest) : Test(InTest)
{
	WorldInitializedHandle = FWorldDelegates::OnPostWorldInitialization.AddLambda(
		[this](UWorld* const World, UWorld::InitializationValues const /*Values*/)
		{
			if (World->WorldType == EWorldType::PIE)
			{
				SubscribeToLines(World);
			}
		});
}

FGeoTutorialBot::~FGeoTutorialBot()
{
	FWorldDelegates::OnPostWorldInitialization.Remove(WorldInitializedHandle);
	for (FWatch& Watch : Watches)
	{
		Unbind(Watch);
	}

	if (UWorld* const World = LineWorld.Get())
	{
		World->GetSubsystem<UGeoEncounterSubsystem>()->OnCatLine.Remove(LineHandle);
	}
}

void FGeoTutorialBot::QueueSession(FAutomationTestBase& Test, TSharedRef<FGeoTutorialBot> Bot)
{
	// Headless -NullRHI spawns no Niagara component: the explosion cue's Set Niagara Variable reads None.
	Test.AddExpectedError(TEXT("On Execute Blueprint:  GC_Explosion"), EAutomationExpectedErrorFlags::Contains, -1);
	// The editor map only: AutomationOpenMap would also start a standalone play session, which the listen one below
	// then replaces after the first stage has run on it.
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/Tutorial"));
	// The map's editor quick-start room (a jump that replays the unlocks before it) would start the session past
	// Landing.
	if (AGeoWorldSettings* const WorldSettings =
			Cast<AGeoWorldSettings>(GEditor->GetEditorWorldContext().World()->GetWorldSettings()))
	{
		WorldSettings->StartingTutoRoomInEditor = NAME_None;
	}

	UGeoGemProfileSave* const ProfileBefore = UGeoGemProfileSave::Load(0);
	ADD_LATENT_AUTOMATION_COMMAND(FGeoStartListenPlayCommand());
	FAutomationTestFramework::Get().EnqueueLatentCommand(Bot);
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FGeoRestoreGemProfileCommand(ProfileBefore));
}

bool FGeoTutorialBot::Update()
{
	bool bFinished = false;
	double const Now = FPlatformTime::Seconds();
	if (!bStageEntered)
	{
		StageStartSeconds = Now;
	}

	if (!Stages.IsValidIndex(StageIndex))
	{
		bFinished = true;
	}
	else if (Now - StageStartSeconds > Stages[StageIndex].TimeoutSeconds)
	{
		Test.AddError(FString::Printf(TEXT("Stage '%s' timed out"), *Stages[StageIndex].Name));
		bFinished = true;
	}
	else
	{
		SubscribeToLines(GetWorld());
		ServeUI();
		CheckEntryPadsClosed();
		if (GetWorld())
		{
			UpdateWatches();
			KeepPlayerHealthy();
			for (TFunction<void()> const& Observer : Observers)
			{
				Observer();
			}
		}

		bool const bFirstTick = !bStageEntered;
		bStageEntered = true;
		if (Stages[StageIndex].Run(bFirstTick))
		{
			++StageIndex;
			bStageEntered = false;
		}
	}

	return bFinished;
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoTutorialBot::AddStage(FString Name, FStageFunction Run, double const TimeoutSeconds)
{
	Stages.Add({MoveTemp(Name), MoveTemp(Run), TimeoutSeconds});
}

void FGeoTutorialBot::AddStartStage(int32 const StartRoom)
{
	AddStage(TEXT("Play session starts"),
			 [this, StartRoom](bool)
			 {
				 bool const bReady = GetWorld() && GetPawn() && Cast<IGeoHUDInterface>(GetController()->GetHUD());
				 if (bReady)
				 {
					 Lines.Reset();
					 LineCursor = 0;
					 GetWorld()->GetWorldSettings()->SetTimeDilation(GeoTutorial::TimeDilation);
					 SetInvincible(true);
					 if (StartRoom == GeoTutorial::Landing)
					 {
						 FindEncounter(GeoTutorial::Landing)->Restart();
					 }
					 else if (StartRoom != INDEX_NONE)
					 {
						 JumpTo(StartRoom);
					 }
				 }

				 return bReady;
			 });
}

void FGeoTutorialBot::AddLineStage(FString Name, FString Fragment)
{
	AddStage(MoveTemp(Name),
			 [this, Fragment = MoveTemp(Fragment)](bool)
			 {
				 int32 const Index = FindLine(Fragment, LineCursor);
				 if (Index != INDEX_NONE)
				 {
					 LineCursor = Index + 1;
				 }

				 return Index != INDEX_NONE;
			 });
}

void FGeoTutorialBot::AddFileLineStage(FString Name, FGeoCatLineCategories Categories)
{
	AddStage(MoveTemp(Name),
			 [this, Categories = MoveTemp(Categories)](bool)
			 {
				 int32 Found = INDEX_NONE;
				 for (int32 Index = LineCursor; Found == INDEX_NONE && Index < Lines.Num(); ++Index)
				 {
					 if (GeoCatLines::HasLine(Categories, Lines[Index]))
					 {
						 Found = Index;
					 }
				 }

				 if (Found != INDEX_NONE)
				 {
					 LineCursor = Found + 1;
				 }

				 return Found != INDEX_NONE;
			 });
}

void FGeoTutorialBot::AddWaitStage(FString Name, double const Seconds)
{
	TSharedRef<double> Start = MakeShared<double>(0.);
	AddStage(MoveTemp(Name),
			 [Start, Seconds](bool const bFirstTick)
			 {
				 if (bFirstTick)
				 {
					 *Start = FPlatformTime::Seconds();
				 }

				 return FPlatformTime::Seconds() - *Start >= Seconds;
			 });
}

void FGeoTutorialBot::AddSpawnStage(int32 const Room, FName const Tag, int32 const Count)
{
	AddStage(FString::Printf(TEXT("Room %d: %d x %s spawn"), Room, Count, *Tag.ToString()),
			 [this, Tag, Count](bool)
			 {
				 int32 const Living = FindLiving(Tag).Num();
				 if (Living > Count)
				 {
					 Test.AddError(FString::Printf(TEXT("%d living %s, expected %d"), Living, *Tag.ToString(), Count));
				 }

				 return Living == Count;
			 });
}

void FGeoTutorialBot::AddExitPadStage(int32 const Room)
{
	AddStage(FString::Printf(TEXT("Room %d: exit pad opens"), Room),
			 [this, Room](bool)
			 {
				 return IsPadOpen(GeoTutorial::ExitPadName(Room));
			 });
}

void FGeoTutorialBot::AddTeleportStages(int32 const Room)
{
	AddStage(FString::Printf(TEXT("Room %d: teleport"), Room),
			 [this, Room](bool const bFirstTick)
			 {
				 if (bFirstTick)
				 {
					 GetPawn()->SetActorLocation(FindPad(GeoTutorial::ExitPadName(Room))->GetActorLocation(), false,
												 nullptr, ETeleportType::TeleportPhysics);
				 }

				 return GetPawn()->GetActorLocation().X > GeoTutorial::RoomCentreX(Room + 1) - GeoTutorial::RoomHalfX;
			 });
	AddStage(FString::Printf(TEXT("Room %d: arrival closes the exit"), Room),
			 [this, Room](bool)
			 {
				 return !IsPadOpen(GeoTutorial::ExitPadName(Room));
			 });
}

// ---------------------------------------------------------------------------------------------------------------------
UWorld* FGeoTutorialBot::GetWorld() const
{
	return GEditor->PlayWorld;
}

AGeoPlayerController* FGeoTutorialBot::GetController() const
{
	return GetWorld() ? Cast<AGeoPlayerController>(GetWorld()->GetFirstPlayerController()) : nullptr;
}

APlayableCharacter* FGeoTutorialBot::GetPawn() const
{
	AGeoPlayerController const* const Controller = GetController();
	return Controller ? Cast<APlayableCharacter>(Controller->GetPawn()) : nullptr;
}

AGeoTeleporter* FGeoTutorialBot::FindPad(FString const& Name) const
{
	for (TActorIterator<AGeoTeleporter> It(GetWorld()); It; ++It)
	{
		if (It->GetName() == Name)
		{
			return *It;
		}
	}

	return nullptr;
}

bool FGeoTutorialBot::IsPadOpen(FString const& Name) const
{
	AGeoTeleporter const* const Pad = FindPad(Name);
	return Pad && Pad->IsOpen();
}

AActor* FGeoTutorialBot::FindByLabel(FString const& Label) const
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetActorLabel() == Label)
		{
			return *It;
		}
	}

	return nullptr;
}

AGeoEncounter* FGeoTutorialBot::FindEncounter(int32 const Room) const
{
	return Cast<AGeoEncounter>(FindByLabel(FString::Printf(TEXT("Tut_%s_Encounter"), *GeoTutorial::RoomName(Room))));
}

int32 FGeoTutorialBot::GetCounter(int32 const Room, FName const Counter) const
{
	return FindEncounter(Room)->GetCounter(Counter);
}

TArray<AEnemyCharacter*> FGeoTutorialBot::FindLiving(FName const Tag) const
{
	TArray<AEnemyCharacter*> Living;
	for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(Tag) && !It->IsDead() && IsValid(*It))
		{
			Living.Add(*It);
		}
	}

	return Living;
}

AGeoEffectZone* FGeoTutorialBot::FindZoneAt(FVector2D const Location) const
{
	for (TActorIterator<AGeoEffectZone> It(GetWorld()); It; ++It)
	{
		if (It->IsActive() && FVector2D::Distance(FVector2D(It->GetActorLocation()), Location) < 100.f)
		{
			return *It;
		}
	}

	return nullptr;
}

void FGeoTutorialBot::MovePawn(FVector2D const Location) const
{
	GetPawn()->SetActorLocation(FVector(Location, GeoTutorial::PawnZ), false, nullptr, ETeleportType::TeleportPhysics);
}

void FGeoTutorialBot::KillPlayer() const
{
	SetInvincible(false);
	GetPawn()->Death();
	SetInvincible(true);
}

float FGeoTutorialBot::HealthRatio(AActor* const Actor)
{
	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Actor);
	return ASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute())
		/ ASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute());
}

void FGeoTutorialBot::SetHealthRatio(AActor* const Actor, float const Ratio)
{
	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Actor);
	ASC->SetNumericAttributeBase(UGeoAttributeSetBase::GetHealthAttribute(),
								 ASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute()) * Ratio);
}

// ---------------------------------------------------------------------------------------------------------------------
FGameplayAbilitySpec const* FGeoTutorialBot::FindSpec(AActor* const Actor, FName const AbilityTagName)
{
	UGeoAbilitySystemComponent const* const ASC = GeoASLib::GetGeoAscFromActor(Actor);
	FGameplayTag const AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName);
	return ASC ? ASC->GetActivatableAbilities().FindByPredicate(
					 [AbilityTag](FGameplayAbilitySpec const& Spec)
					 {
						 return GeoASLib::GetAbilityTagFromSpec(Spec) == AbilityTag;
					 })
			   : nullptr;
}

bool FGeoTutorialBot::IsGranted(FName const AbilityTagName) const
{
	return FindSpec(GetPawn(), AbilityTagName) != nullptr;
}

bool FGeoTutorialBot::IsAbilityActive(AActor* const Actor, FName const AbilityTagName)
{
	FGameplayAbilitySpec const* const Spec = FindSpec(Actor, AbilityTagName);
	return Spec && Spec->IsActive();
}

bool FGeoTutorialBot::ActivatePlayerAbility(FName const AbilityTagName) const
{
	FGameplayAbilitySpec const* const Spec = FindSpec(GetPawn(), AbilityTagName);
	bool bActivated = false;
	if (Test.TestNotNull(*FString::Printf(TEXT("%s is granted"), *AbilityTagName.ToString()), Spec))
	{
		bActivated = GeoASLib::GetGeoAscFromActor(GetPawn())->TryActivateAbilityWithTargetData(
			Spec->Handle, FGameplayTag::RequestGameplayTag(AbilityTagName));
		Test.TestTrue(*FString::Printf(TEXT("%s activates"), *AbilityTagName.ToString()), bActivated);
	}

	return bActivated;
}

int32 FGeoTutorialBot::WatchActivations(TFunction<AActor*()> GetActor, FName const AbilityTagName)
{
	Watches.Add({MoveTemp(GetActor), AbilityTagName});
	return Watches.Num() - 1;
}

void FGeoTutorialBot::UpdateWatches()
{
	for (int32 Index = 0; Index < Watches.Num(); ++Index)
	{
		FWatch& Watch = Watches[Index];
		AActor* const Actor = Watch.GetActor();
		UGeoAbilitySystemComponent* const ASC = Actor ? GeoASLib::GetGeoAscFromActor(Actor) : nullptr;
		if (ASC != Watch.BoundASC.Get())
		{
			Unbind(Watch);
			if (ASC)
			{
				FGameplayTag const AbilityTag = FGameplayTag::RequestGameplayTag(Watch.AbilityTagName);
				Watch.BoundASC = ASC;
				Watch.Handle = ASC->AbilityActivatedCallbacks.AddLambda(
					[this, Index, AbilityTag](UGameplayAbility* const Ability)
					{
						if (Ability && Ability->GetAssetTags().HasTagExact(AbilityTag))
						{
							++Watches[Index].Count;
						}
					});
			}
		}
	}
}

void FGeoTutorialBot::Unbind(FWatch& Watch)
{
	if (UGeoAbilitySystemComponent* const ASC = Watch.BoundASC.Get())
	{
		ASC->AbilityActivatedCallbacks.Remove(Watch.Handle);
	}

	Watch.BoundASC.Reset();
	Watch.Handle.Reset();
}

void FGeoTutorialBot::SetInvincible(bool const bInvincible) const
{
	GEngine->Exec(GetWorld(), bInvincible ? TEXT("Geo.PlayerInvincible 1") : TEXT("Geo.PlayerInvincible 0"));
}

// ---------------------------------------------------------------------------------------------------------------------
int32 FGeoTutorialBot::CountLines(FString const& Fragment) const
{
	int32 Count = 0;
	for (FString const& Line : Lines)
	{
		Count += Line.Contains(Fragment) ? 1 : 0;
	}

	return Count;
}

int32 FGeoTutorialBot::FindLine(FString const& Fragment, int32 const From) const
{
	for (int32 Index = From; Index < Lines.Num(); ++Index)
	{
		if (Lines[Index].Contains(Fragment))
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

void FGeoTutorialBot::ExpectNoLine(FString const& Fragment) const
{
	Test.TestFalse(*FString::Printf(TEXT("No Cat line holds '%s'"), *Fragment), SawLine(Fragment));
}

void FGeoTutorialBot::SubscribeToLines(UWorld* const World)
{
	UGeoEncounterSubsystem* const Encounters = World ? World->GetSubsystem<UGeoEncounterSubsystem>() : nullptr;
	if (Encounters && World != LineWorld.Get())
	{
		if (UWorld* const Previous = LineWorld.Get())
		{
			Previous->GetSubsystem<UGeoEncounterSubsystem>()->OnCatLine.Remove(LineHandle);
		}

		LineWorld = World;
		LineHandle = Encounters->OnCatLine.AddLambda(
			[this](FText const& Line)
			{
				Lines.Add(Line.ToString());
			});
	}
}

void FGeoTutorialBot::ServeUI()
{
	AGeoPlayerController* const Controller = GetController();
	if (Controller && Controller->IsPauseMenuOpen() && !bHoldPauseMenu)
	{
		Pages.Add({CountGrantedAbilities(), GetWorld()->IsPaused()});
		Controller->ClosePauseMenu();
	}

	if (Controller && (Lines.IsEmpty() || KeepLineFragment.IsEmpty() || !Lines.Last().Contains(KeepLineFragment)))
	{
		if (IGeoHUDInterface* const HUD = Cast<IGeoHUDInterface>(Controller->GetHUD()))
		{
			HUD->HideCatLine();
		}
	}
}

void FGeoTutorialBot::KeepPlayerHealthy() const
{
	APlayableCharacter* const Pawn = bKeepPlayerHealthy ? GetPawn() : nullptr;
	if (Pawn && !Pawn->IsDead())
	{
		SetHealthRatio(Pawn, 1.f);
	}
}

void FGeoTutorialBot::CheckEntryPadsClosed()
{
	if (GetWorld() && !bEntryPadWasOpen)
	{
		for (int32 Room = GeoTutorial::Dash; Room < GeoTutorial::RoomCount; ++Room)
		{
			if (IsPadOpen(GeoTutorial::EntryPadName(Room)))
			{
				bEntryPadWasOpen = true;
				Test.AddError(
					FString::Printf(TEXT("%s opened: an entry pad never opens"), *GeoTutorial::EntryPadName(Room)));
			}
		}
	}
}

int32 FGeoTutorialBot::CountGrantedAbilities() const
{
	int32 Granted = 0;
	for (FName const AbilityTagName : {FName("Ability.Spell.Dash"), FName("Ability.Spell.BasicBeam"),
									   FName("Ability.Spell.HealingZone"), FName("Ability.Spell.MoiraBeam")})
	{
		Granted += IsGranted(AbilityTagName) ? 1 : 0;
	}

	return Granted;
}

void FGeoTutorialBot::JumpTo(int32 const Room) const
{
	APlayerStart const* Start = nullptr;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->PlayerStartTag == FName(*GeoTutorial::RoomName(Room)))
		{
			Start = *It;
		}
	}

	AGeoEncounter* const Encounter = FindEncounter(Room);
	if (Test.TestNotNull(*FString::Printf(TEXT("%s has a player start"), *GeoTutorial::RoomName(Room)), Start)
		&& Test.TestNotNull(*FString::Printf(TEXT("%s has an encounter"), *GeoTutorial::RoomName(Room)), Encounter))
	{
		GetPawn()->SetActorLocation(Start->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		Encounter->StartAfterSkippingChain();
	}
}

#endif
