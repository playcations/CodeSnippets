// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "Window.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class MODUMATE_API AWindow : public AStaticMeshActor
{
	GENERATED_UCLASS_BODY()
	
public:

	virtual void BeginPlay() override;
};
