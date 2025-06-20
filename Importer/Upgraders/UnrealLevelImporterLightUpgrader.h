#pragma once

#include "CoreMinimal.h"

#include "Importer/Upgraders/UnrealLevelImporterActorUpgrader.h"

#include "UnrealLevelImporterLightUpgrader.generated.h"

enum ELightType : uint8
{
	NoneType = 0,
	Point,
	Spot,
	Directional,
	Sky
};

enum ELightModifierFlags : uint8
{
	NoModifier = 0,
	Toggleable = 1 << 0,
	Movable = 1 << 1,
	Dominant = 1 << 2
};

inline ELightModifierFlags operator|(ELightModifierFlags A, ELightModifierFlags B)
{
	return static_cast<ELightModifierFlags>(static_cast<uint8>(A) | static_cast<uint8>(B));
}

UCLASS()
class UUnrealLevelImporterLightUpgrader : public UUnrealLevelImporterActorUpgrader
{
	GENERATED_BODY()
public:
	virtual bool Upgrade(FUnrealLevelImporterObject& ImporterObject);

private:
	/**
	* Extracts the flag type and modifiers from the class name
	* @returns Whether or not the extraction was successful
	*/
	bool GetTypeAndFlags(FUnrealLevelImporterObject& Obj, ELightType& OutType, ELightModifierFlags& OutFlags);

	void StripModifiersInline(FString& String);

	FString GetLightComponentName(ELightType LightType, ELightModifierFlags Flags);

	// Collapsed into a function to make the main upgrade method easier to read
	void RemoveUnwantedComponents(FUnrealLevelImporterObject& Object);

	void UpgradeSharedProperties(FUnrealLevelImporterObject& Object, ELightType LightType, ELightModifierFlags Flags);
	void UpgradePointLight(FUnrealLevelImporterObject& Object);
	void UpgradeSpotLight(FUnrealLevelImporterObject& Object);
	void UpgradeDirectionalLight(FUnrealLevelImporterObject& Object);
	void UpgradeSkyLight(FUnrealLevelImporterObject& Object);
};
