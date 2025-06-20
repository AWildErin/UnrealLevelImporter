#include "Importer/Upgraders/UnrealLevelImporterLightUpgrader.h"

#include "Logging/StructuredLog.h"

#include "Importer/UnrealLevelImporterUtils.h"
#include "Importer/UnrealLevelImporterWidget.h"
#include "Importer/UnrealLevelImporterSettings.h"

const static FRegexPattern LightNamePattern(TEXT("(Dominant)?(Point|Sky|Spot|Directional)Light(Movable|Toggleable)?"));

// Default brightness for unitless
constexpr float DEFAULT_BRIGHTNESS = 1.f;
constexpr float DEFAULT_RADIUS = 1024.f;

bool UUnrealLevelImporterLightUpgrader::Upgrade(FUnrealLevelImporterObject& ImporterObject)
{
	if (!Super::Upgrade(ImporterObject))
	{
		return false;
	}

	ELightType LightType = ELightType::NoneType;
	ELightModifierFlags Flags = ELightModifierFlags::NoModifier;
	if (!GetTypeAndFlags(ImporterObject, LightType, Flags))
	{
		return false;
	}

	// Strip modifiers from the class and archetype name
	StripModifiersInline(ImporterObject.ClassName);
	StripModifiersInline(ImporterObject.Archetype);
	StripModifiersInline(*ImporterObject.Properties.Find("ObjectArchetype"));

	// Update components list
	RemoveUnwantedComponents(ImporterObject);

	// Update the light component
	FString LightComponentName = GetLightComponentName(LightType, Flags);
	FUnrealLevelImporterObject* LightComponent = ImporterObject.GetSubObjectByName(LightComponentName);
	if (!LightComponent)
	{
		UE_LOGFMT(LogUnrealImporter, Error, "Failed to find component with name {0}", LightComponentName);
		return false;
	}

	StripModifiersInline(LightComponent->ClassName);
	StripModifiersInline(LightComponent->Archetype);
	StripModifiersInline(*LightComponent->Properties.Find("ObjectArchetype"));

	// Update location and rotation values
	ConvertAndAssignTransform(ImporterObject, *LightComponent);

	// Update component references
	/// @todo How do we actually update values?
	ImporterObject.Properties.Remove("LightComponent");
	ImporterObject.Properties.Add("LightComponent", LightComponent->Name);
	ImporterObject.Properties.Add("RootComponent", LightComponent->Name);

	UpgradeSharedProperties(*LightComponent, LightType, Flags);

	// For some reason there are specific properties for the light components depending on type except on SkyLight..
	switch (LightType)
	{
		case Point:
			ImporterObject.Properties.Add("PointLightComponent", LightComponent->Name);
			UpgradePointLight(*LightComponent);
			break;
		case Spot:
			ImporterObject.Properties.Add("SpotLightComponent", LightComponent->Name);
			UpgradeSpotLight(*LightComponent);
			break;
		case Directional:
			ImporterObject.Properties.Add("DirectionalLightComponent", LightComponent->Name);
			UpgradeDirectionalLight(*LightComponent);
			break;
		case Sky:
			UpgradeSpotLight(*LightComponent);
			break;
		default:
			break;
	}

	return true;
}

bool UUnrealLevelImporterLightUpgrader::GetTypeAndFlags(FUnrealLevelImporterObject& Obj, ELightType& OutType, ELightModifierFlags& OutFlags)
{
	FRegexMatcher RegexMatcher(LightNamePattern, Obj.ClassName);
	if (!RegexMatcher.FindNext())
	{
		return false;
	}

	// clang-format off
	TMap<FString, ELightModifierFlags> ModifierToFlag =
	{
		{"Movable", ELightModifierFlags::Movable},
		{"Toggleable", ELightModifierFlags::Toggleable}
	};
	// clang-format on

	FString DominantStr = RegexMatcher.GetCaptureGroup(1);
	FString TypeStr = RegexMatcher.GetCaptureGroup(2);
	FString ModifierStr = RegexMatcher.GetCaptureGroup(3);

	if (auto* Flag = ModifierToFlag.Find(ModifierStr))
	{
		OutFlags = *Flag;
	}
	else if (!ModifierStr.IsEmpty())
	{
		ensureAlwaysMsgf(0, TEXT("Unknown flag string! %s"), *ModifierStr);
		return false;
	}

	if (!DominantStr.IsEmpty())
	{
		OutFlags = OutFlags | ELightModifierFlags::Dominant;
	}

	if (TypeStr == "Point")
	{
		OutType = ELightType::Point;
	}
	else if (TypeStr == "Spot")
	{
		OutType = ELightType::Spot;
	}
	else if (TypeStr == "Directional")
	{
		OutType = ELightType::Directional;
	}
	else if (TypeStr == "Sky")
	{
		OutType = ELightType::Sky;
	}
	else
	{
		ensureAlwaysMsgf(0, TEXT("Unknown type string! %s"), *TypeStr);
		OutType = ELightType::NoneType;
		return false;
	}

	return true;
}

void UUnrealLevelImporterLightUpgrader::StripModifiersInline(FString& String)
{
	String = String.Replace(TEXT("Movable"), TEXT("")).Replace(TEXT("Toggleable"), TEXT("")).Replace(TEXT("Dominant"), TEXT(""));
}

FString UUnrealLevelImporterLightUpgrader::GetLightComponentName(ELightType LightType, ELightModifierFlags Flags)
{
	FString LightName = "";

	switch (LightType)
	{
		case Directional:
			LightName = TEXT("DirectionalLightComponent0");
			break;
		case Spot:
			LightName = TEXT("SpotLightComponent0");
			break;
		case Sky:
			LightName = TEXT("SkyLightComponent0");
			break;
		case Point:
			LightName = TEXT("PointLightComponent0");
			break;
		default:
			LightName = TEXT("");
			break;
	}

	if ((Flags & ELightModifierFlags::Dominant) == ELightModifierFlags::Dominant)
	{
		return TEXT("Dominant") + LightName;
	}

	return LightName;
}

void UUnrealLevelImporterLightUpgrader::RemoveUnwantedComponents(FUnrealLevelImporterObject& Object)
{
	// Remove existing sprite, arrow and draw components
	TArray<FString> ClassesToRemove = {"DrawLightConeComponent", "DrawLightRadiusComponent", "SpriteComponent", "ArrowComponent"};

	// Number of sub objects before we removed ones we don't need
	int32 SubObjectCountBeforeStrip = Object.SubObjects.Num();
	for (int32 Idx = SubObjectCountBeforeStrip - 1; Idx >= 0; Idx--)
	{
		FUnrealLevelImporterObject& SubObj = Object.SubObjects[Idx];

		if (!ClassesToRemove.Contains(SubObj.ClassName))
		{
			continue;
		}

		Object.SubObjects.RemoveAt(Idx);
	}

	// Clear components array
	for (int32 Idx = 0; Idx < SubObjectCountBeforeStrip; Idx++)
	{
		Object.Properties.Remove(FString::Printf(TEXT("Components(%d)"), Idx));
	}

	// Add back our sub objects
	for (int32 Idx = 0; Idx < Object.SubObjects.Num(); Idx++)
	{
		FUnrealLevelImporterObject& Obj = Object.SubObjects[Idx];
		Object.Properties.Add(FString::Printf(TEXT("Components(%d)"), Idx), Obj.Name);
	}
}

void UUnrealLevelImporterLightUpgrader::UpgradeSharedProperties(FUnrealLevelImporterObject& Object, ELightType LightType, ELightModifierFlags Flags)
{
	const UUnrealLevelImporterSettings* Settings = UUnrealLevelImporterSettings::Get();

	// Update brightness and radius
	{
		float Brightness = DEFAULT_BRIGHTNESS;
		float Radius = DEFAULT_RADIUS;

		if (auto BrightnessPtr = Object.Properties.Find("Brightness"))
		{
			Brightness = FCString::Atof(**BrightnessPtr);
			Object.Properties.Remove("Brightness");
		}

		if (auto RadiusPtr = Object.Properties.Find("Radius"))
		{
			Radius = FCString::Atof(**RadiusPtr);
			Object.Properties.Remove("Radius");
		}

		{
			Brightness *= Settings->LightBrightnessMultiplier;
			Object.Properties.Add("IndirectLightingIntensity", FString::Printf(TEXT("%f"), Settings->IndirectLightingIntensity));
		}

		if (Radius != DEFAULT_RADIUS)
		{
			Radius *= Settings->LightRadiusMultiplier;
		}

		Object.Properties.Add("Intensity", FString::Printf(TEXT("%f"), Brightness));
		Object.Properties.Add("AttenuationRadius", FString::Printf(TEXT("%f"), Radius));
	}

	// Adjust falloff values
	if (LightType != ELightType::Directional)
	{
		Object.Properties.Add("bUseInverseSquaredFalloff", "false");

		if (auto FalloffPtr = Object.Properties.Find("FalloffExponent"))
		{
			Object.Properties.Add("LightFalloffExponent", *FalloffPtr);
		}
		else
		{
			Object.Properties.Add("LightFalloffExponent", "2");
		}
	}

	// Update light function
	{
		FUnrealLevelImporterObject* LightFunctionObject =
			Object.SubObjects.FindByPredicate([&](const FUnrealLevelImporterObject& Object) { return Object.Name.StartsWith(TEXT("LightFunction")); });

		if (LightFunctionObject)
		{
			if (auto MaterialPtr = LightFunctionObject->Properties.Find("SourceMaterial"))
			{
				Object.Properties.Add("LightFunctionMaterial", FUnrealLevelImporterUtils::GetRemappedPackagePath(*MaterialPtr));
			}

			if (auto ScalePtr = LightFunctionObject->Properties.Find("Scale"))
			{
				Object.Properties.Add("LightFunctionScale", *ScalePtr);
			}

			if (auto DisabledBrightnessPtr = LightFunctionObject->Properties.Find("DisabledBrightness"))
			{
				Object.Properties.Add("DisabledBrightness", *DisabledBrightnessPtr);
			}

			Object.SubObjects.Empty();
		}
	}

	// Update mobility
	{
		switch (Flags)
		{
			case Movable:
				Object.Properties.Add("Mobility", "Movable");
				break;
			case Toggleable:
			case Dominant:
				Object.Properties.Add("Mobility", "Stationary");
				break;

			default:
				Object.Properties.Add("Mobility", "Static");
				break;

		}

		if ((Flags & ELightModifierFlags::Dominant) == ELightModifierFlags::Dominant || (Flags & ELightModifierFlags::Toggleable) == ELightModifierFlags::Toggleable)
		{

		}

		if ((Flags & ELightModifierFlags::Movable) == ELightModifierFlags::Movable)
		{

		}
	}
}

void UUnrealLevelImporterLightUpgrader::UpgradePointLight(FUnrealLevelImporterObject& Object) {}

void UUnrealLevelImporterLightUpgrader::UpgradeSpotLight(FUnrealLevelImporterObject& Object) {}

void UUnrealLevelImporterLightUpgrader::UpgradeDirectionalLight(FUnrealLevelImporterObject& Object) {}

void UUnrealLevelImporterLightUpgrader::UpgradeSkyLight(FUnrealLevelImporterObject& Object) {}
