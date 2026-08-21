// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/Data/AbilityInfo.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "GameplayTagsManager.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

/** Returns the CDO's first asset tag under Root, ensuring when the Blueprint's Tag category is missing it.
 *  Called at asset load / property change, not at runtime — synchronous CDO access is intentional. */
static FGameplayTag GetAssetTagFromCDO(TSubclassOf<UGameplayAbility> const& AbilityClass, FGameplayTag Root,
									   TCHAR const* TagPurpose)
{
	if (!AbilityClass)
	{
		return FGameplayTag();
	}

	FGameplayTag const Found = GeoASLib::GetFirstAssetTagUnderRoot(*AbilityClass.GetDefaultObject(), Root);
	ensureMsgf(Found.IsValid(), TEXT("Ability %s has no %s, please fill the AssetTags in the BP under Tag category"),
			   *AbilityClass->GetName(), TagPurpose);
	return Found;
}

static FGameplayTag GetAbilityTagFromCDO(TSubclassOf<UGameplayAbility> const& AbilityClass)
{
	FGameplayTag const SpellRoot =
		UGameplayTagsManager::Get().RequestGameplayTag(FName(RootTagNames::AbilitySpellTag), false);
	return GetAssetTagFromCDO(AbilityClass, SpellRoot, TEXT("AbilityTag"));
}

static FGameplayTag GetAbilityTypeTagFromCDO(TSubclassOf<UGameplayAbility> const& AbilityClass)
{
	return GetAssetTagFromCDO(AbilityClass, FGeoGameplayTags::Get().Ability_Type, TEXT("Ability Type"));
}

/** Expands an effect-data array ({Effects} or a named array property): one line per effect entry that has one. */
static FString BuildEffectsSummary(TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
								   FDescriptionFormat const& Format)
{
	TArray<FString> Lines;
	for (TInstancedStruct<FEffectData> const& Data : EffectDataArray)
	{
		FEffectData const* Effect = Data.GetPtr<FEffectData>();
		FString Line = Effect ? Effect->GetDescriptionLine(Format) : FString();
		if (!Line.IsEmpty())
		{
			Lines.Add(MoveTemp(Line));
		}
	}
	return FString::Join(Lines, TEXT("\n"));
}

/** Resolves a numeric or FScalableFloat property to its scalar value at the given level; false if not such a property.
 */
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

/** The SetByCaller tag a {Damage} / {Heal} / {Shield} token sums the effect array over; invalid for any other. */
static FGameplayTag GetMagnitudeTagForToken(FString const& Token)
{
	FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();
	if (Token == TEXT("Damage"))
	{
		return Tags.Gameplay_Damage;
	}
	if (Token == TEXT("Heal"))
	{
		return Tags.Gameplay_Heal;
	}
	if (Token == TEXT("Shield"))
	{
		return Tags.Gameplay_Shield;
	}
	return FGameplayTag();
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
		OutValue = FormatValueRange(AbilityCDO.GetCooldown(Format.MinLevel()),
									AbilityCDO.GetCooldown(Format.MaxLevel()), Format);
		return true;
	}
	if (Token == TEXT("FireDelay"))
	{
		OutValue = FormatValueRange(AbilityCDO.GetFireDelay(), AbilityCDO.GetFireDelay(), Format);
		return true;
	}
	if (FGameplayTag const MagnitudeTag = GetMagnitudeTagForToken(Token); MagnitudeTag.IsValid())
	{
		float Min = 0.f;
		float Max = 0.f;
		bool bFound = false;
		for (TInstancedStruct<FEffectData> const& Data : AbilityCDO.GetEffectDataArray())
		{
			FMagnitudeEffectData const* Magnitude = Data.GetPtr<FMagnitudeEffectData>();
			if (Magnitude && Magnitude->GetMagnitudeTag() == MagnitudeTag)
			{
				Min += Magnitude->Amount.GetValueAtLevel(Format.MinLevel());
				Max += Magnitude->Amount.GetValueAtLevel(Format.MaxLevel());
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
 * Content/Data/AbilityDescriptions.txt parsed into section header ("[AbilityTag]") → body. Lines starting with # are
 * comments. Re-parsed whenever the file's timestamp changes, so an edit still shows live while a description list
 * built for every ability at once reads the file once instead of once per entry.
 */
static TMap<FString, FString> const& GetDescriptionsBySection()
{
	static TMap<FString, FString> Descriptions;
	static FDateTime ParsedTimeStamp = FDateTime::MinValue();

	FString const FilePath = GetDescriptionsFilePath();
	FDateTime const TimeStamp = IFileManager::Get().GetTimeStamp(*FilePath);
	if (TimeStamp == ParsedTimeStamp)
	{
		return Descriptions;
	}
	ParsedTimeStamp = TimeStamp;
	Descriptions.Reset();

	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
	{
		return Descriptions;
	}

	TArray<FString> Lines;
	FileContent.ParseIntoArrayLines(Lines, false);
	FString SectionHeader;
	TArray<FString> SectionLines;
	auto CommitSection = [&SectionHeader, &SectionLines]()
	{
		if (!SectionHeader.IsEmpty())
		{
			Descriptions.Add(SectionHeader, FString::Join(SectionLines, TEXT("\n")).TrimStartAndEnd());
		}
		SectionLines.Reset();
	};
	for (FString const& Line : Lines)
	{
		if (Line.StartsWith(TEXT("[")))
		{
			CommitSection();
			SectionHeader = Line.TrimEnd();
		}
		else if (!Line.StartsWith(TEXT("#")))
		{
			SectionLines.Add(Line);
		}
	}
	CommitSection();
	return Descriptions;
}

/** Returns the [AbilityTag] section body, empty when the file or the section is missing. */
static FString LoadDescriptionFromFile(FGameplayTag const& AbilityTag)
{
	if (FString const* Description =
			GetDescriptionsBySection().Find(FString::Printf(TEXT("[%s]"), *AbilityTag.ToString())))
	{
		return *Description;
	}
	return FString();
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

	FFileHelper::SaveStringToFile(FString::Join(OutLines, TEXT("\n")) + TEXT("\n"), *FilePath,
								  FFileHelper::EEncodingOptions::ForceUTF8);
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
			: Text.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, OpenBrace + 1);
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

/** Writes every entry's Description back to its file section, making the asset the source of truth. */
static void WriteAllDescriptionsToFile(TArray<FGameplayAbilityInfo*> const& Infos)
{
	for (FGameplayAbilityInfo const* Info : Infos)
	{
		if (Info->AbilityTag.IsValid())
		{
			WriteDescriptionToFile(Info->AbilityTag, Info->Description);
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
void UAbilityInfo::ReloadDescriptionsFromDisc()
{
	LoadDescriptionsFromFile(GetAllAbilityInfoPtrs());
}

// ---------------------------------------------------------------------------------------------------------------------
void UAbilityInfo::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	PopulateAbilityTags();

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FGameplayAbilityInfo, Description))
	{
		WriteAllDescriptionsToFile(GetAllAbilityInfoPtrs());
	}
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
