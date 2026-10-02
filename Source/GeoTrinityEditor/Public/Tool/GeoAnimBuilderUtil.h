// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"

#include "GeoAnimBuilderUtil.generated.h"

class UAnimBlueprint;
class UAnimMontage;
class UAnimSequence;
class USkeletalMesh;
class USkeleton;
class UStaticMesh;

/**
 * Generic animation-authoring primitives for Python/Blueprint automation.
 *
 * These exist because a montage's section layout is unreachable from Python: UAnimMontage::CompositeSections and
 * ::SlotAnimTracks are bare UPROPERTY() (invisible to get/set_editor_property) and FAnimLinkableElement::Link — which
 * keeps a section's cached segment/link data consistent — is C++ only. UAnimMontage exposes no section UFUNCTION
 * (FAnimMontageInstance::SetNextSectionName is runtime playback, not asset editing).
 *
 * A montage's slot track alone is reachable without this class, via UAnimMontageFactory::SourceAnimation at creation.
 *
 * Keep this class free of per-asset functions: it operates on any asset from caller-supplied arguments.
 */
UCLASS()
class GEOTRINITYEDITOR_API UGeoAnimBuilderUtil : public UEditorUtilityObject
{
	GENERATED_BODY()

public:
	/**
	 * Generic: replaces Montage's slot tracks with a single SlotName track holding one segment that plays Sequence
	 * from its start for PlayLength seconds, or the whole of it at 0, and resizes the montage to that. Existing
	 * sections are cleared, since their cached links point into the old track — call SetMontageSections afterwards.
	 * Saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void SetMontageSlotSegment(UAnimMontage* Montage, UAnimSequence* Sequence,
									  FName SlotName = TEXT("DefaultSlot"), float PlayLength = 0.f);

	/**
	 * Generic: rebuilds Montage's sections from three parallel arrays, each entry linked at its StartTimes value so
	 * the section's cached segment/link data stays consistent. A NextSectionNames entry of None ends the chain (the
	 * montage stops there); naming the section itself loops it. Requires a slot track to already exist. Saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void SetMontageSections(UAnimMontage* Montage, TArray<FName> SectionNames, TArray<float> StartTimes,
								   TArray<FName> NextSectionNames);

	/** Generic: Montage's sections as the three parallel arrays SetMontageSections takes, in the montage's order. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void GetMontageSections(UAnimMontage const* Montage, TArray<FName>& OutSectionNames,
								   TArray<float>& OutStartTimes, TArray<FName>& OutNextSectionNames);

	/** Logs Montage's slot tracks, segments and sections to LogTemp — Python can read neither array. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void InspectMontage(UAnimMontage* Montage);

	/**
	 * Generic: Mesh's LOD 0 vertex positions, indexed exactly as USkinWeightModifier indexes its weights (both walk
	 * the FMeshDescription from USkeletalMesh::CloneMeshDescription). Lets a caller pair a vertex's skin weights with
	 * where that vertex actually sits — Python reaches the weights but has no route to a skeletal mesh's geometry.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static TArray<FVector> GetSkeletalMeshVertexPositions(USkeletalMesh* Mesh);

	/**
	 * Generic: rebuilds Mesh as a skinned copy of StaticMesh over the bone hierarchy the three parallel arrays
	 * describe, rebuilds Skeleton to match it, and pairs the two assets up. Saves both.
	 *
	 * Bones are listed parents before children, the first being the root, and each transform is in its parent's
	 * space (ParentNames[0] is ignored). Every vertex lands rigidly on the root — move them onto the bones that
	 * should drive them with USkinWeightModifier afterwards, which is reachable from Python.
	 *
	 * This is the editor's "convert static mesh to skeletal mesh" minus its dialog: neither of the factories behind
	 * that command can be driven from Python, one taking its reference skeleton through a plain C++ member and the
	 * other through bare UPROPERTY()s that get/set_editor_property cannot see. The hierarchy is built here rather
	 * than added afterwards because the conversion takes the reference skeleton to build against, so no later pass
	 * has to reconcile a mesh against a skeleton it was not built on.
	 *
	 * Both assets are rebuilt in place, which a re-run does wholesale — that invalidates any animation authored
	 * against the previous bone list, and beats deleting packages the session could no longer load.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static bool RebuildSkeletalMeshFromStaticMesh(USkeletalMesh* Mesh, UStaticMesh* StaticMesh, USkeleton* Skeleton,
												  TArray<FName> BoneNames, TArray<FName> ParentNames,
												  TArray<FTransform> Transforms);

	/**
	 * Generic: rebuilds AnimBlueprint's AnimGraph as two animation layers split at LayerBone, with an additive on top:
	 *
	 *   Idle -> Slot BaseSlotName -> base cache -> Slot LayerSlotName -> layer cache
	 *   Layered blend: base cache, layer cache on branch LayerBone at depth 0
	 *   Two-way blend: layer cache -> layered blend by BaseSlotName's montage weight -> Slot FullBodySlotName
	 *   Additive identity -> Slot AdditiveSlotName, applied as an additive onto the full-body slot -> Output
	 *
	 * A montage in either slot moves the whole rig over the idle. Where both play, LayerBone's branch follows the
	 * layer montage and everything outside it the base montage — a channel holds the body while what fires over it
	 * takes the parts. A montage in FullBodySlotName moves the whole rig over both; an additive montage in
	 * AdditiveSlotName adds onto whatever the others play. Each slot is registered on the target skeleton in a slot
	 * group named after it, since playing a montage stops every other montage of its group. A null idle plays the
	 * reference pose.
	 *
	 * Every node but the output is replaced, so a re-run rebuilds the graph. Compiles and saves the asset and its
	 * skeleton; returns false when the compile fails.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static bool BuildLayeredAnimGraph(UAnimBlueprint* AnimBlueprint, FName LayerBone, FName BaseSlotName,
									  FName LayerSlotName, FName FullBodySlotName, FName AdditiveSlotName,
									  UAnimSequence* Idle);
};
