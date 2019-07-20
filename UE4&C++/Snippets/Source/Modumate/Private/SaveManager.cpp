// Fill out your copyright notice in the Description page of Project Settings.

#include "SaveManager.h"

#include "ModumateGameInstance.h"
#include "EditManager.h"

#include "Internationalization/Internationalization.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Policies/PrettyJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"

#if WITH_EDITOR
#include "Dialogs/Dialogs.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#endif

#define LOCTEXT_NAMESPACE "Modumate"
typedef TJsonWriterFactory< TCHAR, TPrettyJsonPrintPolicy<TCHAR> > FPrettyJsonStringWriterFactory;
typedef TJsonWriter< TCHAR, TPrettyJsonPrintPolicy<TCHAR> > FPrettyJsonStringWriter;

USaveManager::USaveManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ProjectFileFilterJson(TEXT("Modumate Project JSON (*.json)|*.json"))
	, ProjectJsonIdentifier(TEXT("ModumateProjectJSON"))
{

}

bool USaveManager::StartSave()
{
#if WITH_EDITOR
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

	if (DesktopPlatform)
	{
		const FText Title = FText::Format(LOCTEXT("SaveLoad_SaveProjectDialogTitle", "Choose where to save '{0}'..."), FText::FromString(TEXT("MyModumateProject")));
		const FString CurrentFilename = ProjectPath;

		TArray<FString> OutFilenames;
		bool bSuccess = DesktopPlatform->SaveFileDialog(
			ParentWindowWindowHandle,
			Title.ToString(),
			(CurrentFilename.IsEmpty()) ? TEXT("") : FPaths::GetPath(CurrentFilename),
			(CurrentFilename.IsEmpty()) ? TEXT("") : FPaths::GetBaseFilename(CurrentFilename) + TEXT(".json"),
			ProjectFileFilterJson,
			EFileDialogFlags::None,
			OutFilenames
		);

		if (bSuccess && OutFilenames.Num() > 0)
		{
			ProjectPath = OutFilenames[0];
			return DoSaveJson(ProjectPath);
		}
	}

	return false;
#else
	return DoSaveJson(ProjectPath);
#endif
}

bool USaveManager::DoSaveJson(const FString& ProjectJsonPath)
{
	UModumateGameInstance* GDGameInstance = Cast<UModumateGameInstance>(GetOuter());
	if (GDGameInstance && !ProjectJsonPath.IsEmpty())
	{
		auto ProjectJson = GDGameInstance->EditManager->SerializeToJson();
		if (!ProjectJson.IsValid())
		{
			return false;
		}

		FString ProjectJsonString;
		TSharedRef<FPrettyJsonStringWriter> JsonStringWriter = FPrettyJsonStringWriterFactory::Create(&ProjectJsonString);
		bool bWriteJsonSuccess = FJsonSerializer::Serialize(ProjectJson.ToSharedRef(), JsonStringWriter);
		if (!bWriteJsonSuccess)
		{
			return false;
		}

		return FFileHelper::SaveStringToFile(ProjectJsonString, *ProjectJsonPath);
	}

	return false;
}

bool USaveManager::StartLoad()
{
#if WITH_EDITOR
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

	if (DesktopPlatform)
	{
		const FText Title = LOCTEXT("SaveLoad_LoadProjectDialogTitle", "Choose a project to load...");

		TArray<FString> OutFilenames;
		bool bSuccess = DesktopPlatform->OpenFileDialog(
			ParentWindowWindowHandle,
			Title.ToString(),
			ProjectPath,
			ProjectPath,
			ProjectFileFilterJson,
			EFileDialogFlags::None,
			OutFilenames
		);

		if (bSuccess && OutFilenames.Num() > 0)
		{
			ProjectPath = OutFilenames[0];
			return DoLoadJson(ProjectPath);
		}
	}

	return false;
#else
	return DoLoadJson(ProjectPath);
#endif
}

bool USaveManager::DoLoadJson(const FString& ProjectJsonPath)
{
	UModumateGameInstance* GDGameInstance = Cast<UModumateGameInstance>(GetOuter());
	if (GDGameInstance && !ProjectJsonPath.IsEmpty())
	{
		FString ProjectJsonString;
		bool bLoadFileSuccess = FFileHelper::LoadFileToString(ProjectJsonString, *ProjectJsonPath);
		if (!bLoadFileSuccess || ProjectJsonString.IsEmpty())
		{
			return false;
		}

		auto JsonReader = TJsonReaderFactory<>::Create(ProjectJsonString);

		TSharedPtr<FJsonObject> ProjectJson;
		bool bLoadJsonSuccess = FJsonSerializer::Deserialize(JsonReader, ProjectJson) && ProjectJson.IsValid();
		if (!bLoadJsonSuccess)
		{
			return false;
		}

		return GDGameInstance->EditManager->DeserializeFromJson(ProjectJson);
	}

	return false;
}
