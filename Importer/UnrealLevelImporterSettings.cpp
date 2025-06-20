#include "Importer/UnrealLevelImporterSettings.h"

#include "Engine/BlockingVolume.h"
#include "Misc/App.h"

UUnrealLevelImporterSettings::UUnrealLevelImporterSettings()
{
	// Default actor remapping
	ClassRemapping.Add("PathBlockingVolume", {ABlockingVolume::StaticClass(), {}});

	LightBrightnessMultiplier = 10.f;
	LightRadiusMultiplier = 1.25f;
	IndirectLightingIntensity = 1.f;
}

FName UUnrealLevelImporterSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}
