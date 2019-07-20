// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SaveManager.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class MODUMATE_API USaveManager : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString ProjectPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ProjectFileFilterJson;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ProjectJsonIdentifier;

	UFUNCTION(BlueprintCallable)
	bool StartSave();

	UFUNCTION(BlueprintCallable)
	bool DoSaveJson(const FString& ProjectJsonPath);

	UFUNCTION(BlueprintCallable)
	bool StartLoad();

	UFUNCTION(BlueprintCallable)
	bool DoLoadJson(const FString& ProjectJsonPath);
};
