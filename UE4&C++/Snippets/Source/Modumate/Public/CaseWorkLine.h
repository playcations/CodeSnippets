// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "CaseWorkLine.generated.h"

/**
 * 
 */
UCLASS()
class MODUMATE_API ACaseWorkLine : public AStaticMeshActor
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
		float CaseWorkLineDepth;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		bool bPlaced;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		FBox OriginalBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		class UTextRenderComponent* DimensionTextComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		TArray<AStaticMeshActor*> SortedAttachedFixtures;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		TArray<class UTextRenderComponent*> AttachedFixtureDimensionTexts;

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
