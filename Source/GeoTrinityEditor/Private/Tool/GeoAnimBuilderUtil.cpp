// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoAnimBuilderUtil.h"

#include "AnimGraphNode_ApplyAdditive.h"
#include "AnimGraphNode_IdentityPose.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_SaveCachedPose.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_TwoWayBlend.h"
#include "AnimGraphNode_UseCachedPose.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "EdGraphSchema_K2.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "FileHelpers.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "MeshDescription.h"
#include "ReferenceSkeleton.h"
#include "Rendering/SkeletalMeshModel.h"
#include "StaticToSkeletalMeshConverter.h"

namespace
{
	void FinishMontageEdit(UAnimMontage* Montage)
	{
		Montage->UpdateLinkableElements();
		Montage->PostEditChange();
		UEditorLoadingAndSavingUtils::SavePackages({Montage->GetPackage()}, false);
	}
}

void UGeoAnimBuilderUtil::SetMontageSlotSegment(UAnimMontage* Montage, UAnimSequence* Sequence, FName SlotName,
												 float PlayLength)
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
	if (!ensureMsgf(PlayLength <= Sequence->GetPlayLength(),
					TEXT("Montage %s cannot play %.3f s of Sequence %s, which lasts %.3f s"), *Montage->GetName(),
					PlayLength, *Sequence->GetName(), Sequence->GetPlayLength()))
	{
		return;
	}

	Montage->Modify();

	FAnimSegment Segment;
	Segment.SetAnimReference(Sequence, true);
	if (PlayLength > 0.f)
	{
		Segment.AnimEndTime = PlayLength;
	}

	FSlotAnimationTrack Track;
	Track.SlotName = SlotName;
	Track.AnimTrack.AnimSegments.Add(Segment);

	Montage->SlotAnimTracks.Empty(1);
	Montage->SlotAnimTracks.Add(Track);
	Montage->CompositeSections.Empty();
	Montage->SetCompositeLength(Segment.AnimEndTime);

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

void UGeoAnimBuilderUtil::GetMontageSections(UAnimMontage const* Montage, TArray<FName>& OutSectionNames,
											 TArray<float>& OutStartTimes, TArray<FName>& OutNextSectionNames)
{
	if (!ensureMsgf(Montage, TEXT("GetMontageSections needs a Montage")))
	{
		return;
	}

	for (FCompositeSection const& Section : Montage->CompositeSections)
	{
		OutSectionNames.Add(Section.SectionName);
		OutStartTimes.Add(Section.GetTime());
		OutNextSectionNames.Add(Section.NextSectionName);
	}
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

TArray<FVector> UGeoAnimBuilderUtil::GetSkeletalMeshVertexPositions(USkeletalMesh* Mesh)
{
	TArray<FVector> Positions;
	if (!ensureMsgf(Mesh, TEXT("GetSkeletalMeshVertexPositions needs a SkeletalMesh")))
	{
		return Positions;
	}

	FMeshDescription MeshDescription;
	if (!ensureMsgf(Mesh->CloneMeshDescription(0, MeshDescription),
					TEXT("SkeletalMesh %s has no LOD 0 mesh description"), *Mesh->GetName()))
	{
		return Positions;
	}

	int32 const NumVertices = MeshDescription.Vertices().Num();
	Positions.Reserve(NumVertices);
	for (int32 Index = 0; Index < NumVertices; ++Index)
	{
		Positions.Add(FVector(MeshDescription.GetVertexPosition(FVertexID(Index))));
	}
	return Positions;
}

bool UGeoAnimBuilderUtil::RebuildSkeletalMeshFromStaticMesh(USkeletalMesh* Mesh, UStaticMesh* StaticMesh,
															USkeleton* Skeleton, TArray<FName> BoneNames,
															TArray<FName> ParentNames, TArray<FTransform> Transforms)
{
	if (!ensureMsgf(Mesh && StaticMesh && Skeleton,
					TEXT("RebuildSkeletalMeshFromStaticMesh needs a Mesh, a StaticMesh and a Skeleton")))
	{
		return false;
	}
	if (!ensureMsgf(BoneNames.Num() > 0 && BoneNames.Num() == ParentNames.Num()
						&& BoneNames.Num() == Transforms.Num(),
					TEXT("RebuildSkeletalMeshFromStaticMesh needs three non-empty arrays of equal length (got %d/%d/%d)"),
					BoneNames.Num(), ParentNames.Num(), Transforms.Num()))
	{
		return false;
	}

	FReferenceSkeleton ReferenceSkeleton;
	{
		FReferenceSkeletonModifier Builder(ReferenceSkeleton, Skeleton);
		for (int32 Index = 0; Index < BoneNames.Num(); ++Index)
		{
			int32 const ParentIndex = Builder.FindBoneIndex(ParentNames[Index]);
			if (!ensureMsgf(Index == 0 || ParentIndex != INDEX_NONE,
							TEXT("Bone %s parents to %s, which no earlier bone declares"),
							*BoneNames[Index].ToString(), *ParentNames[Index].ToString()))
			{
				return false;
			}
			Builder.Add(FMeshBoneInfo(BoneNames[Index], BoneNames[Index].ToString(),
									  Index == 0 ? INDEX_NONE : ParentIndex),
						Transforms[Index]);
		}
	}

	Mesh->GetImportedModel()->LODModels.Empty();
	Mesh->SetNumSourceModels(0);

	if (!ensureMsgf(FStaticToSkeletalMeshConverter::InitializeSkeletalMeshFromStaticMesh(Mesh, StaticMesh,
																						 ReferenceSkeleton),
					TEXT("Could not build SkeletalMesh %s out of StaticMesh %s"), *Mesh->GetName(),
					*StaticMesh->GetName()))
	{
		return false;
	}

	// The cached bind pose only recomputes when the bone count changes, and a rebuild may move bones instead.
	Mesh->GetRefBasesInvMatrix().Reset();
	Mesh->CalculateInvRefMatrices();

	// Mesh-sampled Niagara effects read the triangles on the CPU.
	Mesh->GetLODInfo(0)->bAllowCPUAccess = true;
	Mesh->SetSkeleton(Skeleton);
	Skeleton->RecreateBoneTree(Mesh);
	Skeleton->SetPreviewMesh(Mesh);

	UEditorLoadingAndSavingUtils::SavePackages({Mesh->GetPackage(), Skeleton->GetPackage()}, false);
	return true;
}

bool UGeoAnimBuilderUtil::BuildLayeredAnimGraph(UAnimBlueprint* AnimBlueprint, FName LayerBone, FName BaseSlotName,
												FName LayerSlotName, FName FullBodySlotName, FName AdditiveSlotName,
												UAnimSequence* Idle)
{
	if (!ensureMsgf(AnimBlueprint && AnimBlueprint->TargetSkeleton,
					TEXT("BuildLayeredAnimGraph needs an AnimBlueprint with a target skeleton")))
	{
		return false;
	}

	USkeleton* Skeleton = AnimBlueprint->TargetSkeleton;
	if (!ensureMsgf(Skeleton->GetReferenceSkeleton().FindBoneIndex(LayerBone) != INDEX_NONE,
					TEXT("Skeleton %s has no bone %s to split the layers at"), *Skeleton->GetName(),
					*LayerBone.ToString()))
	{
		return false;
	}

	TObjectPtr<UEdGraph> const* AnimGraphEntry = AnimBlueprint->FunctionGraphs.FindByPredicate(
		[](UEdGraph const* Graph) { return Graph->GetFName() == UEdGraphSchema_K2::GN_AnimGraph; });
	if (!ensureMsgf(AnimGraphEntry, TEXT("AnimBlueprint %s has no AnimGraph"), *AnimBlueprint->GetName()))
	{
		return false;
	}

	UEdGraph* AnimGraph = *AnimGraphEntry;
	UAnimGraphNode_Root* Output = nullptr;
	TArray<TObjectPtr<UEdGraphNode>> const ExistingNodes = AnimGraph->Nodes;
	for (UEdGraphNode* Node : ExistingNodes)
	{
		if (UAnimGraphNode_Root* Root = Cast<UAnimGraphNode_Root>(Node))
		{
			Output = Root;
		}
		else
		{
			FBlueprintEditorUtils::RemoveNode(AnimBlueprint, Node, true);
		}
	}

	if (!ensureMsgf(Output, TEXT("AnimGraph of %s has no output node"), *AnimBlueprint->GetName()))
	{
		return false;
	}

	Skeleton->Modify();
	for (FName const SlotName : {BaseSlotName, LayerSlotName, FullBodySlotName, AdditiveSlotName})
	{
		Skeleton->SetSlotGroupName(SlotName, SlotName);
	}

	int32 const ColumnWidth = 320;
	int32 const RowHeight = 200;
	int32 const OutputX = Output->NodePosX;
	int32 const OutputY = Output->NodePosY;

	auto AddSlot = [AnimGraph](FName SlotName, int32 X, int32 Y)
	{
		FGraphNodeCreator<UAnimGraphNode_Slot> Creator(*AnimGraph);
		UAnimGraphNode_Slot* Slot = Creator.CreateNode(false);
		Slot->Node.SlotName = SlotName;
		Slot->NodePosX = X;
		Slot->NodePosY = Y;
		Creator.Finalize();
		return Slot;
	};

	UEdGraphSchema const* Schema = AnimGraph->GetSchema();
	auto Connect = [Schema](UEdGraphNode* From, UEdGraphNode* To, FName InputPinName)
	{
		UEdGraphPin* PosePin = From->FindPin(TEXT("Pose"), EGPD_Output);
		UEdGraphPin* InputPin = To->FindPin(InputPinName, EGPD_Input);
		return ensureMsgf(PosePin && InputPin && Schema->TryCreateConnection(PosePin, InputPin),
						  TEXT("Could not link %s into pin %s of %s"), *From->GetName(), *InputPinName.ToString(),
						  *To->GetName());
	};

	auto AddCache = [AnimGraph](FName SlotName, int32 X, int32 Y)
	{
		FGraphNodeCreator<UAnimGraphNode_SaveCachedPose> Creator(*AnimGraph);
		UAnimGraphNode_SaveCachedPose* Cache = Creator.CreateNode(false);
		Cache->CacheName = SlotName.ToString() + TEXT("Layer");
		Cache->NodePosX = X;
		Cache->NodePosY = Y;
		Creator.Finalize();
		return Cache;
	};

	auto UseCache = [AnimGraph](UAnimGraphNode_SaveCachedPose* Cache, int32 X, int32 Y)
	{
		FGraphNodeCreator<UAnimGraphNode_UseCachedPose> Creator(*AnimGraph);
		UAnimGraphNode_UseCachedPose* Use = Creator.CreateNode(false);
		Use->SaveCachedPoseNode = Cache;
		Use->NodePosX = X;
		Use->NodePosY = Y;
		Creator.Finalize();
		return Use;
	};

	UAnimGraphNode_SaveCachedPose* BaseCache =
		AddCache(BaseSlotName, OutputX - 5 * ColumnWidth, OutputY + 4 * RowHeight);
	UAnimGraphNode_SaveCachedPose* LayerCache =
		AddCache(LayerSlotName, OutputX - 5 * ColumnWidth, OutputY + 5 * RowHeight);

	FGraphNodeCreator<UAnimGraphNode_LayeredBoneBlend> BlendCreator(*AnimGraph);
	UAnimGraphNode_LayeredBoneBlend* Blend = BlendCreator.CreateNode(false);
	FBranchFilter LayerBranch;
	LayerBranch.BoneName = LayerBone;
	LayerBranch.BlendDepth = 0;
	Blend->Node.LayerSetup[0].BranchFilters.Add(LayerBranch);
	Blend->NodePosX = OutputX - 4 * ColumnWidth;
	Blend->NodePosY = OutputY + RowHeight;
	BlendCreator.Finalize();

	FGraphNodeCreator<UAnimGraphNode_TwoWayBlend> BaseWeightBlendCreator(*AnimGraph);
	UAnimGraphNode_TwoWayBlend* BaseWeightBlend = BaseWeightBlendCreator.CreateNode(false);
	BaseWeightBlend->NodePosX = OutputX - 3 * ColumnWidth;
	BaseWeightBlend->NodePosY = OutputY;
	BaseWeightBlendCreator.Finalize();

	FGraphNodeCreator<UK2Node_CallFunction> BaseWeightCreator(*AnimGraph);
	UK2Node_CallFunction* BaseWeight = BaseWeightCreator.CreateNode(false);
	BaseWeight->SetFromFunction(UAnimInstance::StaticClass()->FindFunctionByName(
		GET_FUNCTION_NAME_CHECKED(UAnimInstance, Blueprint_GetSlotMontageLocalWeight)));
	BaseWeight->NodePosX = OutputX - 4 * ColumnWidth;
	BaseWeight->NodePosY = OutputY - RowHeight;
	BaseWeightCreator.Finalize();

	FGraphNodeCreator<UAnimGraphNode_ApplyAdditive> ApplyCreator(*AnimGraph);
	UAnimGraphNode_ApplyAdditive* ApplyAdditive = ApplyCreator.CreateNode(false);
	ApplyAdditive->NodePosX = OutputX - ColumnWidth;
	ApplyAdditive->NodePosY = OutputY;
	ApplyCreator.Finalize();

	// An additive slot plays over the additive identity, which adds nothing while no montage is in it.
	FGraphNodeCreator<UAnimGraphNode_IdentityPose> IdentityCreator(*AnimGraph);
	UAnimGraphNode_IdentityPose* Identity = IdentityCreator.CreateNode(false);
	Identity->NodePosX = OutputX - 3 * ColumnWidth;
	Identity->NodePosY = OutputY + 2 * RowHeight;
	IdentityCreator.Finalize();

	UAnimGraphNode_Slot* FullBodySlot = AddSlot(FullBodySlotName, OutputX - 2 * ColumnWidth, OutputY);
	UAnimGraphNode_Slot* AdditiveSlot = AddSlot(AdditiveSlotName, OutputX - 2 * ColumnWidth, OutputY + 2 * RowHeight);
	UAnimGraphNode_Slot* BaseSlot = AddSlot(BaseSlotName, OutputX - 6 * ColumnWidth, OutputY + 4 * RowHeight);
	UAnimGraphNode_Slot* LayerSlot = AddSlot(LayerSlotName, OutputX - 6 * ColumnWidth, OutputY + 5 * RowHeight);
	// The idle keeps ticking under a montage, and the base slot's weight is read off its own last update.
	BaseSlot->Node.bAlwaysUpdateSourcePose = true;
	LayerSlot->Node.bAlwaysUpdateSourcePose = true;
	UAnimGraphNode_UseCachedPose* LayerSource =
		UseCache(BaseCache, OutputX - 7 * ColumnWidth, OutputY + 5 * RowHeight);
	UAnimGraphNode_UseCachedPose* BlendBase = UseCache(BaseCache, OutputX - 5 * ColumnWidth, OutputY + RowHeight);
	UAnimGraphNode_UseCachedPose* BlendBranch =
		UseCache(LayerCache, OutputX - 5 * ColumnWidth, OutputY + 2 * RowHeight);
	UAnimGraphNode_UseCachedPose* Unweighted = UseCache(LayerCache, OutputX - 4 * ColumnWidth, OutputY);

	// An unlinked slot source plays the reference pose, while a player left without a sequence fails the compile.
	bool bLinked = true;
	if (Idle)
	{
		FGraphNodeCreator<UAnimGraphNode_SequencePlayer> PlayerCreator(*AnimGraph);
		UAnimGraphNode_SequencePlayer* Player = PlayerCreator.CreateNode(false);
		Player->SetAnimationAsset(Idle);
		Player->NodePosX = OutputX - 7 * ColumnWidth;
		Player->NodePosY = OutputY + 4 * RowHeight;
		PlayerCreator.Finalize();
		bLinked = Connect(Player, BaseSlot, TEXT("Source"));
	}

	UEdGraphPin* const SlotNamePin = BaseWeight->FindPin(TEXT("SlotNodeName"), EGPD_Input);
	UEdGraphPin* const WeightPin = BaseWeight->GetReturnValuePin();
	UEdGraphPin* const AlphaPin = BaseWeightBlend->FindPin(TEXT("Alpha"), EGPD_Input);
	bLinked = bLinked
		&& ensureMsgf(SlotNamePin && WeightPin && AlphaPin && Schema->TryCreateConnection(WeightPin, AlphaPin),
					  TEXT("Could not drive the base weight blend from the %s slot weight"), *BaseSlotName.ToString());
	if (bLinked)
	{
		Schema->TrySetDefaultValue(*SlotNamePin, BaseSlotName.ToString());
	}

	bLinked = bLinked && Connect(BaseSlot, BaseCache, TEXT("Pose")) && Connect(LayerSource, LayerSlot, TEXT("Source"))
		&& Connect(LayerSlot, LayerCache, TEXT("Pose")) && Connect(BlendBase, Blend, TEXT("BasePose"))
		&& Connect(BlendBranch, Blend, TEXT("BlendPoses_0")) && Connect(Unweighted, BaseWeightBlend, TEXT("A"))
		&& Connect(Blend, BaseWeightBlend, TEXT("B")) && Connect(BaseWeightBlend, FullBodySlot, TEXT("Source"))
		&& Connect(FullBodySlot, ApplyAdditive, TEXT("Base")) && Connect(Identity, AdditiveSlot, TEXT("Source"))
		&& Connect(AdditiveSlot, ApplyAdditive, TEXT("Additive")) && Connect(ApplyAdditive, Output, TEXT("Result"));

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
	FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);
	UEditorLoadingAndSavingUtils::SavePackages({AnimBlueprint->GetPackage(), Skeleton->GetPackage()}, false);
	return bLinked && AnimBlueprint->Status != BS_Error;
}
