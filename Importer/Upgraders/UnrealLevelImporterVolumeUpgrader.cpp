#include "Importer/Upgraders/UnrealLevelImporterVolumeUpgrader.h"

#include "Logging/StructuredLog.h"

#include "Importer/UnrealLevelImporterWidget.h"
#include "Importer/UnrealLevelImporterUtils.h"

bool UUnrealLevelImporterVolumeUpgrader::Upgrade(FUnrealLevelImporterObject& ImporterObject)
{
	if (!Super::Upgrade(ImporterObject))
	{
		return false;
	}

	ImporterObject.Properties.Remove("Components(0)");
	ImporterObject.Properties.Remove("ObjectArchetype");
	ImporterObject.Properties.Remove("Model");
	ImporterObject.Properties.Remove("BrushComponent");
	ImporterObject.Properties.Remove("CollisionComponent");

	/** @todo We shouldn't blindly do this, we should verify they exist. */
	FUnrealLevelImporterObject& BrushComponent = ImporterObject.SubObjects[0];
	FUnrealLevelImporterObject& Model = ImporterObject.SubObjects[1];

	// We can assume the brush component name will be BrushComponent0, this is how it's always been for the actual object name, however the ObjName may be different.
	FString QuotedModelName = FString::Format(TEXT("\"{0}\""), {Model.Name});

	BrushComponent.Properties.Remove("Brush");
	ConvertAndAssignTransform(ImporterObject, BrushComponent);

	/** @todo This is ugly because of multimap, not sure the best way to do it */
	ImporterObject.Properties.Remove("Brush");
	ImporterObject.Properties.Add("Brush", QuotedModelName);
	ImporterObject.Properties.Add("BrushComponent", "\"BrushComponent0\"");
	ImporterObject.Properties.Add("RootComponent", "\"BrushComponent0\"");

	return true;
}
