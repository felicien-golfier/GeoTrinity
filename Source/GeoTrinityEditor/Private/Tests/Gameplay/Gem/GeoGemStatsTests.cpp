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
#include "Settings/GameDataSettings.h"
#include "Tests/Gameplay/Fake/GeoTestDeployable.h"
#include "Tests/Gameplay/Fake/GeoTestGemDrop.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"
#include "Tests/Gameplay/Gem/GeoGemTestHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoGemTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemClassXpTest, "GeoTrinity.Gems.ClassXp", GeoGemTestFlags)

bool FGeoGemClassXpTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();
	UGeoGemProfileSave* Profile = NewObject<UGeoGemProfileSave>();

	TestEqual(TEXT("A whole health bar on Original"), Catalog.GetXp(EGeoBossType::Boss, 1.f, EGeoDifficulty::Original),
			  400);
	TestEqual(TEXT("Half a health bar, boss alive"), Catalog.GetXp(EGeoBossType::Boss, 0.5f, EGeoDifficulty::Original),
			  200);
	TestEqual(TEXT("Reduced scales XP"), Catalog.GetXp(EGeoBossType::Boss, 1.f, EGeoDifficulty::Reduced), 300);
	TestEqual(TEXT("Safe scales XP"), Catalog.GetXp(EGeoBossType::Boss, 1.f, EGeoDifficulty::Safe), 200);
	TestEqual(TEXT("No more than one health bar"), Catalog.GetXp(EGeoBossType::Boss, 1.5f, EGeoDifficulty::Original),
			  400);
	TestEqual(TEXT("No boss health taken, no XP"), Catalog.GetXp(EGeoBossType::Boss, 0.f, EGeoDifficulty::Original), 0);
	TestEqual(TEXT("A mini-boss bar has its own worth"),
			  Catalog.GetXp(EGeoBossType::MiniBoss, 0.5f, EGeoDifficulty::Original),
			  FMath::RoundToInt32(Catalog.XpPerHealthBar.FindRef(EGeoBossType::MiniBoss) * 0.5f));

	TestEqual(TEXT("Just short of a level"), Profile->AddClassXp(Catalog, EPlayerClass::Triangle, 999), 0);
	TestEqual(TEXT("XP kept towards the next level"), Profile->GetClassXp(EPlayerClass::Triangle), 999);
	TestEqual(TEXT("One more XP levels up"), Profile->AddClassXp(Catalog, EPlayerClass::Triangle, 1), 1);
	TestEqual(TEXT("Level 2"), Profile->GetClassLevel(EPlayerClass::Triangle), 2);
	TestEqual(TEXT("Leftover XP carries over"), Profile->AddClassXp(Catalog, EPlayerClass::Triangle, 2500), 2);
	TestEqual(TEXT("Level 4 with 500 XP"), Profile->GetClassXp(EPlayerClass::Triangle), 500);
	TestEqual(TEXT("Other classes level on their own"), Profile->GetClassLevel(EPlayerClass::Circle), 1);

	int32 XpTaken = 0;
	int32 XpNeeded = 0;
	TestEqual(TEXT("Bar: 500 of 1000 into level 4"),
			  Profile->GetXpProgress(Catalog, EPlayerClass::Triangle, XpTaken, XpNeeded), 0.5f);
	TestEqual(TEXT("Bar numbers below the max"), XpTaken, 500);
	TestEqual(TEXT("Bar needs one level of XP"), XpNeeded, Catalog.XpPerLevel);

	Profile->AddClassXp(Catalog, EPlayerClass::Triangle, 1000000);
	TestEqual(TEXT("Level stops at the max"), Profile->GetClassLevel(EPlayerClass::Triangle), GeoGem::MaxClassLevel);
	TestEqual(TEXT("No XP kept at the max"), Profile->GetClassXp(EPlayerClass::Triangle), 0);

	int32 const XpToMax = (GeoGem::MaxClassLevel - 1) * Catalog.XpPerLevel;
	Profile->AddClassXp(Catalog, EPlayerClass::Square, XpToMax - 1);
	TestEqual(TEXT("Linear: one XP short of the max is level 19"), Profile->GetClassLevel(EPlayerClass::Square), 19);
	TestEqual(TEXT("47.5 Original health bars to level 20, about 10 hours at five kills an hour"),
			  static_cast<float>(XpToMax) / Catalog.XpPerHealthBar.FindRef(EGeoBossType::Boss), 47.5f);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemStatsTest, "GeoTrinity.Gems.CharacterStats", GeoGemTestFlags)

bool FGeoGemStatsTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);

	AActor* Character = World->SpawnActor<AActor>();
	UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Character);
	ASC->RegisterComponent();
	ASC->AddAttributeSetSubobject(NewObject<UCharacterAttributeSet>(Character));
	ASC->AddAttributeSetSubobject(NewObject<UGeoGemAttributeSet>(Character));
	ASC->InitAbilityActorInfo(Character, Character);

	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetMaxHealthAttribute(), 100.f);
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetMaxAmmoAttribute(), 30.f);
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetDamageMultiplierAttribute(), 1.f);
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetDamageReductionAttribute(), 0.f);
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetMovementSpeedMultiplierAttribute(), 1.f);

	auto Value = [ASC](FGameplayAttribute const& Attribute)
	{
		return ASC->GetNumericAttribute(Attribute);
	};

	FGeoGemLoadout Loadout;
	Loadout.Sockets.Init(NAME_None, GeoGem::GetSockets().Num());
	for (int32 i = 0; i < 20; ++i)
	{
		Loadout.Sockets[i] = "Power";
	}
	for (int32 i = 20; i < 30; ++i)
	{
		Loadout.Sockets[i] = "Vigor";
	}
	for (int32 i = 30; i < 45; ++i)
	{
		Loadout.Sockets[i] = "Magazine";
	}
	for (int32 i = 45; i < 54; ++i)
	{
		Loadout.Sockets[i] = "Anchor";
	}
	Loadout.Sockets[54] = "Surplus";

	FActiveGameplayEffectHandle Handle =
		ASC->ApplyGameplayEffectSpecToSelf(*UGeoGemStatsEffect::MakeSpec(*ASC, Catalog, Loadout).Data);
	TestTrue(TEXT("Gem effect applied"), Handle.IsValid());
	TestEqual(TEXT("20 Power: damage +4%"), Value(UCharacterAttributeSet::GetDamageMultiplierAttribute()), 1.04f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("10 Vigor: max health +3.5%"), Value(UCharacterAttributeSet::GetMaxHealthAttribute()), 103.5f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("15 Magazine: max ammo +18%"), Value(UCharacterAttributeSet::GetMaxAmmoAttribute()), 35.4f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("No Guard: damage reduction untouched"),
			  Value(UCharacterAttributeSet::GetDamageReductionAttribute()), 0.f);
	TestEqual(TEXT("No Swift: speed untouched"), Value(UCharacterAttributeSet::GetMovementSpeedMultiplierAttribute()),
			  1.f);
	TestEqual(TEXT("9 Anchor: deployable health +27%"),
			  Value(UGeoGemAttributeSet::GetDeployableHealthMultiplierAttribute()), 1.27f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("No Precision: no base crit chance"), Value(UGeoGemAttributeSet::GetCritChanceAttribute()), 0.f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("No Edge: base crit damage"), Value(UGeoGemAttributeSet::GetCritDamageAttribute()), 1.2f,
			  KINDA_SMALL_NUMBER);
	TestFalse(TEXT("No Critical Core: no crit tag"),
			  ASC->HasMatchingGameplayTag(FGeoGameplayTags::Get().Gem_Core_Critical));

	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetCritChanceAttribute(), 1.f);
	TestEqual(TEXT("Without Critical, a sure crit chance still crits for 120%"), GeoASLib::RollCritMultiplier(ASC),
			  1.2f, KINDA_SMALL_NUMBER);
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetCritChanceAttribute(), 0.f);
	TestEqual(TEXT("Without Critical and no crit chance: no crit"), GeoASLib::RollCritMultiplier(ASC), 1.f);

	ASC->RemoveActiveGameplayEffect(Handle);
	Loadout.Sockets[54] = "Critical";
	Handle = ASC->ApplyGameplayEffectSpecToSelf(*UGeoGemStatsEffect::MakeSpec(*ASC, Catalog, Loadout).Data);
	TestTrue(TEXT("Critical slotted: crit tag held"),
			 ASC->HasMatchingGameplayTag(FGeoGameplayTags::Get().Gem_Core_Critical));
	TestEqual(TEXT("Critical: crit chance +10%"), Value(UGeoGemAttributeSet::GetCritChanceAttribute()), 0.1f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Critical: crit damage +30%"), Value(UGeoGemAttributeSet::GetCritDamageAttribute()), 1.5f,
			  KINDA_SMALL_NUMBER);
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetCritChanceAttribute(), 1.f);
	TestEqual(TEXT("Critical and a sure crit chance: every hit deals 150%"), GeoASLib::RollCritMultiplier(ASC), 1.5f,
			  KINDA_SMALL_NUMBER);
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetCritChanceAttribute(), 0.f);

	ASC->RemoveActiveGameplayEffect(Handle);
	for (int32 i = 0; i < 30; ++i)
	{
		Loadout.Sockets[i] = "Guard";
	}
	Handle = ASC->ApplyGameplayEffectSpecToSelf(*UGeoGemStatsEffect::MakeSpec(*ASC, Catalog, Loadout).Data);
	TestEqual(TEXT("Respec: damage back to base"), Value(UCharacterAttributeSet::GetDamageMultiplierAttribute()), 1.f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Respec: max health back to base"), Value(UCharacterAttributeSet::GetMaxHealthAttribute()), 100.f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("30 Guard: damage reduction +6%"), Value(UCharacterAttributeSet::GetDamageReductionAttribute()),
			  0.06f, KINDA_SMALL_NUMBER);

	ASC->RemoveActiveGameplayEffect(Handle);
	TestFalse(TEXT("Removing the effect takes the Core's tag"),
			  ASC->HasMatchingGameplayTag(FGeoGameplayTags::Get().Gem_Core_Critical));
	TestEqual(TEXT("Removing the effect clears every gem stat"),
			  Value(UCharacterAttributeSet::GetDamageReductionAttribute()), 0.f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Removing the effect restores max ammo"), Value(UCharacterAttributeSet::GetMaxAmmoAttribute()), 30.f,
			  KINDA_SMALL_NUMBER);

	FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetDashCooldownMultiplierAttribute(), 0.8f);
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetDeployableCooldownMultiplierAttribute(), 0.85f);
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetWindUpMultiplierAttribute(), 0.9f);
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetReloadSpeedMultiplierAttribute(), 1.25f);
	FGameplayTagContainer const DashTags(Tags.Ability_Type_Dash);
	FGameplayTagContainer const DeployableTags(Tags.Ability_Type_Deployable);
	FGameplayTagContainer const BasicTags(Tags.Ability_Type_Basic);
	FGameplayTagContainer ReloadTags(Tags.Ability_Type_Basic);
	ReloadTags.AddTag(Tags.Ability_Type_Reload);
	TestEqual(TEXT("Dash cooldown reads the dash multiplier"), GeoASLib::GetGemCooldownMultiplier(ASC, DashTags), 0.8f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Deploy recharge reads the deployable multiplier"),
			  GeoASLib::GetGemCooldownMultiplier(ASC, DeployableTags), 0.85f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("A basic ability's cooldown is untouched"), GeoASLib::GetGemCooldownMultiplier(ASC, BasicTags), 1.f);
	TestEqual(TEXT("Wind-up scales a basic ability's fire delay"), GeoASLib::GetGemFireDelayMultiplier(ASC, BasicTags),
			  0.9f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Reload speed +25% makes the reload 80% as long"),
			  GeoASLib::GetGemFireDelayMultiplier(ASC, ReloadTags), 0.8f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("No ASC: fire delay untouched"), GeoASLib::GetGemFireDelayMultiplier(nullptr, BasicTags), 1.f);

	float const GeneralSpellDistance = GetDefault<UGameDataSettings>()->GeneralSpellDistance;
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetSpellDistanceMultiplierAttribute(), 1.1f);
	TestEqual(TEXT("Reach +10% lengthens beams by 10%"), GeoASLib::GetSpellDistance(ASC), GeneralSpellDistance * 1.1f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("No ASC: beams keep the general spell distance"), GeoASLib::GetSpellDistance(nullptr),
			  GeneralSpellDistance);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemStatsSurviveDeathTest, "GeoTrinity.Gems.StatsSurviveDeath", GeoGemTestFlags)

bool FGeoGemStatsSurviveDeathTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();

	// A listen-server world: the death purge is server-only.
	UWorld* World = UWorld::CreateWorld(EWorldType::PIE, false);
	World->SetPlayInEditorInitialNetMode(NM_ListenServer);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::PIE);
	WorldContext.SetCurrentWorld(World);

	AActor* Character = World->SpawnActor<AActor>();
	UGeoAbilitySystemComponent* ASC = NewObject<UGeoAbilitySystemComponent>(Character);
	ASC->RegisterComponent();
	ASC->AddAttributeSetSubobject(NewObject<UCharacterAttributeSet>(Character));
	ASC->AddAttributeSetSubobject(NewObject<UGeoGemAttributeSet>(Character));
	ASC->InitAbilityActorInfo(Character, Character);
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetDamageMultiplierAttribute(), 1.f);
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetDamageReductionAttribute(), 0.f);

	FGeoGemLoadout Loadout;
	Loadout.Sockets.Init(NAME_None, GeoGem::GetSockets().Num());
	for (int32 i = 0; i < 20; ++i)
	{
		Loadout.Sockets[i] = "Power";
	}
	FActiveGameplayEffectHandle const StatsHandle =
		ASC->ApplyGameplayEffectSpecToSelf(*UGeoGemStatsEffect::MakeSpec(*ASC, Catalog, Loadout).Data);

	UGameplayEffect* OtherEffect = NewObject<UGameplayEffect>(GetTransientPackage());
	OtherEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	FGameplayModifierInfo& Modifier = OtherEffect->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UCharacterAttributeSet::GetDamageReductionAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.5f));
	FActiveGameplayEffectHandle const OtherHandle =
		ASC->ApplyGameplayEffectToSelf(OtherEffect, 1.f, ASC->MakeEffectContext());

	UGameplayEffect* CooldownEffect = NewObject<UGameplayEffect>(GetTransientPackage());
	CooldownEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	FInheritedTagContainer CooldownTags;
	CooldownTags.AddTag(FGeoGameplayTags::Get().Ability_Cooldown);
	CooldownEffect->FindOrAddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(CooldownTags);
	ASC->ApplyGameplayEffectToSelf(CooldownEffect, 1.f, ASC->MakeEffectContext());
	TestTrue(TEXT("Cooldown tag held before death"),
			 ASC->HasMatchingGameplayTag(FGeoGameplayTags::Get().Ability_Cooldown));

	ASC->HandleOwnerDeath({StatsHandle});

	TestTrue(TEXT("Gem stats effect survives death"), ASC->GetActiveGameplayEffect(StatsHandle) != nullptr);
	TestEqual(TEXT("Gem stats still modify the attribute"),
			  ASC->GetNumericAttribute(UCharacterAttributeSet::GetDamageMultiplierAttribute()), 1.04f,
			  KINDA_SMALL_NUMBER);
	TestTrue(TEXT("An unrelated effect is purged"), ASC->GetActiveGameplayEffect(OtherHandle) == nullptr);
	TestEqual(TEXT("Its modifier is gone"),
			  ASC->GetNumericAttribute(UCharacterAttributeSet::GetDamageReductionAttribute()), 0.f, KINDA_SMALL_NUMBER);
	TestFalse(TEXT("The cooldown is cleared"), ASC->HasMatchingGameplayTag(FGeoGameplayTags::Get().Ability_Cooldown));

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoRelaySnapDashTest, "GeoTrinity.Gems.RelaySnapDash", GeoGemTestFlags)

bool FGeoRelaySnapDashTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	FGeoGemInfo const* Relay = Catalog ? Catalog->Find("Relay") : nullptr;
	if (!TestTrue(TEXT("The catalog's Relay gives a Core passive"), Relay && Relay->GrantedAbility))
	{
		return false;
	}

	UWorld* World = CreateTestWorld();

	AGeoTestDeployer* Deployer = World->SpawnActor<AGeoTestDeployer>();
	NewObject<UGeoDeployableManagerComponent>(Deployer)->RegisterComponent();
	UGeoAbilitySystemComponent* ASC = Cast<UGeoAbilitySystemComponent>(Deployer->GetAbilitySystemComponent());
	ASC->GiveAbility(FGameplayAbilitySpec(UGeoTestDeployAbility::StaticClass(), 1));
	FGameplayAbilitySpecHandle const RelayHandle = ASC->GiveAbility(FGameplayAbilitySpec(Relay->GrantedAbility, 1));
	bool const bRelayActive = ASC->TryActivateAbility(RelayHandle);

	FDeployableData Data;
	Data.Owner = Deployer;
	Data.Instigator = Deployer;
	Data.Params.LifeDrainMaxDuration = 10.f;
	AGeoDeployableBase const* Deployable =
		GeoASLib::FullySpawnDeployable(AGeoTestDeployable::StaticClass(), Data, FTransform(FVector(300.f, 0.f, 0.f)));

	if (TestTrue(TEXT("Relay activates and the deployable spawns"), bRelayActive && Deployable))
	{
		FVector Direction = FVector::ZeroVector;
		float Distance = 0.f;
		auto const AimDash = [ASC, &Direction, &Distance](bool const bHasDirection, float const Yaw)
		{
			Direction = FRotator(0.f, Yaw, 0.f).Vector();
			Distance = 500.f;
			ASC->OnDashAiming.Broadcast(FVector::ZeroVector, bHasDirection, Direction, Distance);
		};

		AimDash(true, 10.f);
		TestTrue(TEXT("A dash 10 degrees off turns straight at the deployable"),
				 Direction.Equals(FVector::ForwardVector, KINDA_SMALL_NUMBER));
		TestEqual(TEXT("It stops on the deployable"), Distance, 300.f, 1.f);

		AimDash(true, 90.f);
		TestEqual(TEXT("A dash at 90 degrees keeps its direction"), Direction.Y, 1.0,
				  static_cast<double>(UE_KINDA_SMALL_NUMBER));
		TestEqual(TEXT("And its distance"), Distance, 500.f);

		AimDash(false, 90.f);
		TestTrue(TEXT("A dash from standing still heads for the deployable"),
				 Direction.Equals(FVector::ForwardVector, KINDA_SMALL_NUMBER));

		ASC->CancelAllAbilities();
		AimDash(true, 10.f);
		TestTrue(TEXT("The revive reset (CancelAllAbilities, also run by the owning client) keeps the snap"),
				 Direction.Equals(FVector::ForwardVector, KINDA_SMALL_NUMBER));

		ASC->ClearAbility(RelayHandle);
		AimDash(true, 10.f);
		TestTrue(TEXT("Removing the Relay spec stops the snap"),
				 Direction.Equals(FRotator(0.f, 10.f, 0.f).Vector(), KINDA_SMALL_NUMBER));
	}

	DestroyTestWorld(World);
	return true;
}

#endif
