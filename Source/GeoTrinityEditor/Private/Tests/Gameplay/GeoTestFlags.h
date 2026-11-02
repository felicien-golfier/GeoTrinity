// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Misc/AutomationTest.h"

/** Flags of the areas whose tests span several files (one blob in a unity build, so one declaration). */
inline constexpr EAutomationTestFlags GeoCharactersTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
inline constexpr EAutomationTestFlags GeoDeployableTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
inline constexpr EAutomationTestFlags GeoEncounterTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
inline constexpr EAutomationTestFlags GeoGemTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
