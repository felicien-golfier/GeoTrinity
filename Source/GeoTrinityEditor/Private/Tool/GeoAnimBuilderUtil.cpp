// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoAnimBuilderUtil.h"

#include "AnimGraphNode_ApplyAdditive.h"
#include "AnimGraphNode_IdentityPose.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_SaveCachedPose.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_UseCachedPose.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "EdGraphSchema_K2.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "FileHelpers.h"
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

	// The layer slot's pose is cached so it can both feed the base slot, which a base montage overrides, and the
	// layer branch, which it never does.
	FGraphNodeCreator<UAnimGraphNode_SaveCachedPose> SaveCreator(*AnimGraph);
	UAnimGraphNode_SaveCachedPose* LayerCache = SaveCreator.CreateNode(false);
	LayerCache->CacheName = LayerSlotName.ToString() + TEXT("Layer");
	LayerCache->NodePosX = OutputX - 3 * ColumnWidth;
	LayerCache->NodePosY = OutputY + 3 * RowHeight;
	SaveCreator.Finalize();

	auto UseLayerCache = [AnimGraph, LayerCache](int32 X, int32 Y)
	{
		FGraphNodeCreator<UAnimGraphNode_UseCachedPose> Creator(*AnimGraph);
		UAnimGraphNode_UseCachedPose* Use = Creator.CreateNode(false);
		Use->SaveCachedPoseNode = LayerCache;
		Use->NodePosX = X;
		Use->NodePosY = Y;
		Creator.Finalize();
		return Use;
	};

	FGraphNodeCreator<UAnimGraphNode_LayeredBoneBlend> BlendCreator(*AnimGraph);
	UAnimGraphNode_LayeredBoneBlend* Blend = BlendCreator.CreateNode(false);
	FBranchFilter LayerBranch;
	LayerBranch.BoneName = LayerBone;
	LayerBranch.BlendDepth = 0;
	Blend->Node.LayerSetup[0].BranchFilters.Add(LayerBranch);
	Blend->NodePosX = OutputX - 3 * ColumnWidth;
	Blend->NodePosY = OutputY;
	BlendCreator.Finalize();

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
	UAnimGraphNode_Slot* BaseSlot = AddSlot(BaseSlotName, OutputX - 4 * ColumnWidth, OutputY);
	UAnimGraphNode_Slot* LayerSlot = AddSlot(LayerSlotName, OutputX - 4 * ColumnWidth, OutputY + 3 * RowHeight);
	UAnimGraphNode_UseCachedPose* BaseSource = UseLayerCache(OutputX - 5 * ColumnWidth, OutputY);
	UAnimGraphNode_UseCachedPose* LayerBranchSource = UseLayerCache(OutputX - 4 * ColumnWidth, OutputY + RowHeight);

	// An unlinked slot source plays the reference pose, while a player left without a sequence fails the compile.
	bool bLinked = true;
	if (Idle)
	{
		FGraphNodeCreator<UAnimGraphNode_SequencePlayer> PlayerCreator(*AnimGraph);
		UAnimGraphNode_SequencePlayer* Player = PlayerCreator.CreateNode(false);
		Player->SetAnimationAsset(Idle);
		Player->NodePosX = OutputX - 5 * ColumnWidth;
		Player->NodePosY = OutputY + 3 * RowHeight;
		PlayerCreator.Finalize();
		bLinked = Connect(Player, LayerSlot, TEXT("Source"));
	}

	bLinked = bLinked && Connect(LayerSlot, LayerCache, TEXT("Pose")) && Connect(BaseSource, BaseSlot, TEXT("Source"))
		&& Connect(BaseSlot, Blend, TEXT("BasePose")) && Connect(LayerBranchSource, Blend, TEXT("BlendPoses_0"))
		&& Connect(Blend, FullBodySlot, TEXT("Source")) && Connect(FullBodySlot, ApplyAdditive, TEXT("Base"))
		&& Connect(Identity, AdditiveSlot, TEXT("Source")) && Connect(AdditiveSlot, ApplyAdditive, TEXT("Additive"))
		&& Connect(ApplyAdditive, Output, TEXT("Result"));

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
	FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);
	UEditorLoadingAndSavingUtils::SavePackages({AnimBlueprint->GetPackage(), Skeleton->GetPackage()}, false);
	return bLinked && AnimBlueprint->Status != BS_Error;
}
