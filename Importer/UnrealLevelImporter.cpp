#include "Importer/UnrealLevelImporter.h"

#include "ToolMenus.h"

#include "Importer/UnrealLevelImporterWidget.h"

#define LOCTEXT_NAMESPACE "UnrealLevelImporter"

FName FUnrealLevelImporter::WindowId = FName("UnrealLevelImporter");

void FUnrealLevelImporter::Register()
{
	FGlobalTabmanager::Get()
		->RegisterNomadTabSpawner(WindowId, FOnSpawnTab::CreateStatic(&FUnrealLevelImporter::Spawn))
		.SetDisplayName(LOCTEXT("UnrealLevelImporterTabTitle", "Level Importer"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FUnrealLevelImporter::Unregister()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(WindowId);
}

void FUnrealLevelImporter::Open()
{
	FGlobalTabmanager::Get()->TryInvokeTab(WindowId);
}

void FUnrealLevelImporter::Close()
{
	const TSharedPtr<SDockTab> DockTab = FGlobalTabmanager::Get()->FindExistingLiveTab(WindowId);
	if (DockTab.IsValid())
	{
		DockTab->RequestCloseTab();
	}
}

TSharedRef<SDockTab> FUnrealLevelImporter::Spawn(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab).TabRole(NomadTab)[SNew(SUnrealLevelImporterWidget)];
}

#undef LOCTEXT_NAMESPACE
