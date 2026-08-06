// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

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
	inline FName const Radius(TEXT("User.Radius"));

	/** NS_GeoTrinity_Projectile01 — AGeoProjectile::BulletVFX. */
	inline FName const BulletRadius(TEXT("User.Bullet_Radius"));
	inline FName const BulletHeadColor(TEXT("User.Bullet_HeadColor"));
	inline FName const BulletTrailColor(TEXT("User.Bullet_TrailColor"));
	inline FName const TrailLifetimeScale(TEXT("User.Trail_LifetimeScale"));

	/** Beam systems — UGeoBeamVFXComponent, UBeamPattern. */
	inline FName const BeamWidth(TEXT("User.Beam_Width"));
	inline FName const BeamLength(TEXT("User.Beam_Length"));

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

/** Blueprint-callable access to the GeoNiagaraParams::* names, since Blueprint cannot see a C++ namespace. */
UCLASS()
class GEOTRINITY_API UGeoNiagaraParamsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns the Niagara user parameter name for system or actor lifetime. */
	UFUNCTION(BlueprintPure, Category = "Niagara Params")
	static FName GetLifetime() { return GeoNiagaraParams::Lifetime; }

	/** Returns the Niagara user parameter name for a generic color tint. */
	UFUNCTION(BlueprintPure, Category = "Niagara Params")
	static FName GetColor() { return GeoNiagaraParams::Color; }

	/** Returns the Niagara user parameter name for a generic radius. */
	UFUNCTION(BlueprintPure, Category = "Niagara Params")
	static FName GetRadius() { return GeoNiagaraParams::Radius; }

	/** Returns the Niagara user parameter name for the projectile bullet radius (NS_GeoTrinity_Projectile01). */
	UFUNCTION(BlueprintPure, Category = "Niagara Params")
	static FName GetBulletRadius() { return GeoNiagaraParams::BulletRadius; }

	/** Returns the Niagara user parameter name for the projectile bullet head color. */
	UFUNCTION(BlueprintPure, Category = "Niagara Params")
	static FName GetBulletHeadColor() { return GeoNiagaraParams::BulletHeadColor; }

	/** Returns the Niagara user parameter name for the projectile bullet trail color. */
	UFUNCTION(BlueprintPure, Category = "Niagara Params")
	static FName GetBulletTrailColor() { return GeoNiagaraParams::BulletTrailColor; }

	/** Returns the Niagara user parameter name for the projectile trail lifetime scale. */
	UFUNCTION(BlueprintPure, Category = "Niagara Params")
	static FName GetTrailLifetimeScale() { return GeoNiagaraParams::TrailLifetimeScale; }

	/** Returns the Niagara user parameter name for beam visual width (UGeoBeamVFXComponent, UBeamPattern). */
	UFUNCTION(BlueprintPure, Category = "Niagara Params")
	static FName GetBeamWidth() { return GeoNiagaraParams::BeamWidth; }

	/** Returns the Niagara user parameter name for beam visual length (UGeoBeamVFXComponent, UBeamPattern). */
	UFUNCTION(BlueprintPure, Category = "Niagara Params")
	static FName GetBeamLength() { return GeoNiagaraParams::BeamLength; }

	/** Returns the Niagara user parameter name for the devastating-wave AOE outer radius. */
	UFUNCTION(BlueprintPure, Category = "Niagara Params")
	static FName GetAOERadius() { return GeoNiagaraParams::AOERadius; }

	/** Returns the Niagara user parameter name for the devastating-wave AOE grow animation duration. */
	UFUNCTION(BlueprintPure, Category = "Niagara Params")
	static FName GetAOEGrowDuration() { return GeoNiagaraParams::AOEGrowDuration; }

	/** Returns the Niagara user parameter name for the devastating-wave AOE color tint. */
	UFUNCTION(BlueprintPure, Category = "Niagara Params")
	static FName GetAOEColor() { return GeoNiagaraParams::AOEColor; }

	/** Returns the Niagara user parameter name for the devastating-wave AOE inner annulus radius. */
	UFUNCTION(BlueprintPure, Category = "Niagara Params")
	static FName GetAnnulusRadius() { return GeoNiagaraParams::AnnulusRadius; }

	/** Returns the Niagara user parameter name for the devastating-wave AOE fade-out duration. */
	UFUNCTION(BlueprintPure, Category = "Niagara Params")
	static FName GetFadeOutDuration() { return GeoNiagaraParams::FadeOutDuration; }
};
