// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once
#include "UGeoGameplayLibrary.generated.h"


class AGeoCharacter;
struct FGameplayTag;
// Should be used as default value to spawn projectiles / characters etc... Also should be Playable Character's half
// capsule height.
constexpr float ArbitraryCharacterZ = 50.0f;
constexpr float Sqrt3 = 1.7320508f;


class AEnemyCharacter;
class APlayableCharacter;
enum class EGeoColor : uint8;
class AGeoProjectile;
class APawn;
class APlayerController;
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
	static FColor GetRandomColor();
	/** Returns a deterministic debug color for WorldContextObject based on its object hash. */
	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary", meta = (DefaultToSelf = "WorldContextObject"))
	static FColor GetColorForObject(UObject const* WorldContextObject);

	/**
	 * Returns the Game Data Settings palette color of Color — the same lookup the outline post-process does through the
	 * palette texture, for Blueprints (Gameplay Cues) that receive a slot as a number. A byte pin auto-casts to the
	 * Color pin, so an index carried in cue parameters plugs straight in.
	 *
	 * @param Alpha  Alpha to apply to the returned color. Negative keeps the palette color's own alpha unchanged.
	 */
	UFUNCTION(BlueprintPure, Category = "GameplayLibrary")
	static FLinearColor GetPaletteColorFromIndex(int ColorIndex, float Alpha = -1.f);

	/**
	 * Returns the Game Data Settings palette color for the given semantic color slot.
	 *
	 * @param Alpha  Alpha to apply to the returned color. Negative keeps the palette color's own alpha unchanged.
	 */
	UFUNCTION(BlueprintPure, Category = "GameplayLibrary")
	static FLinearColor GetPaletteColor(EGeoColor Color, float Alpha = -1.f);

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
	/** Returns true when World is running as a dedicated server (no local viewport). */
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
	 * Returns true only for the local player that owns the keyboard and mouse, which is always local player 0.
	 * There is exactly one mouse, so use this — NOT `IsLocalPlayerAvatar` — to gate anything reading it (aim, cursor,
	 * keyboard-layout rebinds). Every other couch-coop player is gamepad-only.
	 */
	static bool IsKeyboardMousePlayer(APlayerController const* PlayerController);

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
	/** Returns the estimated one-way network latency in seconds for the world's first local player controller. */
	static float GetOnWayPingSec(UWorld const* World);
	/** Returns the current server world time in seconds. @see GetServerTime(UObject*) for parameter and warning
	 * details. */
	static float GetServerTime(UWorld const* World, bool bUpdatedWithPing = false);

	/**
	 * Returns the AGeoTargetPoint actors carrying both halves of the point's identity: PurposeTag (TargetPoint.*,
	 * what the point is for) and ArenaTag (Arena.*, the encounter it belongs to). A point can carry several of
	 * either, so the same actor can serve two purposes or two neighbouring arenas.
	 */
	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary", meta = (DefaultToSelf = "WorldContextObject"))
	static TArray<AActor*> GetTargetPoints(UObject const* WorldContextObject, FGameplayTag const PurposeTag,
										   FGameplayTag const ArenaTag);

	/**
	 * Server. Teleports player pawns to the AGeoTargetPoints carrying PurposeTag + ArenaTag (round-robin). By default
	 * it moves everyone — the group respawn, which always teleports. Pass bSkipPlayersInArenaVolume to leave behind any
	 * pawn already standing in an AGeoArenaVolume carrying ArenaTag: the arena's fight-commit move does not drag in
	 * players who walked to their place themselves.
	 */
	static void TeleportPlayersToTargetPoints(UObject const* WorldContextObject, FGameplayTag PurposeTag,
											  FGameplayTag ArenaTag, bool bSkipPlayersInArenaVolume = false);

	/** Returns every currently-alive APlayableCharacter, found by iterating player controllers' pawns. */
	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary", meta = (DefaultToSelf = "WorldContextObject"))
	static TArray<APlayableCharacter*> GetAlivePlayers(UObject const* WorldContextObject);
};

using GeoLib = UGeoGameplayLibrary;
