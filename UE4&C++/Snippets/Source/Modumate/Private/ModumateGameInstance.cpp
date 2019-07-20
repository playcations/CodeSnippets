// Fill out your copyright notice in the Description page of Project Settings.

#include "ModumateGameInstance.h"

#include "EditManager.h"
#include "SaveManager.h"

UModumateGameInstance::UModumateGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, EditManagerClass(UEditManager::StaticClass())
	, EditManager(nullptr)
{

}

void UModumateGameInstance::Init()
{
	EditManager = NewObject<UEditManager>(this, EditManagerClass, FName(TEXT("EditManager")));
	SaveManager = NewObject<USaveManager>(this, USaveManager::StaticClass(), FName(TEXT("SaveManager")));
}

void UModumateGameInstance::Shutdown()
{
	EditManager = nullptr;
}
