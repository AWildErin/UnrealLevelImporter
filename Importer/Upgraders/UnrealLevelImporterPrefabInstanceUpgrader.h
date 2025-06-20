#pragma once

#include "CoreMinimal.h"

#include "Importer/Upgraders/UnrealLevelImporterActorUpgrader.h"

#include "UnrealLevelImporterPrefabInstanceUpgrader.generated.h"

UCLASS()
class UUnrealLevelImporterPrefabInstanceUpgrader : public UUnrealLevelImporterActorUpgrader
{
	GENERATED_BODY()
public:
	virtual bool Upgrade(FUnrealLevelImporterObject& ImporterObject);
};
