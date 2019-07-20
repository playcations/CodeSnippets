// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "RoomNode.generated.h"

class AWall;

/**
 * 
 */
UCLASS(Blueprintable)
class MODUMATE_API ARoomNode : public AActor
{
	GENERATED_UCLASS_BODY()
	
public:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/* Connectivity information, inferred from graph */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<AWall*> SortedWalls;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<AWall*, int32> WallIndexMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bConnectionsDirty;

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
		AWall * AssociatedWall;

	/* UFUNCTIONs */

	UFUNCTION(BlueprintCallable)
	bool ConnectWall(AWall* Wall, bool bAutoUpdate = false);

	UFUNCTION(BlueprintCallable)
	void SortConnectedWalls();

	UFUNCTION(BlueprintPure)
	bool IsConnectedToWall(AWall* Wall) const { return WallIndexMap.Contains(Wall); }

	bool FindAdjacentWalls(const AWall* SourceWall, AWall*& LeftWall, AWall*& RightWall) const;
	bool MergeNode(class ARoomNode* NodeToMerge);

	// Begin serialization interface
	TSharedPtr<class FJsonObject> SerializeToJson() const;
	bool DeserializeFromJson(const TSharedPtr<class FJsonObject>& JsonValue);
	// End serialization interface
};
