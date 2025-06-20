#include "Importer/UnrealLevelImporterUtils.h"

#include "Logging/StructuredLog.h"
#include "Misc/Parse.h"

#include "Importer/UnrealLevelImporterSettings.h"

// Converts UE3 rotation units to degrees
constexpr float UNR_TO_DEG = 0.00549316540360483f;

// Converts degrees to UE3 rotation units
constexpr float DEG_TO_UNR = 182.0444f;

// Package name pattern
// Group 1: Class			- Class of the object
// Group 2: Name			- Package name
// Group 3: Subpackage path	- Sub package path, subsequent .'s are placed with /
const static FRegexPattern PackageNamePattern(TEXT("(\\w*)'([^.]*).(.*)'"));

bool FUnrealLevelImporterUtils::ParseBegin(const TCHAR** Stream, const TCHAR* Match)
{
	const TCHAR* Original = *Stream;
	if (FParse::Command(Stream, TEXT("BEGIN")) && FParse::Command(Stream, Match))
		return true;
	*Stream = Original;
	return false;
}

bool FUnrealLevelImporterUtils::ParseEnd(const TCHAR** Stream, const TCHAR* Match)
{
	const TCHAR* Original = *Stream;
	if (FParse::Command(Stream, TEXT("END")) && FParse::Command(Stream, Match))
		return true; // Gotten.
	*Stream = Original;
	return false;
}

FRotator FUnrealLevelImporterUtils::ParseRotator(const TCHAR* PropText, bool bFixupRotator)
{
	FRotator Rotator = FRotator(0.f);

	if (!FParse::Value(PropText, TEXT("Pitch="), Rotator.Pitch))
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Failed to extract pitch value from {text}. Returning 0, 0, 0 rotation.", PropText);
		return FRotator::ZeroRotator;
	}

	if (!FParse::Value(PropText, TEXT("Yaw="), Rotator.Yaw))
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Failed to extract Yaw value from {text}. Returning 0, 0, 0 rotation.", PropText);
		return FRotator::ZeroRotator;
	}

	if (!FParse::Value(PropText, TEXT("Roll="), Rotator.Roll))
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Failed to extract Roll value from {text}. Returning 0, 0, 0 rotation.", PropText);
		return FRotator::ZeroRotator;
	}

	if (bFixupRotator)
	{
		FRotatorToUE4Inline(Rotator);
	}

	return Rotator;
}

FString FUnrealLevelImporterUtils::WriteRotator(FRotator Rotator)
{
	return FString::Format(TEXT("(Pitch={0},Yaw={1},Roll={2})"), {Rotator.Pitch, Rotator.Yaw, Rotator.Roll});
}

FRotator FUnrealLevelImporterUtils::FRotatorToUE4(FRotator Rotator)
{
	FRotator NewRotator = Rotator;

	Rotator.Pitch *= UNR_TO_DEG;
	Rotator.Yaw *= UNR_TO_DEG;
	Rotator.Roll *= UNR_TO_DEG;

	return NewRotator;
}

void FUnrealLevelImporterUtils::FRotatorToUE4Inline(FRotator& Rotator)
{
	Rotator.Pitch *= UNR_TO_DEG;
	Rotator.Yaw *= UNR_TO_DEG;
	Rotator.Roll *= UNR_TO_DEG;
}

FString FUnrealLevelImporterUtils::FixupRotatorString(const FString& String)
{
	FString NewString = WriteRotator(ParseRotator(*String, true));
	return NewString;
}

FVector FUnrealLevelImporterUtils::ParseVector(const TCHAR* PropText)
{
	FVector Vector = FVector(0.f);

	if (!FParse::Value(PropText, TEXT("X="), Vector.X))
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Failed to extract X value from {text}. Returning ZeroVector.", PropText);
		return FVector::ZeroVector;
	}

	if (!FParse::Value(PropText, TEXT("Y="), Vector.Y))
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Failed to extract Y value from {text}. Returning ZeroVector.", PropText);
		return FVector::ZeroVector;
	}

	if (!FParse::Value(PropText, TEXT("Z="), Vector.Z))
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Failed to extract Z value from {text}. Returning ZeroVector.", PropText);
		return FVector::ZeroVector;
	}

	return Vector;
}

FString FUnrealLevelImporterUtils::WriteVector(FVector Vec)
{
	return FString::Format(TEXT("(X={0},Y={1},Z={2})"), {Vec.X, Vec.Y, Vec.Z});
}

FString FUnrealLevelImporterUtils::GetRemappedPackagePath(const FString& OriginalPath)
{
	FRegexMatcher RegexMatcher(PackageNamePattern, OriginalPath);
	if (!RegexMatcher.FindNext())
	{
		UE_LOGFMT(LogUnrealImporter, Warning, "Failed to extract regex on {0}", OriginalPath);
		return OriginalPath;
	}

	const UUnrealLevelImporterSettings* Settings = UUnrealLevelImporterSettings::Get();

	FString Class = RegexMatcher.GetCaptureGroup(1);
	FString Path = RegexMatcher.GetCaptureGroup(2);
	FString SubPath = RegexMatcher.GetCaptureGroup(3);

	// Search for package in settings
	if (auto RemappedPath = Settings->PackageRemap.Find(Path))
	{
		Path = *RemappedPath;
	}
	else
	{
		UE_LOGFMT(LogUnrealImporter, Warning, "Failed to find remap for package {path}", Path);
		return OriginalPath;
	}

	// Fixup the sub path
	int32 LastDotIndex = INDEX_NONE;

	// Extract the object name
	FString ObjectName = SubPath;

	// If we have sub folders get the last dot and extract the object name from it
	if (SubPath.FindLastChar('.', LastDotIndex))
	{
		ObjectName = SubPath.Mid(LastDotIndex+1, SubPath.Len() - LastDotIndex);
	}

	SubPath = SubPath.Replace(TEXT("."), TEXT("/"));
	SubPath += TEXT(".") + ObjectName;

	// Reconstruct the path
	FString NewFullPath = FString::Format(TEXT("{0}'{1}/{2}'"), {Class, Path, SubPath});

	return NewFullPath;
}
