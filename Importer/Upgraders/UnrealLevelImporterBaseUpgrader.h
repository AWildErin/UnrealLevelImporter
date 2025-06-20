#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "UnrealLevelImporterBaseUpgrader.generated.h"

struct FUnrealLevelImporterObject;

UCLASS()
class UUnrealLevelImporterBaseUpgrader : public UObject
{
	GENERATED_BODY()

public:
	virtual bool Upgrade(FUnrealLevelImporterObject& ImporterObject) PURE_VIRTUAL(UMyBaseClass::Upgrade, return false;);
};
