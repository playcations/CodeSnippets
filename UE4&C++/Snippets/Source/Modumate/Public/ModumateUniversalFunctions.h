// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Object.h"
#include "Runtime/Engine/Classes/Components/TextRenderComponent.h"
#include "ModumateUniversalFunctions.generated.h"
/**
 * 
 */
UCLASS()
class MODUMATE_API UModumateUniversalFunctions : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	static void CentimetersToImperialInches(float Centimeters, UPARAM(ref) TArray<int>& Imperial);
	UFUNCTION(BlueprintCallable)
	static FText ImperialInchesToDimensionStringText(UPARAM(ref) TArray<int>& Imperial);
	UFUNCTION(BlueprintCallable)
	static void SetChildComponentOrientation(UPARAM(ref) USceneComponent* ChildComponent, FVector Up, FVector Right , FVector Forward);
};
