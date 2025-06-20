#pragma once

#include "CoreMinimal.h"

#include "Importer/Upgraders/UnrealLevelImporterActorUpgrader.h"

#include "UnrealLevelImporterStaticMeshUpgrader.generated.h"

UCLASS()
class UUnrealLevelImporterStaticMeshUpgrader : public UUnrealLevelImporterActorUpgrader
{
	GENERATED_BODY()
public:
	virtual bool Upgrade(FUnrealLevelImporterObject& ImporterObject);

private:
	void MergeVertexPaint(FUnrealLevelImporterObject& MeshComp);
	TMap<FString, FString> GetMaterials(FUnrealLevelImporterObject& Object);
	/** Replaces the materials on this object with the correct key and remapped paths */
	void ReplaceMaterials(FUnrealLevelImporterObject& Object);
};
