// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "Floor.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class MODUMATE_API AFloor : public AStaticMeshActor
{
	GENERATED_UCLASS_BODY()

public:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector DimensionTextOffset;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FColor DimensionLineColor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float DimensionLineWeight;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		FVector StartPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		FVector EndPoint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float floorDepth;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		bool bPlaced;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		FBox OriginalBounds;
	/*
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		class UTextRenderComponent* DimensionTextComponent;
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<class ADimensionStringBase> DimensionStringClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		AStaticMeshActor* PreviewFixture;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		class ADimensionStringBase* WidthDimensionString;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		class ADimensionStringBase* HeightDimensionString;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		TArray<AStaticMeshActor*> SortedAttachedFixtures;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		TArray<ADimensionStringBase*> FixtureDimensionStrings;
	/*
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		TArray<class UTextRenderComponent*> AttachedFixtureDimensionTexts;
	*/
	UFUNCTION(BlueprintCallable)
		void SetEndPoint(const FVector& NewEndPoint);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector FixtureTextOffset;

	UFUNCTION(BlueprintCallable)
		void AttachFixture(AStaticMeshActor* Fixture);


	// Begin serialization interface
	TSharedPtr<class FJsonObject> SerializeToJson() const;
	bool DeserializeFromJson(const TSharedPtr<class FJsonObject>& JsonValue);
	// End serialization interface


};
