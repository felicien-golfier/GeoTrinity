// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Buttons/GeoFloorButton.h"
#include "CoreMinimal.h"
#include "Tool/GeoDifficulty.h"

#include "GeoDifficultyButton.generated.h"

/**
 * Floor pad that sets the run's difficulty to its own. One pad per value rather than one pad cycling through them, so
 * choosing is stepping onto the tuning you want and stepping on it twice changes nothing; the pad whose Difficulty is
 * live wears ActiveColor, which is how the group reads the current setting off the floor. Placing a pad is one
 * property: OnConstruction writes the enum's display name into the label, so there is no text to keep in sync.
 * The pad itself only writes AGeoGameState::SetDifficulty — every arena respawns its boss at the new tuning by
 * reacting to OnDifficultyChanged, so nothing here knows an arena.
 */
UCLASS()
class GEOTRINITY_API AGeoDifficultyButton : public AGeoFloorButton
{
	GENERATED_BODY()

protected:
	/** Writes Difficulty's display name into the label, so placing a pad never means typing its text. */
	virtual void OnConstruction(FTransform const& Transform) override;

	/** Subscribes to the difficulty and paints the label for the value already live. */
	virtual void BeginPlay() override;

	virtual void Press() override;

	/** The tuning this pad selects, and the one it lights up for. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoDifficulty")
	EGeoDifficulty Difficulty = EGeoDifficulty::Original;

	/** Label color while this pad's Difficulty is the live one. */
	UPROPERTY(EditAnywhere, Category = "GeoDifficulty")
	FColor ActiveColor = FColor(255, 255, 255);

	/** Label color while another pad holds the live difficulty. */
	UPROPERTY(EditAnywhere, Category = "GeoDifficulty")
	FColor InactiveColor = FColor(80, 80, 80);

private:
	/** Repaints the label for whichever difficulty is live. Bound to AGeoGameState::OnDifficultyChanged. */
	UFUNCTION()
	void RefreshLabel();
};
