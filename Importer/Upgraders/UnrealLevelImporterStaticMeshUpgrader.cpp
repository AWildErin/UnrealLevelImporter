#include "Importer/Upgraders/UnrealLevelImporterStaticMeshUpgrader.h"

#include "Logging/StructuredLog.h"

#include "Importer/UnrealLevelImporterUtils.h"
#include "Importer/UnrealLevelImporterWidget.h"

bool UUnrealLevelImporterStaticMeshUpgrader::Upgrade(FUnrealLevelImporterObject& ImporterObject)
{
	if (!Super::Upgrade(ImporterObject))
	{
		return false;
	}

	FUnrealLevelImporterObject* OriginalComp = ImporterObject.GetSubObjectByName("StaticMeshComponent0");

	// Remove sub objects, this is usually shadow maps which UE5 trips over
	OriginalComp->SubObjects.Empty();

	// Remap the mesh path
	FString* OldMeshPath = OriginalComp->Properties.Find("StaticMesh");
	if (OldMeshPath)
	{
		FString NewMeshPath = FUnrealLevelImporterUtils::GetRemappedPackagePath(*OldMeshPath);
		OriginalComp->Properties.Remove("StaticMesh");
		OriginalComp->Properties.Add("StaticMesh", NewMeshPath);
	}

	ReplaceMaterials(*OriginalComp);
	ConvertAndAssignTransform(ImporterObject, *OriginalComp);

	// MergeVertexPaint(*OriginalComp);
	// ModelComp.CustomProperties = OriginalComp->CustomProperties;

	// Clear old properties
	ImporterObject.Properties.Remove("Components(0)");
	ImporterObject.Properties.Remove("Components(1)");
	ImporterObject.Properties.Remove("ObjectArchetype");
	ImporterObject.Properties.Remove("StaticMeshComponent");

	// Add our new component
	ImporterObject.Properties.Add("RootComponent", "\"StaticMeshComponent0\"");
	ImporterObject.Properties.Add("StaticMeshComponent", "\"StaticMeshComponent0\"");

	return true;
}

void UUnrealLevelImporterStaticMeshUpgrader::MergeVertexPaint(FUnrealLevelImporterObject& MeshComp)
{
	int32 LodIndex = -1;
	const TCHAR* ColorVertexData = nullptr;
	int32 ColorVertexDataCount = -1;

	// Collect the data from the existing CustomLODData
	const TCHAR* RawString = *MeshComp.CustomProperties;
	if (FParse::Command(&RawString, TEXT("CustomLODData")))
	{
		if (!FParse::Value(RawString, TEXT("LOD="), LodIndex))
		{
			UE_LOGFMT(LogUnrealImporter, Error, "Invalid CustomLODData on {0}!", MeshComp.Name);
			return;
		}

		ColorVertexData = FCString::Stristr(RawString, TEXT("ColorVertexData"));
		if (ColorVertexData == nullptr)
		{
			UE_LOGFMT(LogUnrealImporter, Error, "Invalid CustomLODData on {0}!", MeshComp.Name);
			return;
		}

		// Get the number of verts
		FParse::Value(RawString, TEXT("ColorVertexData("), ColorVertexDataCount);
	}
	else
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Missing CustomLODData block on {0}!", MeshComp.Name);
		return;
	}

	// Get the painted vertices from LODData
	FString LodDataKey = FString::Printf(TEXT("LODData(%d)"), LodIndex);
	FString* LodDataValuePtr = MeshComp.Properties.Find(LodDataKey);
	check(LodDataValuePtr);

	// The rest of this code is messy, but it works

	// Chop the first and last ( )
	FString LodDataValue = *LodDataValuePtr;
	LodDataValue.LeftChopInline(1);
	LodDataValue.RightChopInline(1);

	// Get just the Painted Verts string
	const TCHAR* PaintedVerticesRaw = FCString::Stristr(*LodDataValue, TEXT("PaintedVertices"));
	check(PaintedVerticesRaw);

	// Get the index of the first =, and inject (ColorVertexDataCount) before it
	auto PaintedVerticesRawData = FCString::Strchr(PaintedVerticesRaw, '=');
	FString PaintedVertsWithCount = FString::Printf(TEXT("PaintedVertices(%d)%s"), ColorVertexDataCount, FCString::Strchr(PaintedVerticesRaw, '='));

	// Set the data back to custom properties
	/// @todo Can we maybe not recreate this and instead just pull it from the string before?
	FString LodText = FString::Printf(TEXT(" LOD=%d "), LodIndex);
	MeshComp.CustomProperties = TEXT("CustomLODData") + LodText + PaintedVertsWithCount + TEXT(" ") + ColorVertexData;
}

TMap<FString, FString> UUnrealLevelImporterStaticMeshUpgrader::GetMaterials(FUnrealLevelImporterObject& Object)
{
	TMap<FString, FString> Materials;

	for (TPair<FString, FString>& Prop : Object.Properties)
	{
		if (!Prop.Key.StartsWith(TEXT("Materials(")))
		{
			continue;
		}

		FString Key = TEXT("Override") + Prop.Key;

		if (Prop.Value == TEXT("None"))
		{
			Materials.Add(Key, TEXT("None"));
		}
		else
		{
			Materials.Add(Key, FUnrealLevelImporterUtils::GetRemappedPackagePath(Prop.Value));
		}
	}

	return Materials;
}

void UUnrealLevelImporterStaticMeshUpgrader::ReplaceMaterials(FUnrealLevelImporterObject& Object)
{
	TMultiMap<FString, FString> Materials;

	for (TPair<FString, FString>& Prop : Object.Properties)
	{
		if (!Prop.Key.StartsWith(TEXT("Materials(")))
		{
			continue;
		}

		FString Key = TEXT("Override") + Prop.Key;

		if (Prop.Value == TEXT("None"))
		{
			Materials.Add(Key, TEXT("None"));
		}
		else
		{
			Materials.Add(Key, FUnrealLevelImporterUtils::GetRemappedPackagePath(Prop.Value));
		}
	}

	for (auto It = Object.Properties.CreateIterator(); It; ++It)
	{
		if (It.Key().StartsWith(TEXT("Materials")))
		{
			It.RemoveCurrent();
		}
	}

	Object.Properties.Append(Materials);
}
