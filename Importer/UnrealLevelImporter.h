#pragma once

#include "CoreMinimal.h"

/**
* @classs FUnrealLevelImporter
* Simple wrapper class to handle registering and de-registering the nomad tab spawner
* and related pieces of code.
*
* @todo This probably isn't the best but it's good enough for now
*/
class FUnrealLevelImporter
{
public:
	static void Register();
	static void Unregister();

	static void Open();
	static void Close();

	static TSharedRef<SDockTab> Spawn(const FSpawnTabArgs& Args);
private:
	static FName WindowId;
};
