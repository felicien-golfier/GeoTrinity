// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/Component/FX/GeoIndicatorComponent.h"
#include "Components/SceneComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"
#include "Tool/Visual/GeoColor.h"
#include "World/ZoneLayer/GeoZoneLayerSubsystem.h"
#include "World/ZoneLayer/GeoZoneShape.h"
#include "World/ZoneLayer/GeoZoneVisualComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoZoneLayerTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

/** An actor at Location turned by Yaw, holding one zone shape RelativeLocation from its root. */
static UGeoZoneVisualComponent* SpawnZoneShape(UWorld& World, FVector const& Location, float const Yaw,
											   FVector const& RelativeLocation)
{
	AActor* const Actor = World.SpawnActor<AActor>(Location, FRotator(0.f, Yaw, 0.f));
	USceneComponent* const Root = NewObject<USceneComponent>(Actor);
	Actor->SetRootComponent(Root);
	Root->RegisterComponent();
	Actor->SetActorLocationAndRotation(Location, FRotator(0.f, Yaw, 0.f));

	UGeoZoneVisualComponent* const Shape = NewObject<UGeoZoneVisualComponent>(Actor);
	Shape->SetupAttachment(Root);
	Shape->SetRelativeLocation(RelativeLocation);
	Shape->RegisterComponent();
	return Shape;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoZoneLayerPackSlotTest, "GeoTrinity.ZoneLayer.PackSlot", GeoZoneLayerTestFlags)

bool FGeoZoneLayerPackSlotTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* const World = CreateTestWorld();

	UGeoZoneVisualComponent* const Circle =
		SpawnZoneShape(*World, FVector(100.f, 200.f, 0.f), 0.f, FVector::ZeroVector);
	Circle->Shape.Radius = 150.f;
	Circle->ColorMask = 0b101;
	Circle->Fill = 0.4f;
	GeoZoneLayer::FSlotTexels const CircleTexels = GeoZoneLayer::PackSlot(*Circle);
	TestEqual(TEXT("Circle head: shape, mask, no flag, fill"), CircleTexels[0], FLinearColor(0.f, 5.f, 0.f, 0.4f));
	TestEqual(TEXT("Circle: centre XY, radius"), CircleTexels[1], FLinearColor(100.f, 200.f, 150.f, 0.f));
	TestEqual(TEXT("Circle: no third row"), CircleTexels[2], FLinearColor::Transparent);

	// Turned a quarter: the beam runs along +Y, from 50 cm along the owner's forward.
	UGeoZoneVisualComponent* const Beam =
		SpawnZoneShape(*World, FVector(100.f, 200.f, 0.f), 90.f, FVector(50.f, 0.f, 0.f));
	Beam->Shape.Shape = EGeoZoneShape::Beam;
	Beam->Shape.Length = 300.f;
	Beam->Shape.HalfWidth = 20.f;
	Beam->ColorMask = 0b10;
	Beam->Layer = EGeoZoneLayer::Telegraph;
	Beam->bSafeSpotCut = true;
	Beam->Fill = 0.9f;
	// Half a second into two at server time 0: a quarter, whatever Fill says.
	Beam->SetTimedFill(-0.5f, 2.f);
	GeoZoneLayer::FSlotTexels const BeamTexels = GeoZoneLayer::PackSlot(*Beam);
	TestTrue(TEXT("Beam head: shape, mask, both flags, timed fill"),
			 BeamTexels[0].Equals(FLinearColor(1.f, 2.f, 3.f, 0.25f), UE_KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Beam: start XY, end XY"), BeamTexels[1].Equals(FLinearColor(100.f, 250.f, 100.f, 550.f), 0.01f));
	TestEqual(TEXT("Beam: half width"), BeamTexels[2], FLinearColor(20.f, 0.f, 0.f, 0.f));

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoZoneLayerColorMaskTest, "GeoTrinity.ZoneLayer.ColorMask", GeoZoneLayerTestFlags)

bool FGeoZoneLayerColorMaskTest::RunTest(FString const& /*Parameters*/)
{
	FGeoColorParam Heal;
	Heal.Color = EGeoColor::Heal;
	FGeoColorParam Shield;
	Shield.Color = EGeoColor::Shield;
	uint32 const HealBit = 1u << static_cast<uint32>(EGeoColor::Heal);
	uint32 const ShieldBit = 1u << static_cast<uint32>(EGeoColor::Shield);
	TestEqual(TEXT("One colour, one bit"), GeoZoneLayer::MakeColorMask(Heal, {}), HealBit);
	TestEqual(TEXT("Secondary colours add their bits, duplicates merge"),
			  GeoZoneLayer::MakeColorMask(Heal, {Shield, Heal}), HealBit | ShieldBit);

	AddExpectedErrorPlain(TEXT("Param.Color != EGeoColor::Override"), EAutomationExpectedErrorFlags::Contains, 0);
	AddExpectedErrorPlain(TEXT("has no palette slot to draw"), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedError(TEXT("=== Handled ensure: ===|^\\s*$|^Stack:|\\[Callstack\\]"),
					 EAutomationExpectedErrorFlags::Contains, -1);
	TestEqual(TEXT("Override is dropped"), GeoZoneLayer::MakeColorMask(Shield, {FGeoColorParam()}), ShieldBit);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoZoneLayerRegistryTest, "GeoTrinity.ZoneLayer.Registry", GeoZoneLayerTestFlags)

bool FGeoZoneLayerRegistryTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* const World = CreateTestWorld();
	UGeoZoneLayerSubsystem* const Subsystem = World->GetSubsystem<UGeoZoneLayerSubsystem>();
	if (TestNotNull(TEXT("A game world has a zone layer"), Subsystem))
	{
		TArray<UGeoZoneVisualComponent*> Shapes;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Shapes.Add(SpawnZoneShape(*World, FVector::ZeroVector, 0.f, FVector::ZeroVector));
			Shapes.Last()->Shape.Radius = Index + 1.f;
		}

		auto RadiusInSlot = [Subsystem](int32 const Slot)
		{
			return Subsystem->GetSlotTexels(Slot)[1].B;
		};
		Subsystem->PackSlots();
		TestEqual(TEXT("Registered shapes are drawn"), Subsystem->GetZoneCount(), 3);
		TestEqual(TEXT("In registration order"), RadiusInSlot(2), 3.f);

		Shapes[1]->SetZoneVisible(false);
		Subsystem->PackSlots();
		TestEqual(TEXT("A hidden shape is not drawn"), Subsystem->GetZoneCount(), 2);
		TestEqual(TEXT("The next one takes its slot"), RadiusInSlot(1), 3.f);

		Shapes[0]->DestroyComponent();
		Subsystem->PackSlots();
		TestEqual(TEXT("A destroyed shape is not drawn"), Subsystem->GetZoneCount(), 1);
		TestEqual(TEXT("No gap"), RadiusInSlot(0), 3.f);

		Shapes[1]->SetZoneVisible(true);
		Subsystem->PackSlots();
		TestEqual(TEXT("Shown again: drawn after the others"), RadiusInSlot(1), 2.f);

		Shapes[2]->GetOwner()->SetActorHiddenInGame(true);
		Subsystem->PackSlots();
		TestEqual(TEXT("An owner hidden in game hides its shape"), Subsystem->GetZoneCount(), 1);
		Shapes[2]->GetOwner()->SetActorHiddenInGame(false);

		for (int32 Index = 0; Index < GeoZoneLayer::MaxZoneSlots; ++Index)
		{
			SpawnZoneShape(*World, FVector::ZeroVector, 0.f, FVector::ZeroVector);
		}

		AddExpectedErrorPlain(TEXT("ZoneCount < GeoZoneLayer::MaxZoneSlots"), EAutomationExpectedErrorFlags::Contains,
							  0);
		AddExpectedErrorPlain(TEXT("the rest are not drawn"), EAutomationExpectedErrorFlags::Contains, 1);
		AddExpectedError(TEXT("=== Handled ensure: ===|^\\s*$|^Stack:|\\[Callstack\\]"),
						 EAutomationExpectedErrorFlags::Contains, -1);
		Subsystem->PackSlots();
		TestEqual(TEXT("Never more than MaxZoneSlots"), Subsystem->GetZoneCount(), GeoZoneLayer::MaxZoneSlots);
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoZoneLayerIndicatorExpiryTest, "GeoTrinity.ZoneLayer.IndicatorExpiry",
								 GeoZoneLayerTestFlags)

bool FGeoZoneLayerIndicatorExpiryTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* const World = CreateTestWorld();
	UGeoZoneLayerSubsystem* const Subsystem = World->GetSubsystem<UGeoZoneLayerSubsystem>();
	FGeoIndicatorState State;
	State.Size = FVector2D(100.f, 0.f);
	State.ColorMask = 1u << static_cast<uint32>(EGeoColor::Damage);
	State.Duration = 1.f;
	UGeoZoneVisualComponent* const Timed = UGeoIndicatorComponent::SpawnIndicator(World, State);
	State.Duration = 0.f;
	UGeoZoneVisualComponent* const Lasting = UGeoIndicatorComponent::SpawnIndicator(World, State);
	State.StartServerTime = -2.f;
	State.Duration = 1.f;
	TestNull(TEXT("Already over: nothing drawn"), UGeoIndicatorComponent::SpawnIndicator(World, State));
	if (TestNotNull(TEXT("A game world has a zone layer"), Subsystem) && TestNotNull(TEXT("Timed drawn"), Timed)
		&& TestNotNull(TEXT("Lasting drawn"), Lasting))
	{
		Subsystem->PackSlots();
		TestEqual(TEXT("Both telegraphs drawn"), Subsystem->GetZoneCount(), 2);

		TickTestTimers(*World, 0.f);
		TickTestTimers(*World, 0.5f);
		Subsystem->PackSlots();
		TestEqual(TEXT("Halfway: still drawn"), Subsystem->GetZoneCount(), 2);

		TickTestTimers(*World, 0.6f);
		Subsystem->PackSlots();
		TestEqual(TEXT("Past its Duration: removed itself"), Subsystem->GetZoneCount(), 1);
		TestTrue(TEXT("The one left lasts until removed"), IsValid(Lasting) && Lasting->IsRegistered());
	}

	DestroyTestWorld(World);
	return true;
}

#endif
