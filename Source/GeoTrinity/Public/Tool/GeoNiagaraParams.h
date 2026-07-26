// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

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
	/** NS_GeoTrinity_Projectile01 — AGeoProjectile::BulletVFX. */
	inline FName const BulletRadius(TEXT("User.Bullet_Radius"));
	inline FName const BulletHeadColor(TEXT("User.Bullet_HeadColor"));
	inline FName const BulletTrailColor(TEXT("User.Bullet_TrailColor"));
	inline FName const TrailLifetimeScale(TEXT("User.Trail_LifetimeScale"));

	/** Beam systems — UGeoBeamVFXComponent, UBeamPattern. */
	inline FName const BeamWidth(TEXT("User.Beam_Width"));
	inline FName const BeamLength(TEXT("User.Beam_Length"));
	inline FName const BeamColor(TEXT("User.Color"));

	/** Devastating wave AOE and its telegraph — UDevastatingWavePattern. */
	inline FName const AOERadius(TEXT("User.AOE_Radius"));
	inline FName const AOEGrowDuration(TEXT("User.AOE_GrowDuration"));
	inline FName const AOEColor(TEXT("User.AOE_Color"));
	inline FName const AnnulusRadius(TEXT("User.AnnulusRadius"));
	inline FName const FadeOutDuration(TEXT("User.FadeOut_Duration"));
} // namespace GeoNiagaraParams
