// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Tool/GeoColor.h"

#include "GeoNiagaraParams.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/**
 * Every Niagara User parameter name written from C++, declared once. A name here must match the User Parameter authored
 * in the Niagara system — nothing validates it at compile or load time, so a mismatch is silent: the system just keeps
 * its authored default.
 *
 * FName rather than constexpr literals because the setters take FName (it has no constexpr constructor) and declaring
 * them once keeps the hashing out of the per-spawn path. The "User." prefix is optional at the call site
 * (FNiagaraUserRedirectionParameterStore redirects short names to the fully qualified ones), but every name here is
 * fully qualified so it reads exactly like the parameter shown in the Niagara editor.
 */
namespace GeoNiagaraParams
{
	inline FName const Lifetime(TEXT("User.Lifetime"));
	inline FName const Color(TEXT("User.Color"));
	inline FName const Position(TEXT("User.Position"));
	inline FName const Radius(TEXT("User.Radius"));

	/** How strong the effect showing it is, on a 0..1 scale — FGeoVFXParams resolves it from an attribute or an ability
	 * level through its own curve, so a system reads one normalized number and never what drives it. */
	inline FName const NormalizedMagnitude(TEXT("User.NormalizedMagnitude"));

	/** NS_GeoTrinity_Projectile01 — AGeoProjectile::BulletVFX. */
	inline FName const BulletRadius(TEXT("User.Bullet_Radius"));
	inline FName const BulletHeadColor(TEXT("User.Bullet_HeadColor"));
	inline FName const BulletTrailColor(TEXT("User.Bullet_TrailColor"));
	inline FName const TrailLifetimeScale(TEXT("User.Trail_LifetimeScale"));

	/** Beam systems — UGeoBeamVFXComponent, UBeamPattern. */
	inline FName const BeamWidth(TEXT("User.Beam_Width"));
	inline FName const BeamLength(TEXT("User.Beam_Length"));

	/** Systems drawing a colour pattern through MF_MeaningColors — the beam and the zone telegraphs: one colour per
	 * meaning, Color first, bound to the renderer's material by AI/Python/Niagara/meaning_color_bindings.py. */
	inline FName const MeaningColors[] = {Color, FName(TEXT("User.Color2")), FName(TEXT("User.Color3")),
										  FName(TEXT("User.Color4"))};
	static_assert(UE_ARRAY_COUNT(MeaningColors) == GeoColor::MaxMeaningColorCount);
	/** How many of MeaningColors the pattern cycles through. */
	inline FName const ColorCount(TEXT("User.ColorCount"));

	/** Writes Colors, as GeoColor::GetMeaningColors resolves them, into MeaningColors and ColorCount. */
	void SetMeaningColors(UNiagaraComponent* Component, TArray<FLinearColor> const& Colors);

	/** Devastating wave AOE and its telegraph — UDevastatingWavePattern. */
	inline FName const AOERadius(TEXT("User.AOE_Radius"));
	inline FName const AOEGrowDuration(TEXT("User.AOE_GrowDuration"));
	inline FName const AOEColor(TEXT("User.AOE_Color"));
	inline FName const AnnulusRadius(TEXT("User.AnnulusRadius"));
	inline FName const FadeOutDuration(TEXT("User.FadeOut_Duration"));

	/** A beam's live asset plus its optional windup-preview asset (the shared Ray Zone Indicator niagara), bundled so
	 * ApplySwappableAsset callers pass one thing instead of two. Plain aggregate, not a UPROPERTY struct — each owner
	 * (UGeoBeamVFXComponent, UBeamPattern) keeps its own authored fields (different replication needs) and just
	 * assembles one of these at the call site. */
	struct FBeamVfxAssetSet
	{
		UNiagaraSystem* BeamSystem = nullptr;
		UNiagaraSystem* PreviewSystem = nullptr;

		/** Returns PreviewSystem when bWantPreview is true and a preview asset is set; otherwise returns BeamSystem. */
		UNiagaraSystem* GetDesiredAsset(bool const bWantPreview) const
		{
			return (bWantPreview && PreviewSystem) ? PreviewSystem : BeamSystem;
		}
	};

	/** Reassigns Component's asset to Assets.GetDesiredAsset(bWantIndicator) only when it differs — SetAsset resets the
	 * system, so skipping the no-op case avoids restarting an already-correct beam. No-op if Component or the desired
	 * asset is null. Shared by UGeoBeamVFXComponent and UBeamPattern's identical preview<->beam asset handoff. */
	void ApplySwappableAsset(UNiagaraComponent* Component, FBeamVfxAssetSet const& Assets, bool bWantIndicator);
} // namespace GeoNiagaraParams

/**
 * Every material, material parameter collection and custom primitive data name written from C++, declared once for the
 * same reason as GeoNiagaraParams: a name the material lacks is silently ignored and it keeps its default. Custom
 * primitive data is addressed by index rather than name, so its slots below double as the registry keeping two features
 * off one slot — nothing else warns when they collide.
 */
namespace GeoMaterialParams
{
	/** Custom primitive data: the fraction of a deployable's life spent, 0..1 — AGeoDeployableBase, read by
	 * M_PulseCircle's DurationSpent. Spent rather than remaining, so a primitive nobody writes reads as untouched. */
	int32 constexpr DurationSpentPrimitiveDataIndex = 0;
	/** Custom primitive data, two slots (1 and 2): an arena's XY on its floors — AGeoArena, read by the floor looks
	 * centred on it through their ArenaCenter parameter. */
	int32 constexpr ArenaCenterPrimitiveDataIndex = 1;

	/** M_PulseCircle — AGeoEffectZone. Built by AI/Python/Material/make_pulse_circle_material.py. */
	inline FName const ZoneOutlineColor(TEXT("OutlineColor"));
	/** The fill colours, one per meaning, in order. */
	inline FName const ZoneInsideColors[] = {TEXT("InsideColor"), TEXT("InsideColor2"), TEXT("InsideColor3"),
											 TEXT("InsideColor4")};
	static_assert(UE_ARRAY_COUNT(ZoneInsideColors) == GeoColor::MaxMeaningColorCount);
	/** How many fill colours the zone's colour pattern cycles through. */
	inline FName const ZoneColorCount(TEXT("ColorCount"));

	/** Buff pickup mesh — AGeoBuffPickup's buff colour. */
	inline FName const BuffPickupColor(TEXT("Color"));

	/** Character body material 0 — UShieldBurstPassiveComponent: the gauge fill, then its full-face flash, both 0..1.
	 * Built by AI/Python/Material/make_class_badge_materials.py. */
	inline FName const ShieldBurstGauge(TEXT("GlowGauge"));
	inline FName const ShieldBurstFullGauge(TEXT("FullGlowGauge"));

	/** Deployable outline post-process — AGeoGameCamera: the palette texture (GeoColor::CreatePaletteTexture) and its
	 * texel count. Built by AI/Python/Material/make_deployable_outline_material.py. */
	inline FName const OutlinePalette(TEXT("Palette"));
	inline FName const OutlinePaletteSize(TEXT("PaletteSize"));

	/** AGeoGameCamera::CameraParameters collection: the camera's XY and zoom, read by the backdrop layers. */
	inline FName const CameraXY(TEXT("CameraXY"));
	inline FName const CameraZoomRatio(TEXT("ZoomRatio"));

	/** MPC_BackgroundPulse — UGeoBackgroundPulseComponent: one (OriginX, OriginY, Radius, Intensity) per pulse slot. */
	inline FName GetPulseSourceName(int32 const SlotIndex)
	{
		return FName(FString::Printf(TEXT("PulseSource_%02d"), SlotIndex));
	}

	/** MPC_MaskedArea — UDevastatingWavePattern: the pillars shadowing the wave, one position per slot, one radius. */
	inline FName GetPillarPositionName(int32 const SlotIndex)
	{
		return FName(FString::Printf(TEXT("PillarPosWS_%02d"), SlotIndex));
	}
	inline FName const PillarRadius(TEXT("Pillar_Radius"));

	/** HUD sweep materials, 0..1 — the cooldown (UGeoAbilitySlotWidget) and the effect depletion (UGeoStatusBarWidget). */
	inline FName const SweepFill(TEXT("Fill"));
} // namespace GeoMaterialParams

/** Blueprint-callable access to the GeoNiagaraParams::* names, since Blueprint cannot see a C++ namespace. */
UCLASS()
class GEOTRINITY_API UGeoNiagaraParamsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns the Niagara user parameter name for system or actor lifetime. */
	UFUNCTION(BlueprintPure, Category = "GeoNiagaraParams")
	static FName GetLifetimeParameterName() { return GeoNiagaraParams::Lifetime; }

	/** Returns the Niagara user parameter name for a generic color tint. */
	UFUNCTION(BlueprintPure, Category = "GeoNiagaraParams")
	static FName GetColorParameterName() { return GeoNiagaraParams::Color; }

	/** Returns the Niagara user parameter name for a generic world-space position (origin). */
	UFUNCTION(BlueprintPure, Category = "GeoNiagaraParams")
	static FName GetPositionParameterName() { return GeoNiagaraParams::Position; }

	/** Returns the Niagara user parameter name for a generic radius. */
	UFUNCTION(BlueprintPure, Category = "GeoNiagaraParams")
	static FName GetRadiusParameterName() { return GeoNiagaraParams::Radius; }

	/** Returns the Niagara user parameter name for the 0..1 strength of the effect showing it. */
	UFUNCTION(BlueprintPure, Category = "GeoNiagaraParams")
	static FName GetNormalizedMagnitudeParameterName() { return GeoNiagaraParams::NormalizedMagnitude; }

	/** Returns the Niagara user parameter name for the projectile bullet radius (NS_GeoTrinity_Projectile01). */
	UFUNCTION(BlueprintPure, Category = "GeoNiagaraParams")
	static FName GetBulletRadiusParameterName() { return GeoNiagaraParams::BulletRadius; }

	/** Returns the Niagara user parameter name for the projectile bullet head color. */
	UFUNCTION(BlueprintPure, Category = "GeoNiagaraParams")
	static FName GetBulletHeadColorParameterName() { return GeoNiagaraParams::BulletHeadColor; }

	/** Returns the Niagara user parameter name for the projectile bullet trail color. */
	UFUNCTION(BlueprintPure, Category = "GeoNiagaraParams")
	static FName GetBulletTrailColorParameterName() { return GeoNiagaraParams::BulletTrailColor; }

	/** Returns the Niagara user parameter name for the projectile trail lifetime scale. */
	UFUNCTION(BlueprintPure, Category = "GeoNiagaraParams")
	static FName GetTrailLifetimeScaleParameterName() { return GeoNiagaraParams::TrailLifetimeScale; }

	/** Returns the Niagara user parameter name for beam visual width (UGeoBeamVFXComponent, UBeamPattern). */
	UFUNCTION(BlueprintPure, Category = "GeoNiagaraParams")
	static FName GetBeamWidthParameterName() { return GeoNiagaraParams::BeamWidth; }

	/** Returns the Niagara user parameter name for beam visual length (UGeoBeamVFXComponent, UBeamPattern). */
	UFUNCTION(BlueprintPure, Category = "GeoNiagaraParams")
	static FName GetBeamLengthParameterName() { return GeoNiagaraParams::BeamLength; }

	/** Returns the Niagara user parameter name for the devastating-wave AOE outer radius. */
	UFUNCTION(BlueprintPure, Category = "GeoNiagaraParams")
	static FName GetAOERadiusParameterName() { return GeoNiagaraParams::AOERadius; }

	/** Returns the Niagara user parameter name for the devastating-wave AOE grow animation duration. */
	UFUNCTION(BlueprintPure, Category = "GeoNiagaraParams")
	static FName GetAOEGrowDurationParameterName() { return GeoNiagaraParams::AOEGrowDuration; }

	/** Returns the Niagara user parameter name for the devastating-wave AOE color tint. */
	UFUNCTION(BlueprintPure, Category = "GeoNiagaraParams")
	static FName GetAOEColorParameterName() { return GeoNiagaraParams::AOEColor; }

	/** Returns the Niagara user parameter name for the devastating-wave AOE inner annulus radius. */
	UFUNCTION(BlueprintPure, Category = "GeoNiagaraParams")
	static FName GetAnnulusRadiusParameterName() { return GeoNiagaraParams::AnnulusRadius; }

	/** Returns the Niagara user parameter name for the devastating-wave AOE fade-out duration. */
	UFUNCTION(BlueprintPure, Category = "GeoNiagaraParams")
	static FName GetFadeOutDurationParameterName() { return GeoNiagaraParams::FadeOutDuration; }
};
