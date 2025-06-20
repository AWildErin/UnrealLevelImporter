#pragma once

#include "CoreMinimal.h"

#include "Importer/Upgraders/UnrealLevelImporterActorUpgrader.h"

#include "UnrealLevelImporterGenericUpgrader.generated.h"

UCLASS()
class UUnrealLevelImporterGenericUpgrader : public UUnrealLevelImporterActorUpgrader
{
	GENERATED_BODY()
public:
	virtual bool Upgrade(FUnrealLevelImporterObject& ImporterObject);
};
