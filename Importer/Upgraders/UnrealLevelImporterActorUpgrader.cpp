#include "Importer/Upgraders/UnrealLevelImporterActorUpgrader.h"

#include "Importer/UnrealLevelImporterWidget.h"
#include "Importer/UnrealLevelImporterUtils.h"

bool UUnrealLevelImporterActorUpgrader::Upgrade(FUnrealLevelImporterObject& ImporterObject)
{
	ImporterObject.Properties.Add("ActorLabel", "\"" + ImporterObject.Name + "\"");

	return true;
}

bool UUnrealLevelImporterActorUpgrader::ConvertTransform(FUnrealLevelImporterObject& Object, FVector& OutLoc, FRotator& OutRot, FVector& OutScale)
{
	// Copy Location and Rotation
	OutLoc = FVector::ZeroVector;
	OutRot = FRotator::ZeroRotator;
	OutScale = FVector::OneVector;

	if (auto LocationPtr = Object.Properties.Find("Location"))
	{
		OutLoc = FUnrealLevelImporterUtils::ParseVector(**LocationPtr);
		Object.Properties.Remove("Location");
	}

	if (auto RotationPtr = Object.Properties.Find("Rotation"))
	{
		OutRot = FUnrealLevelImporterUtils::ParseRotator(**RotationPtr, true);
		Object.Properties.Remove("Rotation");
	}

	if (auto DrawScale3DPtr = Object.Properties.Find("DrawScale3D"))
	{
		OutScale = FUnrealLevelImporterUtils::ParseVector(**DrawScale3DPtr);
		Object.Properties.Remove("DrawScale3D");
	}

	float DrawScale = 1.f;
	if (auto DrawScalePtr = Object.Properties.Find("DrawScale"))
	{
		DrawScale = FCString::Atof(**DrawScalePtr);
		Object.Properties.Remove("DrawScale");

		OutScale *= DrawScale;
	}

	return true;
}

bool UUnrealLevelImporterActorUpgrader::ConvertAndAssignTransform(FUnrealLevelImporterObject& FromObject, FUnrealLevelImporterObject& ToObject)
{
	FVector Location;
	FRotator Rotation;
	FVector Scale;
	if (!ConvertTransform(FromObject, Location, Rotation, Scale))
	{
		return false;
	}

	// Some objects might have a translation/rotation vector on the root component
	// so we get that and combine the two values
	if (auto TranslationPtr = ToObject.Properties.Find("Translation"))
	{
		FVector SubTranslation = FUnrealLevelImporterUtils::ParseVector(**TranslationPtr);
		ToObject.Properties.Remove("Translation");

		Location += SubTranslation;
	}

	if (auto RotationPtr = ToObject.Properties.Find("Rotation"))
	{
		FRotator SubRotation = FUnrealLevelImporterUtils::ParseRotator(**RotationPtr, true);
		ToObject.Properties.Remove("Rotation");

		Rotation += SubRotation;
	}

	ToObject.Properties.Add("RelativeLocation", FUnrealLevelImporterUtils::WriteVector(Location));
	ToObject.Properties.Add("RelativeRotation", FUnrealLevelImporterUtils::WriteRotator(Rotation));
	ToObject.Properties.Add("RelativeScale3D", FUnrealLevelImporterUtils::WriteVector(Scale));

	return true;
}

FVector UUnrealLevelImporterActorUpgrader::GetCorrectedScale(FUnrealLevelImporterObject& Object)
{
	FVector DrawScale3D = FVector::OneVector;
	float DrawScale = 1.f;

	if (auto DrawScale3DPtr = Object.Properties.Find("DrawScale3D"))
	{
		DrawScale3D = FUnrealLevelImporterUtils::ParseVector(**DrawScale3DPtr);
	}

	if (auto DrawScalePtr = Object.Properties.Find("DrawScale"))
	{
		DrawScale = FCString::Atof(**DrawScalePtr);
	}

	return DrawScale3D * DrawScale;
}
