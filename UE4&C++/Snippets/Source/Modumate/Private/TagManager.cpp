// Fill out your copyright notice in the Description page of Project Settings.

#include "TagManager.h"


ATagManager* ATagManager::SingletonInstance = NULL;

ATagManager::ATagManager()
{
	if (SingletonInstance == NULL)
	{
		SingletonInstance = this;
		PrimaryActorTick.bCanEverTick = true;
	}
	/*else
	{
		Destroy(this);
	}*/
}

void ATagManager::BeginPlay()
{
	Super::BeginPlay();
}

void ATagManager::CreateATag_Implementation(ETagType tagType, AActor * parent, bool bIsDoorLeft)
{
}

ATagManager * ATagManager::GetInstance()
{
	return SingletonInstance;
}

void ATagManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}