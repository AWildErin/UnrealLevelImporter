#include "Importer/Upgraders/UnrealLevelImporterNoteUpgrader.h"

#include "Importer/UnrealLevelImporterUtils.h"
#include "Importer/UnrealLevelImporterWidget.h"

bool UUnrealLevelImporterNoteUpgrader::Upgrade(FUnrealLevelImporterObject& ImporterObject)
{
	if (!Super::Upgrade(ImporterObject))
	{
		return false;
	}

	// Remove old components
	// todo:
	// tags
	// name
	ImporterObject.SubObjects.Empty();
	ImporterObject.Properties.Remove("Components(0)");
	ImporterObject.Properties.Remove("Components(1)");
	ImporterObject.Properties.Remove("ObjectArchetype");

	// Add scene component
	FUnrealLevelImporterObject SceneComp;
	SceneComp.Type = "Object";
	SceneComp.ClassName = "SceneComponent";
	SceneComp.Name = "SceneComp";
	ConvertAndAssignTransform(ImporterObject, SceneComp);

	ImporterObject.SubObjects.Add(SceneComp);

	ImporterObject.Properties.Add("RootComponent", "\"SceneComp\"");

	return true;
}
