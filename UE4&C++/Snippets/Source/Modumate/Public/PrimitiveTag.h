// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PrimitiveTag.generated.h"


UCLASS(Blueprintable)
class MODUMATE_API APrimitiveTag : public AActor
{
	GENERATED_BODY()
		

public:	
	// Sets default values for this component's properties
	APrimitiveTag();

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	/*UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
		UTextRenderComponent* tagInfo;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
		UStaticMeshComponent* tag;*/

	/*UFUNCTION(BlueprintCallable)
		virtual	void AttachAndPlaceTag(FText textInfo, UMaterialInterface* tagMesh);
	UFUNCTION(BlueprintCallable)
		virtual void OrientNorth();
	UFUNCTION(BlueprintCallable)
		virtual void SetPosition();*/

};
