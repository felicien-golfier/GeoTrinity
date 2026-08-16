// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"

#include "GeoAnimBuilderUtil.generated.h"

class UAnimMontage;
class UAnimSequence;
class USkeletalMesh;

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
	 * Generic: replaces Montage's slot tracks with a single SlotName track holding one segment that plays the whole
	 * of Sequence, and resizes the montage to the sequence length. Existing sections are cleared, since their cached
	 * links point into the old track — call SetMontageSections afterwards. Saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void SetMontageSlotSegment(UAnimMontage* Montage, UAnimSequence* Sequence,
									  FName SlotName = TEXT("DefaultSlot"));

	/**
	 * Generic: rebuilds Montage's sections from three parallel arrays, each entry linked at its StartTimes value so
	 * the section's cached segment/link data stays consistent. A NextSectionNames entry of None ends the chain (the
	 * montage stops there); naming the section itself loops it. Requires a slot track to already exist. Saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void SetMontageSections(UAnimMontage* Montage, TArray<FName> SectionNames, TArray<float> StartTimes,
								   TArray<FName> NextSectionNames);

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
};
