#include "Importer/Upgraders/UnrealLevelImporterGenericUpgrader.h"

#include "Logging/StructuredLog.h"

#include "Importer//UnrealLevelImporterUtils.h"
#include "Importer/UnrealLevelImporterWidget.h"

bool UUnrealLevelImporterGenericUpgrader::Upgrade(FUnrealLevelImporterObject& ImporterObject)
{
	if (!Super::Upgrade(ImporterObject))
	{
		return false;
	}

	// One time thing, it's fine to just ignore this
	/// @todo Maybe make a global class ignorer?
	if (ImporterObject.ClassName == TEXT("WorldInfo"))
	{
		return true;
	}

	// Get root component
	FString* ComponentsValue = ImporterObject.Properties.Find("Components(0)");
	if (!ComponentsValue)
	{
		// Didn't find a first value, try a second time
		ComponentsValue = ImporterObject.Properties.Find("Components(1)");
		if (!ComponentsValue)
		{
			return false;
		}
	}

	int32 StartIndex = INDEX_NONE;
	ComponentsValue->FindChar('\'',  StartIndex);

	// This doesn't correctly strip the last '
	FString Name = ComponentsValue->Mid(StartIndex+1, ComponentsValue->Len()-(StartIndex));
	Name.LeftChopInline(1);

	FUnrealLevelImporterObject* Object = ImporterObject.GetSubObjectByObjName(Name);
	if (!Object)
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Failed to find sub object with objname '{0}' on {1}", Name, ImporterObject.Name);
		return false;
	}

	ConvertAndAssignTransform(ImporterObject, *Object);
	ImporterObject.Properties.Add("RootComponent", FString::Printf(TEXT("\"%s\""), *Name));

	return true;
}
