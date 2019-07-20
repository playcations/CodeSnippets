// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DimensionStringBase.generated.h"

UCLASS(Blueprintable)
class MODUMATE_API ADimensionStringBase : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ADimensionStringBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
public:
	// Called every frame
	
	//pure virtual function for derived classes to implement.
	UFUNCTION(BlueprintNativeEvent)
	void SetDimensionString(FVector WPointB, FVector WPointA, FVector TPointB, FVector TPointA,FVector TextUp, FVector TextRight, FVector TextForward);
	UFUNCTION(BlueprintNativeEvent)
	void SetInvisible();
	UFUNCTION(BlueprintNativeEvent)
	void SetVisible();
};
