// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "Wall.generated.h"

USTRUCT(Blueprintable)
struct MODUMATE_API FWallIntersection
{
	GENERATED_USTRUCT_BODY()

public:
	FWallIntersection();

	void Init();

	UPROPERTY()
		class AWall* HitWall;

	UPROPERTY()
		float DistAlongQueryWall;

	UPROPERTY()
		float DistAlongHitWall;

	UPROPERTY()
		FVector Location;
};

USTRUCT()
struct FIntersectionPoints
{
	GENERATED_BODY()

public:

	UPROPERTY()
		FVector b_LeftSideStart;
	UPROPERTY()
		FVector b_RightSideStart;
	UPROPERTY()
		FVector b_LeftSideEnd;
	UPROPERTY()
		FVector b_RightSideEnd;

	UPROPERTY()
		FVector t_LeftSideStart;
	UPROPERTY()
		FVector t_RightSideStart;
	UPROPERTY()
		FVector t_LeftSideEnd;
	UPROPERTY()
		FVector t_RightSideEnd;
};

USTRUCT()
struct FWallBox
{
	GENERATED_BODY()

public:

	UPROPERTY()
		FVector b_LeftSideStart;
	UPROPERTY()
		FVector b_RightSideStart;
	UPROPERTY()
		FVector b_LeftSideEnd;
	UPROPERTY()
		FVector b_RightSideEnd;
	UPROPERTY()
		FVector t_LeftSideStart;
	UPROPERTY()
		FVector t_RightSideStart;
	UPROPERTY()
		FVector t_LeftSideEnd;
	UPROPERTY()
		FVector t_RightSideEnd;

	UPROPERTY()
		TArray<FVector> wallVertices;
	UPROPERTY()
		TArray<int> Triangles;

		void AddVerts();
	
};
/**
 *
 */
UCLASS(Blueprintable)
class MODUMATE_API AWall : public AStaticMeshActor
{
	GENERATED_UCLASS_BODY()

public:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 ID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector DimensionTextOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector FixtureTextOffset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		class ARoomNode* StartNode;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
		FVector StartPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		class ARoomNode* EndNode;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
		FVector EndPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
		bool bPlaced;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		bool bGrounded;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		bool bVisited;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		AWall * LeftChainedWall;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		AWall * RightChainedWall;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		ARoomNode * LeftChainedNode;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		ARoomNode * RightChainedNode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		FBox OriginalBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FQuat wallRot;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector wallMidPoint;
	
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector wallScale;

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
	/* Connectivity information, read from graph */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		class AWall* SourceLeftWall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		class AWall* SourceRightWall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		class AWall* DestLeftWall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		class AWall* DestRightWall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		class ARoom* LeftRoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		class ARoom* RightRoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		TArray<FVector> wallVertices;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		float wallThickness;

	UPROPERTY()
		TArray<FIntersectionPoints> intersections;

	UPROPERTY()
		TArray<FWallBox> WallBoxes;
	/* UFUNCTIONs */

	UFUNCTION(BlueprintPure)
		FVector GetWallStart() const;

	UFUNCTION(BlueprintPure)
		FVector GetWallEnd() const;

	UFUNCTION(BlueprintPure)
		FVector GetWallDir() const;

	UFUNCTION(BlueprintPure)
		FVector GetWallDirFrom(class ARoomNode* Node) const;

	UFUNCTION(BlueprintCallable)
		void SetEndPoint(const FVector& NewEndPoint);

	UFUNCTION(BlueprintCallable)
		void AttachFixture(AStaticMeshActor* Fixture);

	UFUNCTION(BlueprintCallable)
		void UpdatePreviewFixture(AStaticMeshActor* Fixture);

	UFUNCTION(BlueprintCallable)
		void RemovePrevDimStrings();

	UFUNCTION(BlueprintCallable)
		bool SortConnectedWalls();

	UFUNCTION(BlueprintCallable)
		void DebugDrawConnectedWalls();

	UFUNCTION(BlueprintPure)
		bool GetWallIntersection2D(class AWall* OtherWall, FWallIntersection& WallIntersection, float Epsilon = 0.01f) const;
	
	// Begin serialization interface
	TSharedPtr<class FJsonObject> SerializeToJson() const;
	bool DeserializeFromJson(const TSharedPtr<class FJsonObject>& JsonValue);
	// End serialization interface

protected:
	bool FindLeftAndRightWalls(bool bForward, class AWall*& LeftWall, class AWall*& RightWall);
};
