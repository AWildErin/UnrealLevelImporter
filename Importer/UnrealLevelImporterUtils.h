#pragma once

#include "CoreMinimal.h"

namespace FUnrealLevelImporterUtils
{
// Taken from Editor.h because it isn't exported
bool ParseBegin(const TCHAR** Stream, const TCHAR* Match);

// Taken from Editor.h because it isn't exported
bool ParseEnd(const TCHAR** Stream, const TCHAR* Match);

/** @custom_region{Rotator} */
/**
* Parses a rotator from a T3D string
* @param PropText		Text to extract the rotator from
* @param bFixupRotator	Whether or not to correct rotator units
*/
FRotator ParseRotator(const TCHAR* PropText, bool bFixupRotator);

/** Writes the supplied rotator to T3D Format  */
FString WriteRotator(FRotator Rotator);

/** Converts a UE3 FRotator to UE4 units. But does it by copying, meaning the given FRotator is not modified. */
FRotator FRotatorToUE4(FRotator Rotator);

/** Converts a UE3 FRotator to UE4 units. But does it by reference, editing the given FRotator directly. */
void FRotatorToUE4Inline(FRotator& Rotator);

FString FixupRotatorString(const FString& String);
/** @} */

/** @custom_region{Vector} */
FVector ParseVector(const TCHAR* PropText);
FString WriteVector(FVector Vec);
/** @} */

/**
 * Gets the remapped package path for the supplied package
 * Example:

 * StaticMesh'MyPackage.MyMesh' -> StaticMesh'/Game/MyGame/Meshes/MyPackage/MyMesh.MyMesh'
 */
FString GetRemappedPackagePath(const FString& OriginalPath);
};
