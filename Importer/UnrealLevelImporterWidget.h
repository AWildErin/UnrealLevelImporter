#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FUnrealLevelImporterObject;
class UUnrealLevelImporterBaseUpgrader;

/**
 * @class SUnrealLevelImporterWidget
 *
 * @todo Maybe we should extract the importer code to a separate object or something like that
 * @todo Handle remapping objects
 * @todo Handle ignoring objects
 * @todo Handle upgrading objects
 * @todo Creation todo:
 * @todo Handle archetypes
 * @todo Handle name
 * @todo Groups? Not sure if they get exported from UE3
 * @todo Handle getting world. We use GWorld but not sure if that's ideal.
 * @todo Need to handle terrain, polylist and other minor Begin Objects. We can just pass a title to ParseObject
 * @todo Need to handle actor labels. They do not update.
 */
class SUnrealLevelImporterWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUnrealLevelImporterWidget) {}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

private:
	bool IsIgnoredObject(const FString& ObjectClass);

	bool ParseActorT3D(const TCHAR* Header, const TCHAR** Buffer, FUnrealLevelImporterObject& OutObject);
	bool ParseObjectT3D(const TCHAR* Header, const TCHAR** Buffer, FUnrealLevelImporterObject& OutObject);

	FReply ImportMap();

	bool ParseObjects(const FString& FilePath);
	/** @todo Handle updating properties */
	bool RemapObjects();
	bool UpgradeObjects();
	bool ImportActors();

	TArray<FUnrealLevelImporterObject> Objects;

	TMap<FString, TSubclassOf<UUnrealLevelImporterBaseUpgrader>> UpgraderClasses;
};

// T3D Structs
struct FUnrealLevelImporterObject
{
	FString ClassName;

	// Object Name
	FString Name;

	// Object Template
	FString Archetype;

	// Only valid for subobjects. ODes not seem to be used in UE4+
	FString ObjName;

	// Underlying type, usually either Actor or Object
	FString Type;

	// Header for the actual T3D that contains the properties above
	FString T3DHeader;

	// Giant string containing the custom properties for the object
	FString CustomProperties;

	// Child objects
	/** @todo Make this a TMap somehow */
	TArray<FUnrealLevelImporterObject> SubObjects;

	/** @todo We need to use a multimap here because of Polygon and wanting to reuse this struct, but it makes the code ugly :( */
	TMultiMap<FString, FString> Properties;

	// Turns the object back into a T3D
	FString ReconstructT3D(int Indent = 0, bool bWriteRootHeader = true);

	FUnrealLevelImporterObject* GetSubObjectByName(const FString& NameToSearch);
	FUnrealLevelImporterObject* GetSubObjectByObjName(const FString& NameToSearch);

	// General creators
	static FUnrealLevelImporterObject MakeNoteFromActor(FUnrealLevelImporterObject& Actor);
	static FUnrealLevelImporterObject MakeNote(const FString& NoteText);

	static FUnrealLevelImporterObject CreateFromObject(UObject* Object);
};

// Operators
bool operator==(const FUnrealLevelImporterObject& Left, const FUnrealLevelImporterObject& Right);
