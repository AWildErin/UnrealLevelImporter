#include "Importer/Upgraders/UnrealLevelImporterPrefabInstanceUpgrader.h"

#include "Importer//UnrealLevelImporterUtils.h"
#include "Importer/UnrealLevelImporterWidget.h"

bool UUnrealLevelImporterPrefabInstanceUpgrader::Upgrade(FUnrealLevelImporterObject& ImporterObject)
{
	// if (!Super::Upgrade(ImporterObject))
	//{
	//	return false;
	// }

	// Create a note in it's place with the respective properties.
	/** @todo Maybe make this a function too, because there will be others we need to do this for */

	FString OldName = ImporterObject.Name;
	FString NewName = FString::Format(TEXT("Note_{0}"), {ImporterObject.Name});

	// @todo handle if thesse doesn't exist.
	FString TemplatePrefab = "";
	if (auto TemplatePrefabPtr = ImporterObject.Properties.Find("TemplatePrefab"))
	{
		TemplatePrefab = *TemplatePrefabPtr;
		ImporterObject.Properties.Remove("TemplatePrefab");
	}

	FUnrealLevelImporterObject NewObject;
	NewObject.Type = "Actor";
	NewObject.ClassName = "/Script/Engine.Note";
	NewObject.Name = OldName;
	NewObject.Archetype = "/Script/Engine.Note'/Script/Engine.Default__Note'";

	// Add scene component
	FUnrealLevelImporterObject SceneComp;
	SceneComp.Type = "Object";
	SceneComp.ClassName = "SceneComponent";
	SceneComp.Name = "SceneComp";
	ConvertAndAssignTransform(ImporterObject, SceneComp);

	NewObject.SubObjects.Add(SceneComp);

	NewObject.Properties.Add("RootComponent", "\"SceneComp\"");
	NewObject.Properties.Add("ActorLabel", FString::Format(TEXT("\"{0}\""), {OldName})); // This kinda sucks. Would be nice to make something so we didn't always have to do \" ourselves
	NewObject.Properties.Add("Text", FString::Format(TEXT("\"Prefab {0}\""), {TemplatePrefab})); // And this too

	ImporterObject = NewObject;

	return true;
}
