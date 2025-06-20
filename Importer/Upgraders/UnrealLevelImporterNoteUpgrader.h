#pragma once

#include "CoreMinimal.h"

#include "Importer/Upgraders/UnrealLevelImporterActorUpgrader.h"

#include "UnrealLevelImporterNoteUpgrader.generated.h"

UCLASS()
class UUnrealLevelImporterNoteUpgrader : public UUnrealLevelImporterActorUpgrader
{
	GENERATED_BODY()
public:
	virtual bool Upgrade(FUnrealLevelImporterObject& ImporterObject);
};
