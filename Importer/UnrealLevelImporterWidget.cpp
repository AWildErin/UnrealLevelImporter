#include "Importer/UnrealLevelImporterWidget.h"

#include "SlateOptMacros.h"
#include "Logging/StructuredLog.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "BSPOps.h"
#include "Editor.h"
#include "Components/BrushComponent.h"

#include "Importer/UnrealLevelImporterUtils.h"
#include "Importer/UnrealLevelImporterSettings.h"

// Upgraders
#include "Importer/Upgraders/UnrealLevelImporterActorUpgrader.h"
#include "Importer/Upgraders/UnrealLevelImporterGenericUpgrader.h"
#include "Importer/Upgraders/UnrealLevelImporterNoteUpgrader.h"
#include "Importer/Upgraders/UnrealLevelImporterPrefabInstanceUpgrader.h"
#include "Importer/Upgraders/UnrealLevelImporterVolumeUpgrader.h"
#include "Importer/Upgraders/UnrealLevelImporterStaticMeshUpgrader.h"
#include "Importer/Upgraders/UnrealLevelImporterLightUpgrader.h"

using namespace FUnrealLevelImporterUtils;

// We have to use regex for this because there is an undeterminable amount of spaces between the key and the value
const static FRegexPattern SpaceSplitPropertyRegex(TEXT("(\\w+)\\s+([\\+\\-0-9.,]+)"));

#define LOCTEXT_NAMESPACE "UnrealLevelImporter"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SUnrealLevelImporterWidget::Construct(const FArguments& InArgs)
{
	/** @todo find a better place to store these. maybe just on the settings? Also we should naybe make the class the key, and have an array for the objects it uses */
	UpgraderClasses.Add("Note", UUnrealLevelImporterNoteUpgrader::StaticClass());
	UpgraderClasses.Add("PrefabInstance", UUnrealLevelImporterPrefabInstanceUpgrader::StaticClass());
	UpgraderClasses.Add("BlockingVolume", UUnrealLevelImporterVolumeUpgrader::StaticClass());
	UpgraderClasses.Add("PathBlockingVolume", UUnrealLevelImporterVolumeUpgrader::StaticClass());
	UpgraderClasses.Add("Brush", UUnrealLevelImporterVolumeUpgrader::StaticClass());
	UpgraderClasses.Add("StaticMeshActor", UUnrealLevelImporterStaticMeshUpgrader::StaticClass());

	// All the various lights
	UpgraderClasses.Add("PointLight", UUnrealLevelImporterLightUpgrader::StaticClass());
	UpgraderClasses.Add("DominantPointLight", UUnrealLevelImporterLightUpgrader::StaticClass());
	UpgraderClasses.Add("PointLightMovable", UUnrealLevelImporterLightUpgrader::StaticClass());
	UpgraderClasses.Add("PointLightToggleable", UUnrealLevelImporterLightUpgrader::StaticClass());
	UpgraderClasses.Add("SpotLight", UUnrealLevelImporterLightUpgrader::StaticClass());
	UpgraderClasses.Add("DominantSpotLight", UUnrealLevelImporterLightUpgrader::StaticClass());
	UpgraderClasses.Add("SpotLightMovable", UUnrealLevelImporterLightUpgrader::StaticClass());
	UpgraderClasses.Add("SpotLightToggleable", UUnrealLevelImporterLightUpgrader::StaticClass());
	UpgraderClasses.Add("SkyLight", UUnrealLevelImporterLightUpgrader::StaticClass());
	UpgraderClasses.Add("SkyLightToggleable", UUnrealLevelImporterLightUpgrader::StaticClass());
	UpgraderClasses.Add("DirectionalLight", UUnrealLevelImporterLightUpgrader::StaticClass());
	UpgraderClasses.Add("DirectionalLightToggleable", UUnrealLevelImporterLightUpgrader::StaticClass());
	UpgraderClasses.Add("DominantDirectionalLight", UUnrealLevelImporterLightUpgrader::StaticClass());
	UpgraderClasses.Add("DominantDirectionalLightMovable", UUnrealLevelImporterLightUpgrader::StaticClass());

	// clang-format off
	ChildSlot
	[
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Text(LOCTEXT("ImportButtonText", "Import Button"))
			.ToolTipText(LOCTEXT("ImportButtonToolTip", "Import Button"))
			.HAlign(HAlign_Center)
			.OnClicked(this, &SUnrealLevelImporterWidget::ImportMap)
		]
	];
	// clang-format on
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

bool SUnrealLevelImporterWidget::IsIgnoredObject(const FString& ObjectClass)
{
	/// @todo Store these on the class
	TArray<FString> IgnoredObjects;
	IgnoredObjects.Add("MapPackage");
	IgnoredObjects.Add("TopLevelPackage");

	return IgnoredObjects.Contains(ObjectClass);
}

bool SUnrealLevelImporterWidget::ParseActorT3D(const TCHAR* Header, const TCHAR** Buffer, FUnrealLevelImporterObject& OutObject)
{
	if (!FParse::Value(Header, TEXT("CLASS="), OutObject.ClassName))
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Failed to extract classname!");
		return false;
	}

	if (!FParse::Value(Header, TEXT("NAME="), OutObject.Name))
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Failed to extract name!");
		return false;
	}

	// Some of these are optional depending on the object type. 
	FParse::Value(Header, TEXT("OBJNAME="), OutObject.ObjName);
	FParse::Value(Header, TEXT("ARCHETYPE="), OutObject.Archetype);

	OutObject.Type = "Actor";

	FString Line;
	while (FParse::Line(Buffer, Line))
	{
		const TCHAR* LineRawStr = *Line;

		if (ParseEnd(&LineRawStr, TEXT("ACTOR")))
		{
			break;
		}

		// Sub objects
		if (FParse::Command(&LineRawStr, TEXT("BEGIN")))
		{
			FUnrealLevelImporterObject SubObj;
			// Pass the line in directly as the header so we can extract the object type for the object
			if (!ParseObjectT3D(*Line, Buffer, SubObj))
			{
				UE_LOGFMT(LogUnrealImporter, Error, "Failed to parse sub object on: {str}", OutObject.ClassName);
				return false;
			}

			OutObject.SubObjects.Add(SubObj);
			continue;
		}

		Line = Line.TrimStartAndEnd();

		// Properties
		FString Key;
		FString Value;

		if (!Line.Split("=", &Key, &Value))
		{
			UE_LOGFMT(LogUnrealImporter, Error, "Failed to split string: {str}", Line);
			return false;
		}

		OutObject.Properties.Add(Key, Value);
	}

	return true;
}

bool SUnrealLevelImporterWidget::ParseObjectT3D(const TCHAR* Header, const TCHAR** Buffer, FUnrealLevelImporterObject& OutObject)
{
	if (!FParse::Value(Header, TEXT("BEGIN "), OutObject.Type))
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Failed to extract object type!");
		return false;
	}

	// Some of these are optional depending on the object type.
	// - These are all optional on Polygon and Polylist
	// - ObjName and Archetype are optional on general objects, as the engine will figure them out for us.
	FParse::Value(Header, TEXT("CLASS="), OutObject.ClassName);
	FParse::Value(Header, TEXT("NAME="), OutObject.Name);
	FParse::Value(Header, TEXT("OBJNAME="), OutObject.ObjName);
	FParse::Value(Header, TEXT("ARCHETYPE="), OutObject.Archetype);

	// Validate which are optional
	if (OutObject.Type == "Object" && (OutObject.ClassName.IsEmpty() || OutObject.Name.IsEmpty()))
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Object was missing Class or Name. Header: {header}", Header);
		UE_DEBUG_BREAK();
		return false;
	}
	else if (OutObject.Type == "Model" && (OutObject.Name.IsEmpty()))
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Model was missing Name. Header: {header}", Header);
		UE_DEBUG_BREAK();
		return false;
	}

	bool bPropertySplitBySpaces = OutObject.Type == "Polygon";

	FString Line;
	while (FParse::Line(Buffer, Line))
	{
		const TCHAR* LineRawStr = *Line;

		// Exit out if we're parsing the end block for our type
		if (ParseEnd(&LineRawStr, *OutObject.Type.ToUpper()))
		{
			break;
		}

		// Sub objects
		if (FParse::Command(&LineRawStr, TEXT("BEGIN")))
		{
			FUnrealLevelImporterObject SubObj;
			// Pass the line in directly as the header so we can extract the object type for the object
			if (!ParseObjectT3D(*Line, Buffer, SubObj))
			{
				UE_LOGFMT(LogUnrealImporter, Error, "Failed to parse sub object on: {str}", OutObject.ClassName);
				return false;
			}

			OutObject.SubObjects.Add(SubObj);
			continue;
		}

		if (FParse::Command(&LineRawStr, TEXT("CUSTOMPROPERTIES")))
		{
			OutObject.CustomProperties = LineRawStr;
			continue;
		}

		Line = Line.TrimStartAndEnd();

		// Properties
		FString Key;
		FString Value;

		// We need to handle this differently depending on the object. Polygons for example are split by spaces, and not =
		if (bPropertySplitBySpaces)
		{
			FRegexMatcher RegexMatcher(SpaceSplitPropertyRegex, Line);

			if (RegexMatcher.FindNext())
			{
				Key = RegexMatcher.GetCaptureGroup(1);
				Value = RegexMatcher.GetCaptureGroup(2);
			}
			else
			{
				UE_LOGFMT(LogUnrealImporter, Error, "Failed to split string: {str}", Line);
				return false;
			}
		}
		else if (!Line.Split("=", &Key, &Value))
		{
			UE_LOGFMT(LogUnrealImporter, Error, "Failed to split string: {str}", Line);
			return false;
		}

		OutObject.Properties.Add(Key, Value);
	}

	return true;
}

FReply SUnrealLevelImporterWidget::ImportMap()
{
	UE_LOGFMT(LogUnrealImporter, Log, "Hello world!");

	Objects.Empty();

	FString FilePath = "S:/LevelImporter/MyMap.t3d";

	if (!ParseObjects(FilePath))
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Failed to upgrade objects.");
		return FReply::Handled();
	}

	// Remap any class names and properties. (properties are not remapped yet. todo!)
	if (!RemapObjects())
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Failed to remap objects.");
		return FReply::Handled();
	}

	// Upgrade objects to UE4+ styled T3Ds
	if (!UpgradeObjects())
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Failed to upgrade objects.");
		return FReply::Handled();
	}

	// Import objects into map
	if (!ImportActors())
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Failed to import actors into level.");
		return FReply::Handled();
	}

	return FReply::Handled();
}


bool SUnrealLevelImporterWidget::ParseObjects(const FString& FilePath)
{
	FString LoadedText;
	if (!FFileHelper::LoadFileToString(LoadedText, *FilePath))
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Failed to load file \"{name}\"", FilePath);
		return false;
	}

	const TCHAR* Buffer = *LoadedText;
	FParse::Next(&Buffer);

	if (!ParseBegin(&Buffer, TEXT("MAP")))
	{
		UE_LOGFMT(LogUnrealImporter, Error, "File did not contain map!", FilePath);
		return false;
	}
	else
	{
		FString NameValue;
		if (FParse::Value(Buffer, TEXT("Name="), NameValue))
		{
			UE_LOGFMT(LogUnrealImporter, Log, "Import map: {name}", NameValue);
		}
	}

	FString StrLine;
	// Parse the T3D and fill the data
	while (FParse::Line(&Buffer, StrLine))
	{
		const TCHAR* Str = *StrLine;

		if (ParseBegin(&Str, TEXT("ACTOR")))
		{
			FUnrealLevelImporterObject Object;
			if (!ParseActorT3D(Str, &Buffer, Object))
			{
				UE_LOGFMT(LogUnrealImporter, Error, "Failed to parse actor!");
				break;
			}

			Objects.Add(Object);
		}
	}

	return true;
}

bool SUnrealLevelImporterWidget::RemapObjects()
{
	const UUnrealLevelImporterSettings* Settings = UUnrealLevelImporterSettings::Get();

	for (auto& Remap : Settings->ClassRemapping)
	{
		FString ClassName = Remap.Key;
		const FUnrealLevelImporterRemap& RemapData = Remap.Value;

		for (auto& Object : Objects)
		{
			if (Object.ClassName != ClassName || RemapData.Class == nullptr)
			{
				continue;
			}

			Object.ClassName = RemapData.Class->GetClassPathName().ToString();

			if (!Object.Archetype.IsEmpty())
			{
				Object.Archetype = RemapData.Class->GetDefaultObject()->GetFullName();
			}
		}
	}

	return true;
}

bool SUnrealLevelImporterWidget::UpgradeObjects()
{
	// Upgrade Objects
	/** @todo Should we return false if we can't upgrade any object? */
	for (auto& Object : Objects)
	{
		TObjectPtr<UClass> UpgraderClass = nullptr;
		TSubclassOf<UUnrealLevelImporterBaseUpgrader>* UpgraderObjectClass = UpgraderClasses.Find(Object.ClassName);
		if (!UpgraderObjectClass)
		{
			UE_LOGFMT(LogUnrealImporter, Warning, "Failed to find upgrader for {class}. Using generic actor importer instead", Object.ClassName);
			UpgraderClass = UUnrealLevelImporterGenericUpgrader::StaticClass();
		}
		else
		{
			UpgraderClass = *UpgraderObjectClass;
		}

		if (!UpgraderClass)
		{
			UE_LOGFMT(LogUnrealImporter, Error, "Failed to find upgrader for {class}.", Object.ClassName);
			continue;
		}

		TObjectPtr<UUnrealLevelImporterBaseUpgrader> Upgrader = NewObject<UUnrealLevelImporterBaseUpgrader>(GWorld, UpgraderClass);
		if (!Upgrader)
		{
			UE_LOGFMT(LogUnrealImporter, Error, "Failed to construct upgrader class for {class}. Wanted to use {upgrader}.", Object.ClassName, GetNameSafe(UpgraderClass));
			continue;
		}

		// Could we upgrade the object? It's fine to carry on if not.
		if (!Upgrader->Upgrade(Object))
		{
			UE_LOGFMT(LogUnrealImporter, Error, "Failed to upgrade class {class}.", Object.ClassName);
			return false;
		}

		// Clear up upgrader object
		Upgrader->MarkAsGarbage();
		Upgrader = nullptr;
	}

	return true;
}

bool SUnrealLevelImporterWidget::ImportActors()
{
	GEditor->IsImportingT3D = true;
	GIsImportingT3D = true;

	// Import objects
	for (auto& Object : Objects)
	{
		FString T3DText = Object.ReconstructT3D(0);

		if (Object.ClassName == "DecalActor")
		{
			continue;
		}

		UClass* Class = nullptr;
		if (!ParseObject<UClass>(*T3DText, TEXT("CLASS="), Class, nullptr, EParseObjectLoadingPolicy::FindOrLoad))
		{
			UE_LOGFMT(LogUnrealImporter, Warning, "Failed to load/find class for {name}. Replacing it with note.", Object.Name);
			Object = FUnrealLevelImporterObject::MakeNoteFromActor(Object);

			//T3DText = Object.ReconstructT3D();
			//if (!ParseObject<UClass>(*T3DText, TEXT("CLASS="), Class, nullptr, EParseObjectLoadingPolicy::FindOrLoad))
			//{
			//	UE_LOGFMT(LogUnrealImporter, Error, "Failed to load/find class for {name} even after making a note for it!", Object.Name);
			//	continue;
			//}
		}

		//FString ObjectClass;
		//FString ObjectPath;
		//FPackageName::ParseExportTextPath("/Script/Engine.BlockingVolume", &ObjectClass, &ObjectPath);
		//UClass* ArchetypeClass = UClass::TryFindTypeSlow<UClass>(ObjectClass, EFindFirstObjectOptions::EnsureIfAmbiguous);
		//AActor* Archetype = Cast<AActor>(StaticFindObject(ArchetypeClass, nullptr, *ObjectPath));

		FActorSpawnParameters Params;
		Params.Name = FName(Object.Name);
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		//Params.Template = Archetype; // Object.Archetype // Need to upgrade to UE4 flavoured archetypes
		AActor* NewActor = GWorld->SpawnActor(Class, nullptr, nullptr, Params);
		if (!NewActor)
		{
			UE_LOGFMT(LogUnrealImporter, Error, "Failed to import {name}! See output log for more info.", Object.Name);
			continue;
		}

		FImportObjectParams ImportParams;
		ImportParams.DestData = reinterpret_cast<uint8*>(NewActor);
		ImportParams.SourceText = *T3DText;
		ImportParams.ObjectStruct = NewActor->GetClass();
		ImportParams.SubobjectRoot = NewActor;
		ImportParams.SubobjectOuter = NewActor;
		ImportParams.Warn = GWarn;
		ImportParams.Depth = 0;
		ImportParams.LineNumber = 0;
		ImportParams.bShouldCallEditChange = true;

		ImportObjectProperties(ImportParams);

		// Label won't update if we do it via the import
		/// @todo How can we better do this, it's annoying the actor label doesn't properly update if you do it via t3d import
		FString ActorLabel = NewActor->GetActorLabel();
		NewActor->SetActorLabel("IntermimName");
		NewActor->SetActorLabel(ActorLabel);

		// This is needed for the bottom code, no idea why.
		GEditor->SelectActor(NewActor, true, false, true, true);

		// Rebuild or revalidate the brushes.
		TObjectPtr<ABrush> Brush = Cast<ABrush>(NewActor);
		if (Brush)
		{
			const bool bIsStaticBrush = Brush->IsStaticBrush();
			if (!bIsStaticBrush)
			{
				FBSPOps::RebuildBrush(Brush->Brush);
			}
		
			FBSPOps::bspValidateBrush(Brush->Brush, true, false);

			Brush->GetBrushComponent()->Brush = Brush->Brush;
		}

	}


	GEditor->IsImportingT3D = 0;
	GIsImportingT3D = false;

	return true;
}

FString FUnrealLevelImporterObject::ReconstructT3D(int Indent /*= 0*/, bool bWriteRootHeader /*= true*/)
{
	const FString NewLine = TEXT("\r\n");

	FString IndentString = "";

	for (int i = 0; i < Indent; i++)
	{
		IndentString += "\t";
	}

	FString T3DString;

	if (bWriteRootHeader)
	{
		T3DString += FString::Format(TEXT("{0}Begin {1}"), {IndentString, Type});

		if (!ClassName.IsEmpty())
		{
			T3DString += FString::Format(TEXT(" Class={0}"), {ClassName});
		}

		if (!Name.IsEmpty())
		{
			T3DString += FString::Format(TEXT(" Name={0}"), {Name});
		}

		if (!ObjName.IsEmpty())
		{
			T3DString += FString::Format(TEXT(" ObjName={0}"), {ObjName});
		}

		if (!Archetype.IsEmpty())
		{
			T3DString += FString::Format(TEXT(" Archetype={0}"), {Archetype});
		}

		T3DString += NewLine;
	}

	// Write sub objects into the current t3d
	for (auto& SubObj : SubObjects)
	{
		T3DString += SubObj.ReconstructT3D(Indent + 1);
		T3DString += NewLine;
	}

	bool bPropertySplitBySpaces = Type == "Polygon";

	// Write the properies
	for (auto& Prop : Properties)
	{
		if (bPropertySplitBySpaces)
		{
			T3DString += FString::Format(TEXT("{0}\t{1} {2}"), {IndentString, Prop.Key, Prop.Value});
		}
		else
		{
			T3DString += FString::Format(TEXT("{0}\t{1}={2}"), {IndentString, Prop.Key, Prop.Value});
		}
		T3DString += NewLine;
	}

	if (bWriteRootHeader)
	{
		T3DString += FString::Format(TEXT("{0}End {1}"), {IndentString, Type});
	}

	return T3DString;
}

FUnrealLevelImporterObject* FUnrealLevelImporterObject::GetSubObjectByName(const FString& NameToSearch)
{
	FUnrealLevelImporterObject* Object = SubObjects.FindByPredicate([&](const FUnrealLevelImporterObject& Object) { return Object.Name == NameToSearch; });
	return Object;
}

FUnrealLevelImporterObject* FUnrealLevelImporterObject::GetSubObjectByObjName(const FString& NameToSearch)
{
	FUnrealLevelImporterObject* Object = SubObjects.FindByPredicate([&](const FUnrealLevelImporterObject& Object) { return Object.ObjName == NameToSearch; });
	return Object;
}

FUnrealLevelImporterObject FUnrealLevelImporterObject::MakeNoteFromActor(FUnrealLevelImporterObject& Actor)
{
	const FString NewLine = TEXT("\\r\\n");

	FUnrealLevelImporterObject Object;

	// Get translation data from root object if it exists
	FString Location;
	FString Rotation;

	// Check to see if this has been upgraded
	if (auto RootCompName = Actor.Properties.Find("RootComponent"))
	{
		FUnrealLevelImporterObject* RootComponent = Actor.GetSubObjectByName(*RootCompName->TrimQuotes());
		if (RootComponent)
		{
			if (auto LocationPtr = RootComponent->Properties.Find("RelativeLocation"))
			{
				Location = *LocationPtr;
			}

			if (auto RotationPtr = RootComponent->Properties.Find("RelativeRotation"))
			{
				Rotation = *RotationPtr;
				Rotation = FUnrealLevelImporterUtils::FixupRotatorString(Rotation);
			}
		}
	}
	// Try the current actor for the legacy properties.
	else
	{
		if (auto LocationPtr = Actor.Properties.Find("Location"))
		{
			Location = *LocationPtr;
		}

		if (auto RotationPtr = Actor.Properties.Find("Rotation"))
		{
			Rotation = *RotationPtr;
			Rotation = FUnrealLevelImporterUtils::FixupRotatorString(Rotation);
		}
	}

	Object.ClassName = "Note";
	Object.Name = Actor.Name;
	Object.Type = "Actor";

	FUnrealLevelImporterObject SceneComp;
	SceneComp.Type = "Object";
	SceneComp.ClassName = "SceneComponent";
	SceneComp.Name = "SceneComp";
	SceneComp.Properties.Add("RelativeLocation", Location);
	SceneComp.Properties.Add("RelativeRotation", Rotation);

	Object.SubObjects.Add(SceneComp);
	Object.Properties.Add("RootComponent", "\"SceneComp\"");
	Object.Properties.Add("ActorLabel", FString::Format(TEXT("\"{0}\""), {Actor.Name}));

	// Only add the properties from the root object.

	FString NoteText = "\"Properties for Root Object." + NewLine + NewLine;

	for (auto& Prop : Actor.Properties)
	{
		NoteText += FString::Format(TEXT("{0}={1}"), {Prop.Key.ReplaceQuotesWithEscapedQuotes(), Prop.Value.ReplaceQuotesWithEscapedQuotes()});
		NoteText += NewLine;
	}

	NoteText += "\"";

	Object.Properties.Add("Text", NoteText);

	return Object;
}

FUnrealLevelImporterObject FUnrealLevelImporterObject::MakeNote(const FString& NoteText)
{
	return FUnrealLevelImporterObject();
}

FUnrealLevelImporterObject FUnrealLevelImporterObject::CreateFromObject(UObject* Object)
{
	FUnrealLevelImporterObject NewObject;

	if (Object == nullptr)
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Passed null object to FUnrealLevelImporterObject::CreateFromObject!");
		return NewObject;
	}

	NewObject.Type = Object->IsA<AActor>() ? "Actor" : "Object";
	NewObject.Name = Object->GetName();
	NewObject.ClassName = Object->GetClass()->GetFullName();
	NewObject.Archetype = Object->GetArchetype()->GetFullName();

	/** @todo Handle subobjects and properties */

	return NewObject;
}

bool operator==(const FUnrealLevelImporterObject& Left, const FUnrealLevelImporterObject& Right)
{
	/// @todo Handle checking properties and sub objects

	// clang-format off
	return	Left.ClassName == Right.ClassName &&
			Left.Name == Right.Name &&
			Left.Archetype == Right.Archetype &&
			Left.ObjName == Right.ObjName &&
			Left.Type == Right.Type &&
			Left.T3DHeader == Right.T3DHeader &&
			Left.CustomProperties == Right.CustomProperties;
	// clang-format on
}

#undef LOCTEXT_NAMESPACE
