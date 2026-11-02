// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Gem/GeoCorePassiveAbility.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/AttributeSet/GeoGemAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Actor/Arena/GeoArena.h"
#include "Actor/GeoGemDrop.h"
#include "Algo/Count.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Characters/PlayableCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Gem/GeoGemCatalog.h"
#include "Gem/GeoGemMeshes.h"
#include "Gem/GeoGemProfileSave.h"
#include "Gem/GeoGemStatsEffect.h"
#include "Misc/AutomationTest.h"
#include "Tests/Gameplay/GeoTestFlags.h"
#include "Settings/GameDataSettings.h"
#include "Tests/Gameplay/Fake/GeoTestDeployable.h"
#include "Tests/Gameplay/Fake/GeoTestGemDrop.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"
#include "Tests/Gameplay/Gem/GeoGemTestHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemEchoTest, "GeoTrinity.Gems.Echo", GeoGemTestFlags)

bool FGeoGemEchoTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();
	UGeoGemProfileSave* Profile = NewObject<UGeoGemProfileSave>();
	EPlayerClass const PlayerClass = EPlayerClass::Triangle;
	Profile->AddGems("Power", 2);
	Profile->AddGems("Guard", 1);
	Profile->AddGems("Surplus", 1);

	TestEqual(TEXT("Echo level starts at 0"), Profile->GetEchoLevel(), 0);
	TestFalse(TEXT("Echo locked while no class reached 20"), Profile->CanEcho(Catalog, PlayerClass, "Power"));
	Profile->SetClassLevel(PlayerClass, GeoGem::EchoUnlockLevel);
	TestEqual(TEXT("The first class at 20 takes the echo to 1"), Profile->GetEchoLevel(), 1);
	TestTrue(TEXT("Echo open at level 20"), Profile->CanEcho(Catalog, PlayerClass, "Power"));
	TestTrue(TEXT("Echo open on a class at level 1"), Profile->CanEcho(Catalog, EPlayerClass::Circle, "Power"));
	Profile->SetClassLevel(EPlayerClass::Circle, GeoGem::EchoUnlockLevel);
	TestEqual(TEXT("A second class at 20 keeps it"), Profile->GetEchoLevel(), 1);
	Profile->SetClassLevel(EPlayerClass::Circle, 1);
	TestFalse(TEXT("A Core is refused"), Profile->CanEcho(Catalog, PlayerClass, "Surplus"));
	TestFalse(TEXT("A gem with no copy is refused"), Profile->CanEcho(Catalog, PlayerClass, "Vigor"));
	TestFalse(TEXT("An unknown gem is refused"), Profile->CanEcho(Catalog, PlayerClass, "Nothing"));
	TestEqual(TEXT("A Core says why"), Profile->GetEchoRefusal(Catalog, PlayerClass, "Surplus"),
			  EGeoGemRefusal::EchoCore);
	TestEqual(TEXT("No copy says why"), Profile->GetEchoRefusal(Catalog, PlayerClass, "Vigor"), EGeoGemRefusal::NoCopy);
	TestEqual(TEXT("An open echo takes it"), Profile->GetEchoRefusal(Catalog, PlayerClass, "Power"),
			  EGeoGemRefusal::None);
	Profile->SetClassLevel(PlayerClass, 1);
	TestEqual(TEXT("A locked echo says why"), Profile->GetEchoRefusal(Catalog, PlayerClass, "Power"),
			  EGeoGemRefusal::EchoLocked);
	Profile->SetClassLevel(PlayerClass, GeoGem::EchoUnlockLevel);

	TestEqual(TEXT("Both copies slotted"), Profile->FillSockets(Catalog, PlayerClass, "Power"), 2);
	TestTrue(TEXT("Echoing a slotted type"), Profile->SetEcho(Catalog, PlayerClass, "Power"));
	TestEqual(TEXT("The echoed gem"), Profile->GetEcho(PlayerClass), FName("Power"));
	TestEqual(TEXT("Echoing empties the build's sockets of that type"),
			  static_cast<int32>(Algo::Count(Profile->GetLoadout(PlayerClass).Sockets, FName("Power"))), 0);
	TestEqual(TEXT("The echo takes one copy"), Profile->GetFreeCount("Power", PlayerClass), 1);
	TestEqual(TEXT("The echo reserves a copy from the Forge"), Profile->GetBreakableCount("Power"), 1);
	TestFalse(TEXT("Echoing the echoed gem again"), Profile->SetEcho(Catalog, PlayerClass, "Power"));
	TestEqual(TEXT("Echoing the echoed gem says why"), Profile->GetEchoRefusal(Catalog, PlayerClass, "Power"),
			  EGeoGemRefusal::AlreadyEcho);
	TestTrue(TEXT("Slotting the echoed type"),
			 Profile->Equip(Catalog, PlayerClass, FindSocket(EGeoGemTier::Chip, 1), "Power"));
	TestTrue(TEXT("Slotting it empties the echo"), Profile->GetEcho(PlayerClass).IsNone());
	TestEqual(TEXT("The echo's copy moved"), Profile->GetFreeCount("Power", PlayerClass), 1);
	TestTrue(TEXT("Echoing it again empties the socket"), Profile->SetEcho(Catalog, PlayerClass, "Power"));
	TestEqual(TEXT("Fill of an echoed type clears the echo, then fills"),
			  Profile->FillSockets(Catalog, PlayerClass, "Power"), 2);
	TestTrue(TEXT("Fill emptied the echo"), Profile->GetEcho(PlayerClass).IsNone());
	TestTrue(TEXT("Echo a slotted type, one copy owned"), Profile->SetEcho(Catalog, PlayerClass, "Power"));
	TestEqual(TEXT("Echoed copies count once"), Profile->GetFreeCount("Power", PlayerClass), 1);

	TestTrue(TEXT("Another gem replaces the echo"), Profile->SetEcho(Catalog, PlayerClass, "Guard"));
	TestEqual(TEXT("The replaced gem is free again"), Profile->GetFreeCount("Power", PlayerClass), 2);
	Profile->ClearEcho(PlayerClass);
	TestTrue(TEXT("Cleared echo"), Profile->GetEcho(PlayerClass).IsNone());
	TestEqual(TEXT("Cleared echo gives its copy back"), Profile->GetFreeCount("Guard", PlayerClass), 1);

	Profile->SetEcho(Catalog, PlayerClass, "Power");
	TestEqual(TEXT("A class under 20 keeps its XP"), Profile->AddClassXp(Catalog, EPlayerClass::Circle, 10), 0);
	TestEqual(TEXT("Its XP goes to the class"), Profile->GetClassXp(EPlayerClass::Circle), 10);
	TestEqual(TEXT("Not to the echo"), Profile->GetEchoXp(), 0);
	TestEqual(TEXT("Max class level: XP raises the echo level"), Profile->AddClassXp(Catalog, PlayerClass, 3500), 0);
	TestEqual(TEXT("Echo level 4"), Profile->GetEchoLevel(), 4);
	TestEqual(TEXT("Echo XP towards the next"), Profile->GetEchoXp(), 500);
	int32 XpTaken = 0;
	int32 XpNeeded = 0;
	float const Fill = Profile->GetXpProgress(Catalog, PlayerClass, XpTaken, XpNeeded);
	TestEqual(TEXT("The bar carries on with the echo XP"), XpTaken, 500);
	TestEqual(TEXT("The bar needs the next echo level"), XpNeeded, Catalog.GetEchoXpForLevel(5));
	TestEqual(TEXT("Bar fill is echo XP over its cost"), Fill, 500.f / XpNeeded, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("The class level stays"), Profile->GetClassLevel(PlayerClass), GeoGem::MaxClassLevel);
	TestEqual(TEXT("The loadout carries the echo"), Profile->GetLoadout(PlayerClass).Echo, FName("Power"));
	TestEqual(TEXT("The loadout carries the echo level"), Profile->GetLoadout(PlayerClass).EchoLevel, 4);
	TestEqual(TEXT("A class under 20 sees the shared level"), Profile->GetLoadout(EPlayerClass::Circle).EchoLevel, 4);

	Profile->SetClassLevel(PlayerClass, GeoGem::EchoUnlockLevel - 1);
	TestTrue(TEXT("No class at 20 empties the echo"), Profile->GetEcho(PlayerClass).IsNone());
	TestEqual(TEXT("No class at 20 resets the echo level"), Profile->GetEchoLevel(), 0);
	TestEqual(TEXT("And its XP"), Profile->GetEchoXp(), 0);
	TestFalse(TEXT("Echo locked again"), Profile->CanEcho(Catalog, PlayerClass, "Power"));

	Profile->SetClassLevel(PlayerClass, GeoGem::EchoUnlockLevel);
	Profile->AddClassXp(Catalog, PlayerClass, 1000000);
	TestEqual(TEXT("Echo level stops at the max"), Profile->GetEchoLevel(), Catalog.EchoMaxLevel);
	int32 const KeptXp = Profile->GetEchoXp();
	Profile->AddClassXp(Catalog, PlayerClass, 123);
	TestEqual(TEXT("Every echo XP point is kept"), Profile->GetEchoXp(), KeptXp + 123);

	Profile->SetClassLevel(PlayerClass, 1);
	Profile->SetEchoLevel(Catalog, 5);
	TestEqual(TEXT("An echo level needs a class at 20"), Profile->GetClassLevel(EPlayerClass::Triangle),
			  GeoGem::EchoUnlockLevel);
	TestEqual(TEXT("The echo level is set"), Profile->GetEchoLevel(), 5);

	Profile->SetEchoLevel(Catalog, 1000);
	TestEqual(TEXT("Echo level clamps to the max"), Profile->GetEchoLevel(), Catalog.EchoMaxLevel);
	Profile->SetEchoLevel(Catalog, -5);
	TestEqual(TEXT("Echo level clamps to 0"), Profile->GetEchoLevel(), 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemEchoStatsTest, "GeoTrinity.Gems.EchoStats", GeoGemTestFlags)

bool FGeoGemEchoStatsTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();
	TestEqual(TEXT("Level 0: no echo share"), Catalog.GetEchoShare(EGeoGemTier::Chip, 0), 0.f);
	TestEqual(TEXT("Level 1: Chip 1%"), Catalog.GetEchoShare(EGeoGemTier::Chip, 1), 0.01f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Level 1: no Cut"), Catalog.GetEchoShare(EGeoGemTier::Cut, 1), 0.f);
	TestEqual(TEXT("Level 1: no Prism"), Catalog.GetEchoShare(EGeoGemTier::Prism, 1), 0.f);
	TestEqual(TEXT("Level 2 adds Cut"), Catalog.GetEchoShare(EGeoGemTier::Cut, 2), 0.01f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Level 3 adds Prism"), Catalog.GetEchoShare(EGeoGemTier::Prism, 3), 0.01f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Level 4: Chip 2%"), Catalog.GetEchoShare(EGeoGemTier::Chip, 4), 0.02f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Level 100: Chip 34%"), Catalog.GetEchoShare(EGeoGemTier::Chip, 100), 0.34f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Level 100: Cut 33%"), Catalog.GetEchoShare(EGeoGemTier::Cut, 100), 0.33f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Level 100: Prism 33%"), Catalog.GetEchoShare(EGeoGemTier::Prism, 100), 0.33f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("A Core has no share"), Catalog.GetEchoShare(EGeoGemTier::Core, 100), 0.f);
	TestEqual(TEXT("Echo XP from the curve"), Catalog.GetEchoXpForLevel(2), 1000);
	TestEqual(TEXT("Level 1: a Chip echo is worth 30 sockets x 1%"), Catalog.GetEchoCopies(EGeoGemTier::Chip, 1), 0.3f,
			  KINDA_SMALL_NUMBER);
	FGeoGemInfo const& Magazine = *Catalog.Find("Magazine");
	TestFalse(TEXT("A zero echo share still shows its value"),
			  Magazine.GetMagnitudeText(Catalog.GetEchoCopies(EGeoGemTier::Cut, 1)).IsEmpty());

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);

	AActor* Character = World->SpawnActor<AActor>();
	UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Character);
	ASC->RegisterComponent();
	ASC->AddAttributeSetSubobject(NewObject<UCharacterAttributeSet>(Character));
	ASC->AddAttributeSetSubobject(NewObject<UGeoGemAttributeSet>(Character));
	ASC->InitAbilityActorInfo(Character, Character);
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetMaxAmmoAttribute(), 30.f);
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetDamageMultiplierAttribute(), 1.f);

	FGeoGemLoadout Loadout;
	Loadout.Sockets.Init(NAME_None, GeoGem::GetSockets().Num());
	auto Apply = [&](FName const Echo, int32 const EchoLevel)
	{
		Loadout.Echo = Echo;
		Loadout.EchoLevel = EchoLevel;
		return ASC->ApplyGameplayEffectSpecToSelf(*UGeoGemStatsEffect::MakeSpec(*ASC, Catalog, Loadout).Data);
	};

	FActiveGameplayEffectHandle Handle = Apply("Power", 1);
	TestEqual(TEXT("Echoed Chip: 30 sockets x 0.2% x 1%"),
			  ASC->GetNumericAttribute(UCharacterAttributeSet::GetDamageMultiplierAttribute()), 1.0006f,
			  KINDA_SMALL_NUMBER);
	ASC->RemoveActiveGameplayEffect(Handle);

	Handle = Apply("Power", 100);
	TestEqual(TEXT("Echoed Chip at level 100: 34% of 6%"),
			  ASC->GetNumericAttribute(UCharacterAttributeSet::GetDamageMultiplierAttribute()), 1.0204f,
			  KINDA_SMALL_NUMBER);
	ASC->RemoveActiveGameplayEffect(Handle);

	Handle = Apply("Magazine", 1);
	TestEqual(TEXT("Echoed Cut at level 1: no share yet"),
			  ASC->GetNumericAttribute(UCharacterAttributeSet::GetMaxAmmoAttribute()), 30.f, KINDA_SMALL_NUMBER);
	ASC->RemoveActiveGameplayEffect(Handle);

	Handle = Apply("Magazine", 2);
	TestEqual(TEXT("Echoed Cut at level 2: 15 sockets x 1.2% x 1%"),
			  ASC->GetNumericAttribute(UCharacterAttributeSet::GetMaxAmmoAttribute()), 30.054f, KINDA_SMALL_NUMBER);
	ASC->RemoveActiveGameplayEffect(Handle);

	Handle = Apply("Anchor", 3);
	TestEqual(TEXT("Echoed Prism at level 3: 9 sockets x 3% x 1%"),
			  ASC->GetNumericAttribute(UGeoGemAttributeSet::GetDeployableHealthMultiplierAttribute()), 1.0027f,
			  KINDA_SMALL_NUMBER);
	ASC->RemoveActiveGameplayEffect(Handle);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
