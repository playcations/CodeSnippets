// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EditManager.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class MODUMATE_API UEditManager : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	virtual class UWorld* GetWorld() const override;
	//walls
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class AWall> WallClass;

	UFUNCTION(BlueprintCallable)
	class AWall* StartWall(const FVector& WallOrigin);

	UFUNCTION(BlueprintCallable)
	class AWall* FinishWall(const FVector& WallEnd);

	UFUNCTION(BlueprintCallable)
	void RemoveWall(class AWall* Wall);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<class AWall*> Walls;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class AWall* PendingWall;

	//Floors
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class AFloor> FloorClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<class AFloor*> FloorLines;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class AFloor* PendingFloorLine;
	
	UFUNCTION(BlueprintCallable)
	class AFloor* StartFloor(const FVector& FloorOrigin);

	UFUNCTION(BlueprintCallable)
	class AFloor* FinishFloorLine(const FVector& FloorEnd);

	UFUNCTION(BlueprintCallable)
	void RemoveFloor(class AFloor* Floor);
	
	UFUNCTION(BlueprintCallable)
	void GenterateFloorBase(UProceduralMeshComponent* FloorBase, float depth);
	
	// CaseWork
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class ACaseWorkLine> CaseWorkLineClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<class ACaseWorkLine*> CaseWorkLines;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<class AActor*> CaseworkCompletes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class ACaseWorkLine* PendingCaseWorkLine;

	UFUNCTION(BlueprintCallable)
	class ACaseWorkLine* StartCaseWorkLine(const FVector& CaseWorkLineOrigin);

	UFUNCTION(BlueprintCallable)
	class ACaseWorkLine* FinishCaseWorkLine(const FVector& CaseWorkLineEnd);

	UFUNCTION(BlueprintCallable)
	void RemoveCaseWorkLine(class ACaseWorkLine* CaseWorkLine);

	UFUNCTION(BlueprintCallable)
	void GenterateCaseWork(UProceduralMeshComponent* CaseWorkBaseBase, float height, AActor* CaseworkGeneratedActor);

	//windows
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class AWindow> WindowClass;

	UFUNCTION(BlueprintCallable)
	class AWindow* PlaceWindow(class AWall* Wall, const FVector& WindowPos);
	
	//dimension strings
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<class ADimensionStringBase> DimensionStringClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<class ADimensionStringBase*> InteriorDimensionStrings;

	UPROPERTY(BlueprintReadOnly)
		bool FoundGrounded;

	//rooms
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class ARoom> RoomClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class ARoomNode> RoomNodeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RoomNodeEpsilon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<class ARoom*> Rooms;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<class ARoomNode*> RoomNodes;


	
	//other
	UFUNCTION(BlueprintCallable)
	TArray<int32> Triangulate(TArray<FVector> vertices);

	UFUNCTION(BlueprintCallable)
	void GenterateWall(UProceduralMeshComponent* WallMesh, float height, float thickness, AWall* PendingWallActor);

	UFUNCTION(BlueprintCallable)
	void CutWindowIntoWall(UProceduralMeshComponent* WallMesh, AWall* CurrentWallActor, FVector Origin, FVector BoxExtend, bool bIsDoor, bool isPreview);
	
	UFUNCTION(BlueprintCallable)
	void TriangulateWallBoxes(int i_currentWallVertSize, int i_currentLocalVertSize, TArray<int> &Triangle);

	UFUNCTION(BlueprintCallable)
		bool IsWithinBoxBounds(FVector origin, FVector bounds, AWall *CurrentWall);


	// Begin serialization interface
	TSharedPtr<class FJsonObject> SerializeToJson() const;
	bool DeserializeFromJson(const TSharedPtr<class FJsonObject>& JsonValue);
	// End serialization interface
	UFUNCTION(BlueprintCallable)
	void MakeHeightDMInvisible();
	UFUNCTION(BlueprintCallable)
	void MakeHeightDMVisible();

protected:
	class AWall* SpawnWall(const FVector& Origin = FVector::ZeroVector);
	class AFloor* SpawnFloor(const FVector& Origin = FVector::ZeroVector);
	class ACaseWorkLine* SpawnCaseWorkLine(const FVector& Origin = FVector::ZeroVector);

	bool GetIntersectingWalls(class AWall* QueryWall, TArray<struct FWallIntersection>& OutIntersections);
	void OnWallMoved(class AWall* ChangedWall);
	bool ResetWallConnectivity(class AWall* ChangedWall);
	void UpdateRoomsFromWalls();
	void UpdateDimensionStringsForInteriorWalls();
	void UpdateGrounded(AWall * Wall);
	void UpdateGrounded(ARoomNode * Node);
	
	class ARoom* CreateRoomFromData(const struct FRoomData& RoomData);
	class ARoom* FindRoomFromData(const struct FRoomData& RoomData, bool bCreateIfNotFound = false);
	class ARoom* FindMostSimilarRoom(const struct FRoomData& RoomData, int32& NumSharedWalls);
	class ARoomNode* CreateNodeAtPoint(const FVector& Position);
	class ARoomNode* FindNodeAtPoint(const FVector& Position, class ARoomNode* IgnoreNode = nullptr);
	class ARoomNode* FindOrCreateNodeAtPoint(const FVector& Position);
};
