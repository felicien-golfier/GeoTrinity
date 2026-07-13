// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/Data/AbilityInfo.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "GameplayTagsManager.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

/**
 * Walks the CDO's asset tags and returns the first tag under the AbilitySpell root.
 * Called once at asset load / property change, not at runtime — synchronous CDO access is intentional.
 */
static FGameplayTag GetAbilityTagFromCDO(TSubclassOf<UGameplayAbility> const& AbilityClass)
{

	if (!AbilityClass)
	{
		return FGameplayTag();
	}

	FGameplayTag const SpellRoot =
		UGameplayTagsManager::Get().RequestGameplayTag(FName(RootTagNames::AbilitySpellTag), false);
	for (FGameplayTag const& Tag : AbilityClass.GetDefaultObject()->GetAssetTags())
	{
		if (Tag.MatchesTag(SpellRoot))
		{
			return Tag;
		}
	}

	ensureMsgf(false, TEXT("Ability %s has no AbilityTag, please fill the AssetTags in the BP under Tag category"),
			   *AbilityClass->GetName());

	return FGameplayTag();
}

static FGameplayTag GetAbilityTypeTagFromCDO(TSubclassOf<UGameplayAbility> const& AbilityClass)
{
	if (!AbilityClass)
	{
		return FGameplayTag();
	}

	FGameplayTag const AbilityTypeRoot = FGeoGameplayTags::Get().Ability_Type;
	for (FGameplayTag const& Tag : AbilityClass.GetDefaultObject()->GetAssetTags())
	{
		if (Tag.MatchesTag(AbilityTypeRoot))
		{
			return Tag;
		}
	}

	ensureMsgf(false, TEXT("Ability %s has no Ability Type, please fill the AssetTags in the BP under Tag category"),
			   *AbilityClass->GetName());
	return FGameplayTag();
}

namespace
{
	// Level bounds a {Token:range} suffix evaluates over — for values whose curve is driven by another system than
	// ability level (e.g. the reload's remaining-ammo scale), rendered as a level 1 → 10 min-max range.
	constexpr int32 MinDescriptionLevel = 1;
	constexpr int32 MaxDescriptionLevel = 10;

	// How a resolved scalar value is rendered: as-is, or as a percentage (raw ×100) / bonus percentage ((raw−1)×100),
	// selected per token by a {Token:%} / {Token:+%} suffix so a 1.5 multiplier reads "150%" or "50%".
	enum class EValueFormat : uint8
	{
		Plain,
		Percent,
		BonusPercent
	};

	// Per-token render options. Levels are equal unless bShowRange, in which case a value is evaluated at
	// MinDescriptionLevel and MaxDescriptionLevel to show its full curve range.
	struct FDescriptionFormat
	{
		int32 AbilityLevel = 1;
		bool bShowRange = false;
		bool bRichTextValues = false;
		EValueFormat ValueFormat = EValueFormat::Plain;

		int32 MinLevel() const { return bShowRange ? MinDescriptionLevel : AbilityLevel; }
		int32 MaxLevel() const { return bShowRange ? MaxDescriptionLevel : AbilityLevel; }
	};
}

/** Wraps a resolved value in the <Value> rich-text style tag so the UI can color it. */
static FString MarkUpValue(FString const& Value, FDescriptionFormat const& Format)
{
	return Format.bRichTextValues ? FString::Printf(TEXT("<Value>%s</>"), *Value) : Value;
}

static FString FormatValueRange(float Min, float Max, FDescriptionFormat const& Format)
{
	TCHAR const* Suffix = TEXT("");
	if (Format.ValueFormat != EValueFormat::Plain)
	{
		float const Base = Format.ValueFormat == EValueFormat::BonusPercent ? 1.f : 0.f;
		Min = (Min - Base) * 100.f;
		Max = (Max - Base) * 100.f;
		Suffix = TEXT("%");
	}
	return MarkUpValue(FMath::IsNearlyEqual(Min, Max) ? FString::Printf(TEXT("%g%s"), Min, Suffix)
													  : FString::Printf(TEXT("%g-%g%s"), Min, Max, Suffix),
					   Format);
}

static FString FormatScalableRange(FScalableFloat const& Scalable, FDescriptionFormat const& Format)
{
	return FormatValueRange(Scalable.GetValueAtLevel(Format.MinLevel()), Scalable.GetValueAtLevel(Format.MaxLevel()),
							Format);
}

static FString GetTagLeafName(FGameplayTag const& Tag)
{
	FString LeafName = Tag.GetTagName().ToString();
	int32 LastDotIndex = INDEX_NONE;
	LeafName.FindLastChar(TEXT('.'), LastDotIndex);
	return LastDotIndex == INDEX_NONE ? LeafName : LeafName.Mid(LastDotIndex + 1);
}

/** Expands an effect-data array ({Effects} or a named array property): one line per effect entry. */
static FString BuildEffectsSummary(TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
								   FDescriptionFormat const& Format)
{
	TArray<FString> Lines;
	for (TInstancedStruct<FEffectData> const& Data : EffectDataArray)
	{
		if (FDamageEffectData const* Damage = Data.GetPtr<FDamageEffectData>())
		{
			Lines.Add(FString::Printf(TEXT("Damage: %s"), *FormatScalableRange(Damage->DamageAmount, Format)));
		}
		else if (FHealEffectData const* Heal = Data.GetPtr<FHealEffectData>())
		{
			Lines.Add(FString::Printf(TEXT("Heal: %s"), *FormatScalableRange(Heal->HealAmount, Format)));
		}
		else if (FShieldEffectData const* Shield = Data.GetPtr<FShieldEffectData>())
		{
			Lines.Add(FString::Printf(TEXT("Shield: %s"), *FormatScalableRange(Shield->ShieldAmount, Format)));
		}
		else if (FStatusEffectData const* Status = Data.GetPtr<FStatusEffectData>())
		{
			Lines.Add(FString::Printf(
				TEXT("%s status (%s chance)"), *GetTagLeafName(Status->StatusTag),
				*MarkUpValue(FString::Printf(TEXT("%d%%"), Status->StatusChance), Format)));
		}
		else if (FGameplayEffectData const* Effect = Data.GetPtr<FGameplayEffectData>())
		{
			if (!Effect->GameplayEffect)
			{
				continue;
			}
			FString const Name = Effect->DataTag.IsValid() ? GetTagLeafName(Effect->DataTag)
														   : Effect->GameplayEffect->GetName();
			FString Line = FString::Printf(TEXT("%s: %s"), *Name, *FormatScalableRange(Effect->Magnitude, Format));
			if (Effect->Duration.GetValueAtLevel(Format.MinLevel()) > 0.f)
			{
				Line += FString::Printf(TEXT(" for %ss"), *FormatScalableRange(Effect->Duration, Format));
			}
			Lines.Add(Line);
		}
		else if (FContextDamageMultiplierEffectData const* Multiplier = Data.GetPtr<FContextDamageMultiplierEffectData>())
		{
			FDescriptionFormat BonusPercentFormat = Format;
			BonusPercentFormat.ValueFormat = EValueFormat::BonusPercent;
			Lines.Add(FString::Printf(TEXT("%s more damage"),
									  *FormatScalableRange(Multiplier->Multiplier, BonusPercentFormat)));
		}
		else if (Data.GetPtr<FLethalEffectData>())
		{
			Lines.Add(TEXT("Lethal"));
		}
	}
	return FString::Join(Lines, TEXT("\n"));
}

/** Resolves a numeric or FScalableFloat property to its scalar value at the given level; false if not such a property. */
static bool ResolvePropertyScalar(FString const& PropertyName, UGeoGameplayAbility const& AbilityCDO, int32 Level,
								  float& OutValue)
{
	FProperty const* Property = AbilityCDO.GetClass()->FindPropertyByName(*PropertyName);
	if (FNumericProperty const* Numeric = CastField<FNumericProperty>(Property))
	{
		void const* ValuePtr = Numeric->ContainerPtrToValuePtr<void>(&AbilityCDO);
		OutValue = Numeric->IsFloatingPoint() ? Numeric->GetFloatingPointPropertyValue(ValuePtr)
											  : Numeric->GetSignedIntPropertyValue(ValuePtr);
		return true;
	}
	if (FStructProperty const* Struct = CastField<FStructProperty>(Property);
		Struct && Struct->Struct == TBaseStructure<FScalableFloat>::Get())
	{
		OutValue = Struct->ContainerPtrToValuePtr<FScalableFloat>(&AbilityCDO)->GetValueAtLevel(Level);
		return true;
	}
	return false;
}

/** Resolves Token to its formatted value at the Format levels (range when they differ). */
static bool ResolveDescriptionToken(FString const& Token, UGeoGameplayAbility const& AbilityCDO,
									FDescriptionFormat const& Format, FString& OutValue)
{
	// {A*B} multiplies two numeric/scalable properties — e.g. a per-unit value by its max count for a cap.
	FString LeftName;
	FString RightName;
	if (Token.Split(TEXT("*"), &LeftName, &RightName))
	{
		float LeftMin, LeftMax, RightMin, RightMax;
		if (ResolvePropertyScalar(LeftName, AbilityCDO, Format.MinLevel(), LeftMin)
			&& ResolvePropertyScalar(LeftName, AbilityCDO, Format.MaxLevel(), LeftMax)
			&& ResolvePropertyScalar(RightName, AbilityCDO, Format.MinLevel(), RightMin)
			&& ResolvePropertyScalar(RightName, AbilityCDO, Format.MaxLevel(), RightMax))
		{
			OutValue = FormatValueRange(LeftMin * RightMin, LeftMax * RightMax, Format);
			return true;
		}
		return false;
	}

	if (Token == TEXT("Cooldown"))
	{
		OutValue = FormatValueRange(AbilityCDO.GetCooldown(Format.MinLevel()), AbilityCDO.GetCooldown(Format.MaxLevel()),
									Format);
		return true;
	}
	if (Token == TEXT("FireDelay"))
	{
		OutValue = FormatValueRange(AbilityCDO.GetFireDelay(), AbilityCDO.GetFireDelay(), Format);
		return true;
	}
	if (Token == TEXT("Damage") || Token == TEXT("Heal") || Token == TEXT("Shield"))
	{
		float Min = 0.f;
		float Max = 0.f;
		bool bFound = false;
		for (TInstancedStruct<FEffectData> const& Data : AbilityCDO.GetEffectDataArray())
		{
			FScalableFloat const* Amount = nullptr;
			if (FDamageEffectData const* Damage = Token == TEXT("Damage") ? Data.GetPtr<FDamageEffectData>() : nullptr)
			{
				Amount = &Damage->DamageAmount;
			}
			else if (FHealEffectData const* Heal = Token == TEXT("Heal") ? Data.GetPtr<FHealEffectData>() : nullptr)
			{
				Amount = &Heal->HealAmount;
			}
			else if (FShieldEffectData const* Shield =
						 Token == TEXT("Shield") ? Data.GetPtr<FShieldEffectData>() : nullptr)
			{
				Amount = &Shield->ShieldAmount;
			}
			if (Amount)
			{
				Min += Amount->GetValueAtLevel(Format.MinLevel());
				Max += Amount->GetValueAtLevel(Format.MaxLevel());
				bFound = true;
			}
		}
		OutValue = FormatValueRange(Min, Max, Format);
		return bFound;
	}

	float Min, Max;
	if (ResolvePropertyScalar(Token, AbilityCDO, Format.MinLevel(), Min)
		&& ResolvePropertyScalar(Token, AbilityCDO, Format.MaxLevel(), Max))
	{
		OutValue = FormatValueRange(Min, Max, Format);
		return true;
	}

	FProperty const* Property = AbilityCDO.GetClass()->FindPropertyByName(*Token);
	FStructProperty const* Struct = CastField<FStructProperty>(Property);
	FArrayProperty const* Array = CastField<FArrayProperty>(Property);
	FStructProperty const* Inner = Array ? CastField<FStructProperty>(Array->Inner) : nullptr;
	if (Inner && Inner->Struct == TBaseStructure<FInstancedStruct>::Get())
	{
		OutValue = BuildEffectsSummary(
			*Array->ContainerPtrToValuePtr<TArray<TInstancedStruct<FEffectData>>>(&AbilityCDO), Format);
		return true;
	}
	// A single TInstancedStruct effect property resolves to its magnitude scalar (e.g. {SpeedBuffEffect:%}).
	if (Struct && Struct->Struct == TBaseStructure<FInstancedStruct>::Get())
	{
		TInstancedStruct<FEffectData> const& Effect =
			*Struct->ContainerPtrToValuePtr<TInstancedStruct<FEffectData>>(&AbilityCDO);
		if (FGameplayEffectData const* GameplayEffect = Effect.GetPtr<FGameplayEffectData>())
		{
			OutValue = FormatScalableRange(GameplayEffect->Magnitude, Format);
			return true;
		}
	}
	return false;
}

static FString GetDescriptionsFilePath()
{
	return FPaths::ProjectContentDir() / TEXT("Data/AbilityDescriptions.txt");
}

/**
 * Returns the [AbilityTag] section of Content/Data/AbilityDescriptions.txt, empty when the file or section is
 * missing. Lines starting with # are comments. Re-read on every call so the file can be edited live.
 */
static FString LoadDescriptionFromFile(FGameplayTag const& AbilityTag)
{
	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *GetDescriptionsFilePath()))
	{
		return FString();
	}

	TArray<FString> Lines;
	FileContent.ParseIntoArrayLines(Lines, false);
	FString const SectionHeader = FString::Printf(TEXT("[%s]"), *AbilityTag.ToString());
	TArray<FString> SectionLines;
	bool bInSection = false;
	for (FString const& Line : Lines)
	{
		if (Line.StartsWith(TEXT("[")))
		{
			if (bInSection)
			{
				break;
			}
			bInSection = Line.TrimEnd() == SectionHeader;
		}
		else if (bInSection && !Line.StartsWith(TEXT("#")))
		{
			SectionLines.Add(Line);
		}
	}
	return FString::Join(SectionLines, TEXT("\n")).TrimStartAndEnd();
}

#if WITH_EDITOR
/**
 * Writes Description as the body of the [AbilityTag] section of Content/Data/AbilityDescriptions.txt, replacing the
 * existing section body when present (preserving every other section and the header comments) or appending a new
 * section otherwise. Editor-only: keeps the plain-text source of truth in sync when the field is edited in the asset.
 */
static void WriteDescriptionToFile(FGameplayTag const& AbilityTag, FString const& Description)
{
	FString const FilePath = GetDescriptionsFilePath();
	FString FileContent;
	FFileHelper::LoadFileToString(FileContent, *FilePath);

	TArray<FString> Lines;
	FileContent.ParseIntoArrayLines(Lines, false);
	FString const SectionHeader = FString::Printf(TEXT("[%s]"), *AbilityTag.ToString());

	TArray<FString> OutLines;
	bool bReplaced = false;
	bool bSkippingBody = false;
	for (FString const& Line : Lines)
	{
		if (bSkippingBody && !Line.StartsWith(TEXT("[")))
		{
			continue;
		}
		bSkippingBody = false;
		if (Line.TrimEnd() == SectionHeader)
		{
			OutLines.Add(Line);
			OutLines.Add(Description);
			OutLines.Add(FString());
			bReplaced = true;
			bSkippingBody = true;
			continue;
		}
		OutLines.Add(Line);
	}

	if (!bReplaced)
	{
		if (OutLines.Num() > 0 && !OutLines.Last().IsEmpty())
		{
			OutLines.Add(FString());
		}
		OutLines.Add(SectionHeader);
		OutLines.Add(Description);
	}

	FFileHelper::SaveStringToFile(FString::Join(OutLines, TEXT("\n")) + TEXT("\n"), *FilePath);
}
#endif

// ---------------------------------------------------------------------------------------------------------------------
FString FGameplayAbilityInfo::GetResolvedDescription(int32 AbilityLevel, bool bRichTextValues) const
{
	FString const FileDescription = LoadDescriptionFromFile(AbilityTag);
	FString const& Text = FileDescription.IsEmpty() ? Description : FileDescription;

	UGeoGameplayAbility const* AbilityCDO =
		AbilityClass ? Cast<UGeoGameplayAbility>(AbilityClass->GetDefaultObject()) : nullptr;
	if (!AbilityCDO)
	{
		return Text;
	}

	FDescriptionFormat Format;
	Format.AbilityLevel = AbilityLevel;
	Format.bRichTextValues = bRichTextValues;

	FString Resolved;
	Resolved.Reserve(Text.Len());
	int32 Index = 0;
	while (Index < Text.Len())
	{
		int32 const OpenBrace = Text.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Index);
		int32 const CloseBrace = OpenBrace == INDEX_NONE
									 ? INDEX_NONE
									 : Text.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart,
												 OpenBrace + 1);
		if (CloseBrace == INDEX_NONE)
		{
			Resolved += Text.Mid(Index);
			break;
		}

		Resolved += Text.Mid(Index, OpenBrace - Index);
		FString const Token = Text.Mid(OpenBrace + 1, CloseBrace - OpenBrace - 1);

		// Suffixes, combinable in any order: {Token:range} evaluates over the curve's level 1→10 range (for values
		// driven by another system than ability level); {Token:%} / {Token:+%} renders the scalar as a percentage /
		// bonus percentage.
		FString TokenName = Token;
		FDescriptionFormat TokenFormat = Format;
		for (bool bStripped = true; bStripped;)
		{
			bStripped = false;
			if (TokenName.RemoveFromEnd(TEXT(":range")))
			{
				TokenFormat.bShowRange = true;
				bStripped = true;
			}
			else if (TokenName.RemoveFromEnd(TEXT(":+%")))
			{
				TokenFormat.ValueFormat = EValueFormat::BonusPercent;
				bStripped = true;
			}
			else if (TokenName.RemoveFromEnd(TEXT(":%")))
			{
				TokenFormat.ValueFormat = EValueFormat::Percent;
				bStripped = true;
			}
		}

		FString TokenValue;
		if (TokenName == TEXT("Effects"))
		{
			Resolved += BuildEffectsSummary(AbilityCDO->GetEffectDataArray(), TokenFormat);
		}
		else if (ResolveDescriptionToken(TokenName, *AbilityCDO, TokenFormat, TokenValue))
		{
			Resolved += TokenValue;
		}
		else
		{
			UE_LOG(LogGeoASC, Warning, TEXT("Ability %s description token {%s} could not be resolved"),
				   *AbilityDisplayName, *Token);
			Resolved += Text.Mid(OpenBrace, CloseBrace - OpenBrace + 1);
		}
		Index = CloseBrace + 1;
	}
	return Resolved;
}

static void PopulateTagsForPlayerAbilitiesArray(TArray<FPlayersGameplayAbilityInfo>& Infos)
{
	for (FPlayersGameplayAbilityInfo& Info : Infos)
	{
		Info.AbilityTag = GetAbilityTagFromCDO(Info.AbilityClass);
		Info.TypeOfAbilityTag = GetAbilityTypeTagFromCDO(Info.AbilityClass);
	}
}

#if WITH_EDITOR
/** Pulls each entry's Description from its file section, so the asset shows the plain-text source of truth. */
static void LoadDescriptionsFromFile(TArray<FGameplayAbilityInfo*> const& Infos)
{
	for (FGameplayAbilityInfo* Info : Infos)
	{
		if (!Info->AbilityTag.IsValid())
		{
			continue;
		}
		FString const FileDescription = LoadDescriptionFromFile(Info->AbilityTag);
		if (!FileDescription.IsEmpty())
		{
			Info->Description = FileDescription;
		}
	}
}
#endif

// ---------------------------------------------------------------------------------------------------------------------
void UAbilityInfo::PopulateAbilityTags()
{
	for (FGameplayAbilityInfo& Info : EnemyAbilityInfos)
	{
		Info.AbilityTag = GetAbilityTagFromCDO(Info.AbilityClass);
	}
	PopulateTagsForPlayerAbilitiesArray(TriangleAbilities);
	PopulateTagsForPlayerAbilitiesArray(CircleAbilities);
	PopulateTagsForPlayerAbilitiesArray(SquareAbilities);
	PopulateTagsForPlayerAbilitiesArray(SharedAbilities);

	// Tags just changed — drop the cache so GetAbilityClassForTag rebuilds it on next access.
	AbilityClassByTag.Reset();
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FGameplayAbilityInfo*> UAbilityInfo::GetAllAbilityInfoPtrs()
{
	TArray<FGameplayAbilityInfo*> Ptrs;
	for (TArray<FPlayersGameplayAbilityInfo>* Array :
		 {&TriangleAbilities, &CircleAbilities, &SquareAbilities, &SharedAbilities})
	{
		for (FPlayersGameplayAbilityInfo& Info : *Array)
		{
			Ptrs.Add(&Info);
		}
	}
	for (FGameplayAbilityInfo& Info : EnemyAbilityInfos)
	{
		Ptrs.Add(&Info);
	}
	return Ptrs;
}

// ---------------------------------------------------------------------------------------------------------------------
void UAbilityInfo::PostLoad()
{
	Super::PostLoad();
	if (!FGeoGameplayTags::AreNativeTagsInitialized())
	{
		return;
	}
	PopulateAbilityTags();
#if WITH_EDITOR
	LoadDescriptionsFromFile(GetAllAbilityInfoPtrs());
#endif
}

#if WITH_EDITOR
// ---------------------------------------------------------------------------------------------------------------------
FGameplayAbilityInfo* UAbilityInfo::FindAbilityInfo(FName ArrayName, int32 Index)
{
	auto const IndexInto = [Index](auto& Array) -> FGameplayAbilityInfo*
	{ return Array.IsValidIndex(Index) ? &Array[Index] : nullptr; };

	if (ArrayName == GET_MEMBER_NAME_CHECKED(UAbilityInfo, TriangleAbilities))
	{
		return IndexInto(TriangleAbilities);
	}
	if (ArrayName == GET_MEMBER_NAME_CHECKED(UAbilityInfo, CircleAbilities))
	{
		return IndexInto(CircleAbilities);
	}
	if (ArrayName == GET_MEMBER_NAME_CHECKED(UAbilityInfo, SquareAbilities))
	{
		return IndexInto(SquareAbilities);
	}
	if (ArrayName == GET_MEMBER_NAME_CHECKED(UAbilityInfo, SharedAbilities))
	{
		return IndexInto(SharedAbilities);
	}
	if (ArrayName == GET_MEMBER_NAME_CHECKED(UAbilityInfo, EnemyAbilityInfos))
	{
		return IndexInto(EnemyAbilityInfos);
	}
	return nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
void UAbilityInfo::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	PopulateAbilityTags();

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FGameplayAbilityInfo, Description))
	{
		FName const ArrayName = PropertyChangedEvent.GetMemberPropertyName();
		// Write only the entry the user just edited; reloading first would clobber it with the on-disk value.
		FGameplayAbilityInfo const* Edited = FindAbilityInfo(ArrayName, PropertyChangedEvent.GetArrayIndex(ArrayName.ToString()));
		if (Edited && Edited->AbilityTag.IsValid())
		{
			WriteDescriptionToFile(Edited->AbilityTag, Edited->Description);
		}
	}
	// Merge external edits to other sections back into the asset (also refreshes after an AbilityClass/tag change).
	LoadDescriptionsFromFile(GetAllAbilityInfoPtrs());
}
#endif

// ---------------------------------------------------------------------------------------------------------------------
TArray<FPlayersGameplayAbilityInfo> UAbilityInfo::GetAbilitiesForClass(EPlayerClass PlayerClass) const
{
	ensureMsgf(PlayerClass != EPlayerClass::None && PlayerClass != EPlayerClass::All,
			   TEXT("GetAbilitiesForClass called with invalid PlayerClass %s"), *UEnum::GetValueAsString(PlayerClass));

	TArray<FPlayersGameplayAbilityInfo> Result = SharedAbilities;
	switch (PlayerClass)
	{
	case EPlayerClass::Triangle:
		Result.Append(TriangleAbilities);
		break;
	case EPlayerClass::Circle:
		Result.Append(CircleAbilities);
		break;
	case EPlayerClass::Square:
		Result.Append(SquareAbilities);
		break;
	default:
		break;
	}
	return Result;
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FPlayersGameplayAbilityInfo> UAbilityInfo::GetAllPlayersAbilityInfos() const
{
	TArray<FPlayersGameplayAbilityInfo> Result;
	Result.Append(TriangleAbilities);
	Result.Append(CircleAbilities);
	Result.Append(SquareAbilities);
	Result.Append(SharedAbilities);
	return Result;
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FGameplayAbilityInfo> UAbilityInfo::GetAllAbilityInfos() const
{
	TArray<FGameplayAbilityInfo> AllInfos;
	for (FPlayersGameplayAbilityInfo const& Info : GetAllPlayersAbilityInfos())
	{
		AllInfos.Add(Info);
	}
	for (FGameplayAbilityInfo const& Info : EnemyAbilityInfos)
	{
		AllInfos.Add(Info);
	}
	return AllInfos;
}
// ---------------------------------------------------------------------------------------------------------------------
TSubclassOf<UGameplayAbility> UAbilityInfo::GetAbilityClassForTag(FGameplayTag AbilityTag) const
{
	if (!AbilityTag.IsValid())
	{
		return nullptr;
	}

	if (AbilityClassByTag.IsEmpty())
	{
		for (FGameplayAbilityInfo const& Info : GetAllAbilityInfos())
		{
			if (Info.AbilityTag.IsValid() && IsValid(Info.AbilityClass))
			{
				AbilityClassByTag.Add(Info.AbilityTag, Info.AbilityClass);
			}
		}
	}

	TSubclassOf<UGameplayAbility> const* Found = AbilityClassByTag.Find(AbilityTag);
	return Found ? *Found : nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FGameplayAbilityInfo> UAbilityInfo::FindAbilityInfoForListOfTag(TArray<FGameplayTag> const& AbilityTags,
																	   bool bLogIfNotFound) const
{
	TArray<FGameplayAbilityInfo> CorrespondingInfos;
	for (FGameplayAbilityInfo const& Info : EnemyAbilityInfos)
	{
		if (AbilityTags.Contains(Info.AbilityTag))
		{
			CorrespondingInfos.Add(Info);
		}
	}

	for (FPlayersGameplayAbilityInfo const& Info : GetAllPlayersAbilityInfos())
	{
		if (AbilityTags.Contains(Info.AbilityTag))
		{
			CorrespondingInfos.Add(Info);
		}
	}

	if (bLogIfNotFound && CorrespondingInfos.Num() != AbilityTags.Num())
	{

		UE_LOG(LogGeoASC, Error, TEXT("NOT all tags were found on AbilityInfos %s"), *GetName());
	}

	return CorrespondingInfos;
}
