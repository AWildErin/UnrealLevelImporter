#pragma once

#include "Engine/DeveloperSettings.h"

#include "UnrealLevelImporterSettings.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct FUnrealLevelImporterRemap
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Remap")
	TSubclassOf<AActor> Class;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Remap")
	TMap<FString, FString> Properties;
};

/**
* @class UUnrealLevelImporterSettings
*
* @todo Add a second map that allows us to specify what upgrader to use on specific objects
*/
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Level Importer Settings"))
class UUnrealLevelImporterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UUnrealLevelImporterSettings();

	// Begin UDeveloperSettings interface
	virtual FName GetCategoryName() const override;
	// End UDeveloperSettings interface

	static const UUnrealLevelImporterSettings* Get() { return GetDefault<UUnrealLevelImporterSettings>(); };

    UPROPERTY(EditAnywhere, Config, Category = "Remapping")
	TMap<FString, FUnrealLevelImporterRemap> ClassRemapping;

	UPROPERTY(EditAnywhere, Config, Category = "Remapping")
	TMap<FString, FString> PackageRemap;

	// Importer settings
	UPROPERTY(EditAnywhere, Config, Category = "Importing|Lights")
	float LightBrightnessMultiplier;

	UPROPERTY(EditAnywhere, Config, Category = "Importing|Lights")
	float LightRadiusMultiplier;

	UPROPERTY(EditAnywhere, Config, Category = "Importing|Lights")
	float IndirectLightingIntensity;

};
