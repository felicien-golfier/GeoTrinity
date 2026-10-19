// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "GeoMessageSubsystem.generated.h"

enum class EGeoColor : uint8;

/** One message of the log: what it says, in which colour, and when it was posted (FPlatformTime::Seconds). */
struct FGeoMessage
{
	FText Text;
	EGeoColor Color;
	double PostTime = 0.;
};

/** How long messages live and how many show at once; the log widget's tunables of the same names. */
struct FGeoMessageLayoutParams
{
	int32 MaxLines = 5;
	/** Seconds a message is shown before it fades out. */
	float Lifetime = 4.f;
	/** Seconds of a fade in or out. */
	float FadeTime = .4f;
	/** Seconds a line takes to ease into a freed slot. */
	float SlideTime = .25f;
};

/** Where and how visible one message is: Slot 0 is the bottom line, a fraction while it eases between two. */
struct FGeoMessageLine
{
	int32 MessageIndex = 0;
	float Slot = 0.f;
	float Alpha = 0.f;
};

DECLARE_MULTICAST_DELEGATE(FOnGeoMessagePosted);

/**
 * Owns the live messages of the UI (UGeoMessageLog widgets only paint them), so changing page never drops one.
 * Message state is a function of post times: Layout places every line from them, so a log shown mid-life paints the
 * same frame as one that was there from the start.
 */
UCLASS()
class GEOTRINITYUI_API UGeoMessageSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Posts to the subsystem of WorldContext's game instance; nothing without one. */
	static void Post(UObject const* WorldContext, FText const& Text, EGeoColor Color);

	/** Adds a message at the current time, last in the stack, unless the newest says the same and was posted less than
	 * DuplicateSeconds ago. */
	void PostMessage(FText const& Text, EGeoColor Color);

	/** Forgets every message, as when the menu closes. */
	void Clear();

	/** Forgets the messages that are gone from the screen at Now, so the log can go back to sleep. */
	void Prune(double Now, FGeoMessageLayoutParams const& Params);

	TArray<FGeoMessage> const& GetMessages() const { return Messages; }

	/**
	 * Slot and alpha of every message visible at Now. The bottom line is the oldest live message; a message takes the
	 * line above the live ones older than it, and eases down when one of them expires. A message arriving over MaxLines
	 * live ones expires the oldest at once. Alpha fades in over FadeTime after the post, and out over FadeTime after
	 * the expiry.
	 */
	static TArray<FGeoMessageLine> Layout(TArray<FGeoMessage> const& Messages, double Now,
										  FGeoMessageLayoutParams const& Params);

	/** Seconds an identical message right after the newest is taken for the same click and dropped. */
	static constexpr double DuplicateSeconds = 2.;

	/** Fires when a message is posted, so a sleeping log wakes. */
	FOnGeoMessagePosted OnMessagePosted;

private:
	/** Time each message stops being live: its post time plus Lifetime, or the post time of the one that pushed it out.
	 */
	static TArray<double> ComputeExpiries(TArray<FGeoMessage> const& Messages, FGeoMessageLayoutParams const& Params);

	TArray<FGeoMessage> Messages;
};
