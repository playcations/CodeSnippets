// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ModumateGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class MODUMATE_API UModumateGameInstance : public UGameInstance
{
	GENERATED_UCLASS_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UEditManager> EditManagerClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UEditManager* EditManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USaveManager* SaveManager;
};
