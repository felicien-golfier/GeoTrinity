// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Animation/AnimSequence.h"
#include "Misc/AutomationTest.h"
#include "Tool/GeoAnimBuilderUtil.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoAnimBuilderTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoBoneChannelKeysTest, "GeoTrinity.AnimBuilder.BoneChannelKeys",
								 GeoAnimBuilderTestFlags)

bool FGeoBoneChannelKeysTest::RunTest(FString const& /*Parameters*/)
{
	UAnimSequence const* Source = LoadObject<UAnimSequence>(
		nullptr, TEXT("/Game/Characters/Anim/ClassBadge/Circle/SK_CircleBadge_Sequence_Idle.SK_CircleBadge_Sequence_Idle"));
	if (!TestNotNull(TEXT("The Circle badge idle loads"), Source))
	{
		return false;
	}

	UAnimSequence* Sequence = DuplicateObject<UAnimSequence>(Source, GetTransientPackage());
	FName const Bone = TEXT("bottom");
	int32 const Channel = 0;

	TestTrue(TEXT("The bone tracks are listed"), UGeoAnimBuilderUtil::GetBoneTrackNames(Sequence).Contains(Bone));

	TArray<int32> const Frames = {0, 10, 30};
	TArray<float> const Values = {1.f, 5.f, -2.f};
	TestTrue(TEXT("Three sparse keys are written"),
			 UGeoAnimBuilderUtil::SetBoneChannelKeys(Sequence, Bone, Channel, Frames, Values, 0.f));

	TArray<int32> ReadFrames;
	TArray<float> ReadValues;
	float Constant = 0.f;
	TestTrue(TEXT("The channel reads back"),
			 UGeoAnimBuilderUtil::GetBoneChannelKeys(Sequence, Bone, Channel, ReadFrames, ReadValues, Constant));
	TestEqual(TEXT("Frames read back as written"), ReadFrames, Frames);
	TestEqual(TEXT("Values read back as written"), ReadValues, Values);

	TestTrue(TEXT("An empty key list makes the channel a constant"),
			 UGeoAnimBuilderUtil::SetBoneChannelKeys(Sequence, Bone, Channel, {}, {}, 7.f));
	ReadFrames.Reset();
	ReadValues.Reset();
	UGeoAnimBuilderUtil::GetBoneChannelKeys(Sequence, Bone, Channel, ReadFrames, ReadValues, Constant);
	TestEqual(TEXT("A constant holds no key"), ReadFrames.Num(), 0);
	TestEqual(TEXT("The constant reads back"), Constant, 7.f);

	TestFalse(TEXT("A bone the skeleton lacks is refused"),
			  UGeoAnimBuilderUtil::SetBoneChannelKeys(Sequence, TEXT("NoSuchBone"), Channel, Frames, Values, 0.f));
	return true;
}

#endif
