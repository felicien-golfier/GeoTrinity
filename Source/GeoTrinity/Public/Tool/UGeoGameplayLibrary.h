// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once
#include "UGeoGameplayLibrary.generated.h"


class AGeoCharacter;
struct FGameplayTag;
// Should be used as default value to spawn projectiles / characters etc... Also should be Playable Character's half
// capsule height.
constexpr float ArbitraryCharacterZ = 50.0f;


class AEnemyCharacter;
class AGeoProjectile;
class APawn;
class UCameraShakeBase;
struct FAbilityPayload;
struct FEffectData;

static FColor const ColorPalette[] = {
	FColor::Black,		  FColor::Red,	   FColor::Green,  FColor::Blue,	   FColor::Yellow,
	FColor::Cyan,		  FColor::Magenta, FColor::Orange, FColor::Emerald,	   FColor::Purple,
	FColor::Turquoise,	  FColor::Silver,  FColor::White,  FColor(75, 0, 130), // Indigo
	FColor(255, 20, 147), // Pink
	FColor(0, 128, 128), // Teal
	FColor(220, 20, 60), // Crimson
	FColor(191, 255, 0), // Lime
	FColor(139, 69, 19), // Brown
	FColor(0, 0, 128), // Navy
};

/** General-purpose Blueprint function library for GeoTrinity. Provides server detection, camera shake, and
 *  network-time utilities used across abilities, projectiles, and UI. */
UCLASS()
class UGeoGameplayLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns a randomly selected color from the static debug ColorPalette array. */
	static FColor GetRandomColorFromPalette();
	/** Returns a deterministic debug color for WorldContextObject based on its object hash. */
	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary", meta = (DefaultToSelf = "WorldContextObject"))
	static FColor GetColorForObject(UObject const* WorldContextObject);

	/** Returns true when running with authority (listen server or dedicated server). */
	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary", meta = (DefaultToSelf = "WorldContextObject"))
	static bool IsServer(UObject const* WorldContextObject);

	/** Triggers a camera shake on the local player controller. Safe to call from any machine — no-op on dedicated
	 * server. */
	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary", meta = (DefaultToSelf = "WorldContextObject"))
	static void TriggerCameraShake(UObject const* WorldContextObject, TSubclassOf<UCameraShakeBase> ShakeClass,
								   float Scale = 1.f);

	/** Returns true when World is running with authority. */
	static bool IsServer(UWorld const* World);

	/**
	 * Returns true only on a dedicated server (a machine with no local player / viewport).
	 * Use this to gate cosmetic-only work (montages, local Gameplay Cues, VFX): `if (!IsDedicatedServer(...))`.
	 * Do NOT use `!IsServer()` for visuals — that wrongly skips the listen-server host, which IS a rendering player.
	 */
	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary", meta = (DefaultToSelf = "WorldContextObject"))
	static bool IsDedicatedServer(UObject const* WorldContextObject);
	static bool IsDedicatedServer(UWorld const* World);
	/** Non-pawn overload of IsLocalPlayerAvatar; casts Actor to APawn before applying the same check. Returns false for
	 * non-pawn actors. */
	static bool IsLocalPlayerAvatar(AActor const* Actor);

	/**
	 * Returns true only for the viewing human player's own avatar on this machine.
	 * Use this — NOT `IsLocallyControlled()` — to gate "my own pawn" cosmetics (hide own floating bar, local-player hit
	 * flash). On a listen server the host's AI pawns are also locally controlled, so `IsLocallyControlled()` alone is
	 * true for every host enemy; the extra `IsPlayerControlled()` term excludes them.
	 */
	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary")
	static bool IsLocalPlayerAvatar(APawn const* Pawn);

	/**
	 * Returns Owner cast to AGeoCharacter; if Owner is a PlayerState, resolves and returns its pawn instead.
	 * Returns nullptr when neither cast succeeds (e.g. non-character owner actor).
	 */
	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary")
	static AGeoCharacter* GetCharacterFromOwner(AActor* Owner);

	/**
	 * Returns the current server world time in seconds.
	 * Use only for network synchronization (e.g. projectile spawn times) — not for local timing.
	 * For local delta-time measurements on the client, use GetWorld()->GetTimeSeconds() instead.
	 *
	 * @param bUpdatedWithPing  When false (default), returns the raw replicated server time. When true, subtracts
	 *                          half the estimated round-trip ping so the result approximates the server's "current"
	 *                          time as seen by the client — useful for scheduling client-side predicted events.
	 *
	 * @warning  Do NOT use this for measuring local durations (e.g. charge time, cooldown UI). The value is
	 *           a network approximation and can drift or jump. Use GetWorld()->GetTimeSeconds() for local timing.
	 */
	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary", meta = (DefaultToSelf = "WorldContextObject"))
	static float GetServerTime(UObject const* WorldContextObject, bool bUpdatedWithPing = false);
	static float GetServerTime(UWorld const* World, bool bUpdatedWithPing = false);

	/** Returns all AGeoTargetPoint actors in the world whose tags include LocationTag. */
	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary", meta = (DefaultToSelf = "WorldContextObject"))
	static TArray<AActor*> GetTargetPoints(UObject const* WorldContextObject, FGameplayTag const LocationTag);
};

using GeoLib = UGeoGameplayLibrary;
