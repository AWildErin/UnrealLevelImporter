#pragma once

#include "CoreMinimal.h"

#include "Importer/Upgraders/UnrealLevelImporterBaseUpgrader.h"

#include "UnrealLevelImporterActorUpgrader.generated.h"

/**
* @class UUnrealLevelImporterActorUpgrader
*
* @todo Handle these on the base object
* - setting root component
* - setting location and rotation on the root component
*/
UCLASS()
class UUnrealLevelImporterActorUpgrader : public UUnrealLevelImporterBaseUpgrader
{
	GENERATED_BODY()
public:
	virtual bool Upgrade(FUnrealLevelImporterObject& ImporterObject);

protected:
	/** Gets the location and rotation, removes the old keys from the object and converts them from UE3 style. If key doesn't exist, it sets a default value. */
	bool ConvertTransform(FUnrealLevelImporterObject& Object, FVector& OutLoc, FRotator& OutRot, FVector& OutScale);

	/** Takes the transform properties from one object and assigns them to another*/
	bool ConvertAndAssignTransform(FUnrealLevelImporterObject& FromObject, FUnrealLevelImporterObject& ToObject);

	/** Returns DrawScale3D * DrawScale as an FVector*/
	FVector GetCorrectedScale(FUnrealLevelImporterObject& Object);
};
