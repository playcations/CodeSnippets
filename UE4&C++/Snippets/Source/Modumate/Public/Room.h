// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "Room.generated.h"


namespace FGraphUtils
{
	/** Determines whether two edges may be merged. */
	MODUMATE_API bool AreEdgesMergeable(const FVector& V0, const FVector& V1, const FVector& V2);

	/** Given three direction vectors, indicates if A and B are on the same 'side' of Vec. */
	MODUMATE_API bool VectorsOnSameSide(const FVector& Vec, const FVector& A, const FVector& B);

	/** Util to see if P lies within triangle created by A, B and C. */
	MODUMATE_API bool PointInTriangle(const FVector& A, const FVector& B, const FVector& C, const FVector& P);
};

USTRUCT()
struct MODUMATE_API FRoomData
{
	GENERATED_USTRUCT_BODY()

public:

	FRoomData();

	UPROPERTY()
	int32 ID;

	UPROPERTY()
	FVector Normal;

	UPROPERTY()
	float Winding;

	UPROPERTY()
	bool bClosed;

	UPROPERTY()
	TArray<class ARoomNode*> Nodes;

	UPROPERTY()
	TArray<int32> LoopWallIndices;

	UPROPERTY()
	TArray<class AWall*> WallsOrdered;

	UPROPERTY()
	TArray<bool> WallDirections;

	UPROPERTY()
	TMap<class AWall*, int32> ForwardWallIndices;

	UPROPERTY()
	TMap<class AWall*, int32> BackwardWallIndices;

	UPROPERTY()
	TArray<int32> TriangleIndices;

	UPROPERTY()
	int32 MinWallIDIndex;

	bool AddWall(class AWall* NewWall, bool bWallForward);
	bool Close();
	bool ContainsWall(class AWall* Wall, bool bWallForward, int32& WallIndex) const;
	bool IsClockWise() const { return Winding > 0.0f; }
	int32 CompareWalls(const FRoomData& Other) const;
	bool Equals(const FRoomData& Other, bool bCompareIDs = false) const;
	float GetWinding(const FVector& FromDir, const FVector& ToDir) const;

protected:
	bool Triangulate();
};

/**
 * 
 */
UCLASS(Blueprintable)
class MODUMATE_API ARoom : public AStaticMeshActor
{
	GENERATED_UCLASS_BODY()
	
public:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY()
	FRoomData RoomData;

	UFUNCTION(BlueprintPure)
	bool IsInterior() const;

	UFUNCTION(BlueprintCallable)
	void DebugDraw(float Duration = 2.0f) const;

	void UpdateRoomData(const FRoomData& NewRoomData);

	// Begin serialization interface
	TSharedPtr<class FJsonObject> SerializeToJson() const;
	bool DeserializeFromJson(const TSharedPtr<class FJsonObject>& JsonValue);
	// End serialization interface
};
