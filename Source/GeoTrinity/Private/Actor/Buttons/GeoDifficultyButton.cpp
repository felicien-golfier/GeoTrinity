// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Buttons/GeoDifficultyButton.h"

#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameClasses/GeoGameState.h"

void AGeoDifficultyButton::OnConstruction(FTransform const& Transform)
{
	Super::OnConstruction(Transform);
	Label->SetText(UEnum::GetDisplayValueAsText(Difficulty));
}

void AGeoDifficultyButton::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetGameStateChecked<AGeoGameState>()->OnDifficultyChanged.AddUniqueDynamic(
		this, &AGeoDifficultyButton::RefreshLabel);
	RefreshLabel();
}

void AGeoDifficultyButton::Press()
{
	GetWorld()->GetGameStateChecked<AGeoGameState>()->SetDifficulty(Difficulty);
}

void AGeoDifficultyButton::RefreshLabel()
{
	bool const bIsLive = GetWorld()->GetGameStateChecked<AGeoGameState>()->GetDifficulty() == Difficulty;
	Label->SetTextRenderColor(bIsLive ? ActiveColor : InactiveColor);
}
