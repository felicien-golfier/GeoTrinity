// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoAnimBuilderUtil.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "FileHelpers.h"

namespace
{
	void FinishMontageEdit(UAnimMontage* Montage)
	{
		Montage->UpdateLinkableElements();
		Montage->PostEditChange();
		UEditorLoadingAndSavingUtils::SavePackages({Montage->GetPackage()}, false);
	}
}

void UGeoAnimBuilderUtil::SetMontageSlotSegment(UAnimMontage* Montage, UAnimSequence* Sequence, FName SlotName)
{
	if (!ensureMsgf(Montage && Sequence, TEXT("SetMontageSlotSegment needs both a Montage and a Sequence")))
	{
		return;
	}
	if (!ensureMsgf(Montage->GetSkeleton() == Sequence->GetSkeleton(),
					TEXT("Montage %s and Sequence %s use different skeletons"), *Montage->GetName(),
					*Sequence->GetName()))
	{
		return;
	}

	Montage->Modify();

	FAnimSegment Segment;
	Segment.SetAnimReference(Sequence, true);

	FSlotAnimationTrack Track;
	Track.SlotName = SlotName;
	Track.AnimTrack.AnimSegments.Add(Segment);

	Montage->SlotAnimTracks.Empty(1);
	Montage->SlotAnimTracks.Add(Track);
	Montage->CompositeSections.Empty();
	Montage->SetCompositeLength(Sequence->GetPlayLength());

	FinishMontageEdit(Montage);
}

void UGeoAnimBuilderUtil::SetMontageSections(UAnimMontage* Montage, TArray<FName> SectionNames,
											 TArray<float> StartTimes, TArray<FName> NextSectionNames)
{
	if (!ensureMsgf(Montage, TEXT("SetMontageSections needs a Montage")))
	{
		return;
	}
	if (!ensureMsgf(SectionNames.Num() > 0 && SectionNames.Num() == StartTimes.Num()
						&& SectionNames.Num() == NextSectionNames.Num(),
					TEXT("SetMontageSections needs three non-empty arrays of equal length (got %d/%d/%d)"),
					SectionNames.Num(), StartTimes.Num(), NextSectionNames.Num()))
	{
		return;
	}
	if (!ensureMsgf(Montage->SlotAnimTracks.Num() > 0,
					TEXT("Montage %s has no slot track — call SetMontageSlotSegment first"), *Montage->GetName()))
	{
		return;
	}

	Montage->Modify();
	Montage->CompositeSections.Empty(SectionNames.Num());
	for (int32 Index = 0; Index < SectionNames.Num(); ++Index)
	{
		FCompositeSection Section;
		Section.SectionName = SectionNames[Index];
		Section.NextSectionName = NextSectionNames[Index];
		Section.Link(Montage, StartTimes[Index]);
		Montage->CompositeSections.Add(Section);
	}

	FinishMontageEdit(Montage);
}

void UGeoAnimBuilderUtil::InspectMontage(UAnimMontage* Montage)
{
	if (!ensureMsgf(Montage, TEXT("InspectMontage needs a Montage")))
	{
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("Montage %s — length %.3f, %d slot track(s), %d section(s)"), *Montage->GetName(),
		   Montage->GetPlayLength(), Montage->SlotAnimTracks.Num(), Montage->CompositeSections.Num());

	for (FSlotAnimationTrack const& Track : Montage->SlotAnimTracks)
	{
		UE_LOG(LogTemp, Display, TEXT("  slot '%s'"), *Track.SlotName.ToString());
		for (FAnimSegment const& Segment : Track.AnimTrack.AnimSegments)
		{
			UAnimSequenceBase const* Reference = Segment.GetAnimReference();
			UE_LOG(LogTemp, Display, TEXT("    segment %s start %.3f anim [%.3f..%.3f] rate %.2f loops %d"),
				   Reference ? *Reference->GetName() : TEXT("None"), Segment.StartPos, Segment.AnimStartTime,
				   Segment.AnimEndTime, Segment.AnimPlayRate, Segment.LoopingCount);
		}
	}

	for (FCompositeSection const& Section : Montage->CompositeSections)
	{
		UE_LOG(LogTemp, Display, TEXT("  section '%s' at %.3f -> '%s'"), *Section.SectionName.ToString(),
			   Section.GetTime(), *Section.NextSectionName.ToString());
	}
}
