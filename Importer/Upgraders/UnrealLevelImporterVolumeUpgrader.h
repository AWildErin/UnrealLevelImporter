#pragma once

#include "CoreMinimal.h"

#include "Importer/Upgraders/UnrealLevelImporterActorUpgrader.h"

#include "UnrealLevelImporterVolumeUpgrader.generated.h"

UCLASS()
class UUnrealLevelImporterVolumeUpgrader : public UUnrealLevelImporterActorUpgrader
{
	GENERATED_BODY()
public:
	virtual bool Upgrade(FUnrealLevelImporterObject& ImporterObject);
};
