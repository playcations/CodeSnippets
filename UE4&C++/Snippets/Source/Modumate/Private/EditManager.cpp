// Fill out your copyright notice in the Description page of Project Settings.

#include "EditManager.h"

#include "Serialization/JsonTypes.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "ModumateGameInstance.h"
#include "DrawDebugHelpers.h"
#include "Wall.h"
#include "Window.h"
#include "Floor.h"
#include "Room.h"
#include "CaseWorkLine.h"
#include "RoomNode.h"
#include "DimensionStringBase.h"
#include "ProceduralMeshComponent.h"

UEditManager::UEditManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, WallClass(AWall::StaticClass())
	, WindowClass(AWindow::StaticClass())
	, FloorClass(AWindow::StaticClass())
	, CaseWorkLineClass(ACaseWorkLine::StaticClass())
	, DimensionStringClass(ADimensionStringBase::StaticClass())
	, FoundGrounded(false)
	, RoomClass(ARoom::StaticClass())
	, RoomNodeClass(ARoomNode::StaticClass())
	, RoomNodeEpsilon(0.1f)
	, PendingWall(nullptr)
	, PendingFloorLine(nullptr)
	, PendingCaseWorkLine(nullptr)
{

}

UWorld* UEditManager::GetWorld() const
{
	if (UModumateGameInstance* GDGameInstance = CastChecked<UModumateGameInstance>(GetOuter()))
	{
		return GDGameInstance->GetWorld();
	}

	return nullptr;
}


TSharedPtr<FJsonObject> UEditManager::SerializeToJson() const
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());

	TArray<TSharedPtr<FJsonValue>> WallsJson;
	for (const AWall* Wall : Walls)
	{
		auto WallJson = Wall->SerializeToJson();
		WallsJson.Add(MakeShareable(new FJsonValueObject(WallJson)));
	}
	ResultJson->SetArrayField(GET_MEMBER_NAME_STRING_CHECKED(UEditManager, Walls), WallsJson);

	return ResultJson;
}

bool UEditManager::DeserializeFromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	auto WallsJson = JsonObject->GetArrayField(GET_MEMBER_NAME_STRING_CHECKED(UEditManager, Walls));

	for (int32 iWall = 0; iWall < WallsJson.Num(); ++iWall)
	{
		AWall* NewWall = SpawnWall();
		NewWall->DeserializeFromJson(WallsJson[iWall]->AsObject());
		Walls.Add(NewWall);
		OnWallMoved(NewWall);
	}

	return true;
}

/*******
Windows
*******/

AWindow* UEditManager::PlaceWindow(AWall* Wall, const FVector& WindowPos)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FVector WallRight = Wall->GetActorRightVector();
	FVector ProjectedWindowPos = FVector::PointPlaneProject(WindowPos, Wall->StartPoint, WallRight);
	FQuat ProjectedWindowRot(FRotationMatrix::MakeFromX(WallRight));
	AWindow* NewWindow = GetWorld()->SpawnActor<AWindow>(WindowClass, FTransform(ProjectedWindowRot, ProjectedWindowPos), SpawnParams);
	Wall->AttachFixture(NewWindow);

	return NewWindow;
}

/*******
Floors
*******/

AFloor* UEditManager::StartFloor(const FVector& floorOrigin)
{
	if (!ensureAlways(!PendingFloorLine))
	{
		return nullptr;
	}

	PendingFloorLine = SpawnFloor(floorOrigin);
	return PendingFloorLine;
}

AFloor* UEditManager::FinishFloorLine(const FVector& floorEnd)
{
	if (!ensureAlways(PendingFloorLine))
	{
		return nullptr;
	}

	PendingFloorLine->SetEndPoint(floorEnd);
	PendingFloorLine->bPlaced = true;

	FloorLines.Add(PendingFloorLine);

	AFloor* NewFloor = PendingFloorLine;
	PendingFloorLine = nullptr;
	return NewFloor;
}

AFloor* UEditManager::SpawnFloor(const FVector& Origin)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	return GetWorld()->SpawnActor<AFloor>(FloorClass, FTransform(FQuat::Identity, Origin), SpawnParams);
}

void UEditManager::RemoveFloor(class AFloor* Floor)
{
	if (ensureAlways(Floor))
	{
		if (PendingFloorLine == Floor)
		{
			PendingFloorLine = nullptr;
		}

		FloorLines.Remove(Floor);
		Floor->Destroy();
	}
}

void UEditManager::GenterateFloorBase(UProceduralMeshComponent* FloorBase, float depth)
{
	//FloorBase = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));


	FloorBase->bUseAsyncCooking = true;

	TArray<FVector> vertices;
	//vertices.Add(FloorLines[0]->StartPoint);
	for (int i = 0; i != FloorLines.Num(); i++)
	{
		vertices.Add(FloorLines[i]->StartPoint);
	}

	TArray<int32> triangles = Triangulate(vertices);

	//add thickness here
	//setting up double sided floor
	TArray<FVector> bottomVertices;
	for (int i = 0; i != FloorLines.Num(); i++)
	{
		FVector _i = FloorLines[i]->StartPoint;

		_i.Z = -depth;
		bottomVertices.Add(_i);
	}


	TArray<int32> bottomTriangle;
	for (int i = 0; i < triangles.Num(); i++)
	{
		bottomTriangle.Add(triangles[(triangles.Num() - 1 - i)] + vertices.Num());
	}

	TArray<int32> sides;
	for (int i = 0; i < vertices.Num(); i++)
	{
		if (i < vertices.Num() - 1)
		{
			sides.Add(i + (vertices.Num()));
			sides.Add(i + 1);
			sides.Add(i);

			sides.Add(i + (vertices.Num()));
			sides.Add(i + (vertices.Num() + 1));
			sides.Add(i + 1);
		}
		else
		{
			sides.Add(i + (vertices.Num()));
			sides.Add(i + 1);
			sides.Add(i);

			sides.Add(vertices.Num());
			sides.Add(0);
			sides.Add(i);
		}
	}

	//add verst to vertecies
	for (int i = 0; i != bottomVertices.Num(); i++)
	{
		vertices.Add(bottomVertices[i]);
	}

	//add rest of the indeces
	for (int i = 0; i < sides.Num(); i++)
	{
		triangles.Add(sides[i]);
	}

	for (int i = 0; i < bottomTriangle.Num(); i++)
	{
		triangles.Add(bottomTriangle[i]);
	}

	//setup the rest of the variables
	//TODO PEYVAN Figure out algo to fix \/

	TArray<FVector> normals;
	//for (int i = 0; i != bottomTriangle.Num(); i++)
	//{
	//	normals.Add(FVector(0, 0, 1));
	//}
	//for (int i = 0; i != bottomTriangle.Num(); i++)
	//{
	//	normals.Add(FVector(0, 0, -1));
	//}

	//TODO PEYVAN Figure out UV algo to fix
	TArray<FVector2D> UV0;
	/*for (int i = 0; i != triangles.Num() / 3; i++)
	{
		UV0.Add(FVector2D(0.0f, 0.0f));
		UV0.Add(FVector2D(1.0f, 0.0f));
		UV0.Add(FVector2D(0.0f, 1.0f));
	}*/

	TArray<FProcMeshTangent> tangents;
	/*for (int i = 0; i != triangles.Num(); i++)
	{
		tangents.Add(FProcMeshTangent(0, 1, 0));
	}*/


	TArray<FLinearColor> vertexColors;
	for (int i = 0; i != triangles.Num(); i++)
	{
		vertexColors.Add(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}

	//TODO PEYVAN Figure out algo to fix /\

	normals.Empty();
	UV0.Empty();
	tangents.Empty();
	//vertexColors.Empty();


	FloorBase->CreateMeshSection_LinearColor(0, vertices, triangles, normals, UV0, vertexColors, tangents, true);



}



/*******
Casework
*******/

ACaseWorkLine* UEditManager::StartCaseWorkLine(const FVector& CaseWorkLineOrigin)
{
	if (!ensureAlways(!PendingCaseWorkLine))
	{
		return nullptr;
	}

	PendingCaseWorkLine = SpawnCaseWorkLine(CaseWorkLineOrigin);
	return PendingCaseWorkLine;
}

ACaseWorkLine* UEditManager::FinishCaseWorkLine(const FVector& CaseWorkLineEnd)
{
	if (!ensureAlways(PendingCaseWorkLine))
	{
		return nullptr;
	}

	PendingCaseWorkLine->SetEndPoint(CaseWorkLineEnd);
	PendingCaseWorkLine->bPlaced = true;

	CaseWorkLines.Add(PendingCaseWorkLine);

	ACaseWorkLine* NewCaseWorkLine = PendingCaseWorkLine;
	PendingCaseWorkLine = nullptr;
	return NewCaseWorkLine;
}

ACaseWorkLine* UEditManager::SpawnCaseWorkLine(const FVector& Origin)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	return GetWorld()->SpawnActor<ACaseWorkLine>(CaseWorkLineClass, FTransform(FQuat::Identity, Origin), SpawnParams);
}

void UEditManager::RemoveCaseWorkLine(class ACaseWorkLine* CaseWorkLine)
{
	if (ensureAlways(CaseWorkLine))
	{
		if (PendingCaseWorkLine == CaseWorkLine)
		{
			PendingCaseWorkLine = nullptr;
		}

		CaseWorkLines.Remove(CaseWorkLine);
		CaseWorkLine->Destroy();
	}
}

void UEditManager::GenterateCaseWork(UProceduralMeshComponent* CaseWorkBaseBase, float height, AActor* CaseworkGeneratedActor)
{
	
	CaseWorkBaseBase->bUseAsyncCooking = true;

	TArray<FVector> vertices;
	for (int i = 0; i != CaseWorkLines.Num(); i++)
	{
		vertices.Add(CaseWorkLines[i]->StartPoint);
	}

	TArray<int32> triangles = Triangulate(vertices);

	//add thickness here
	//setting up double sided CaseWork
	TArray<FVector> topVertices;
	for (int i = 0; i != CaseWorkLines.Num(); i++)
	{
		FVector _i = CaseWorkLines[i]->StartPoint;

		_i.Z = height;
		topVertices.Add(_i);
	}

	TArray<int32> bottomTriangle;
	for (int i = 0; i < triangles.Num(); i++)
	{
		bottomTriangle.Add(triangles[(triangles.Num() - 1 - i)] + vertices.Num());
	}

	TArray<int32> sides;
	for (int i = 0; i < vertices.Num(); i++)
	{
		if (i < vertices.Num() - 1)
		{
			sides.Add(i + (vertices.Num()));
			sides.Add(i + 1);
			sides.Add(i);

			sides.Add(i + (vertices.Num()));
			sides.Add(i + (vertices.Num() + 1));
			sides.Add(i + 1);
		}
		else
		{
			sides.Add(i + (vertices.Num()));
			sides.Add(i + 1);
			sides.Add(i);

			sides.Add(vertices.Num());
			sides.Add(0);
			sides.Add(i);
		}
	}

	////add verst to vertecies
	TArray<FVector> AllVerts;
	for (int i = 0; i != topVertices.Num(); i++)
	{
		AllVerts.Add(topVertices[i]);
	}
	for (int i = 0; i != topVertices.Num(); i++)
	{
		AllVerts.Add(vertices[i]);
	}



	//add rest of the indeces
	for (int i = 0; i < sides.Num(); i++)
	{
		triangles.Add(sides[i]);
	}

	for (int i = 0; i < bottomTriangle.Num(); i++)
	{
		triangles.Add(bottomTriangle[i]);
	}

	//setup the rest of the variables
	//TODO PEYVAN Figure out algo to fix \/

	TArray<FVector> normals;
	TArray<FVector2D> UV0;
	TArray<FProcMeshTangent> tangents;
	TArray<FLinearColor> vertexColors;
	for (int i = 0; i != triangles.Num(); i++)
	{
		vertexColors.Add(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}

	//TODO PEYVAN Figure out algo to fix /\

	normals.Empty();
	UV0.Empty();
	tangents.Empty();
	//vertexColors.Empty();


	CaseWorkBaseBase->CreateMeshSection_LinearColor(0, AllVerts, triangles, normals, UV0, vertexColors, tangents, true);
	CaseWorkLines.Empty();
	CaseworkCompletes.Add(CaseworkGeneratedActor);
}


/*******
walls
*******/

AWall* UEditManager::StartWall(const FVector& WallOrigin)
{
	if (!ensureAlways(!PendingWall))
	{
		return nullptr;
	}

	PendingWall = SpawnWall(WallOrigin);
	return PendingWall;
}

AWall* UEditManager::FinishWall(const FVector& WallEnd)
{
	if (!ensureAlways(PendingWall))
	{
		return nullptr;
	}

	TArray<FWallIntersection> Intersections;
	if (GetIntersectingWalls(PendingWall, Intersections))
	{
		for (FWallIntersection& Intersection : Intersections)
		{
			UE_LOG(LogTemp, Log, TEXT("Intersection between %s and %s at %s"), *PendingWall->GetName(), *Intersection.HitWall->GetName(), *Intersection.Location.ToString());
			DrawDebugSphere(GetWorld(), Intersection.Location, 20.0f, 32, FColor::Red, true, 2.0f, 0, 1.0f);
		}

		// For now, fail wall placement if it causes an intersection, instead of splitting the wall.
		return nullptr;
	}

	PendingWall->ID = Walls.Num();
	PendingWall->SetEndPoint(WallEnd);
	PendingWall->bPlaced = true;

	Walls.Add(PendingWall);

	AWall* NewWall = PendingWall;
	PendingWall = nullptr;

	OnWallMoved(NewWall);

	
	UpdateRoomsFromWalls();
	
	
	UpdateDimensionStringsForInteriorWalls();

	return NewWall;
}

void UEditManager::RemoveWall(class AWall* Wall)
{
	if (ensureAlways(Wall))
	{
		if (PendingWall == Wall)
		{
			PendingWall = nullptr;

			RoomNodes.Remove(Wall->StartNode);
			Wall->StartNode->Destroy();

			Wall->WidthDimensionString->Destroy();
			Wall->HeightDimensionString->Destroy();
			for (ADimensionStringBase* String : Wall->FixtureDimensionStrings)
			{
				String->Destroy();
			}
			Wall->FixtureDimensionStrings.Empty();

			RoomNodes.Remove(Wall->EndNode);
			Wall->EndNode->Destroy();

			Walls.Remove(Wall);
			Wall->Destroy();
		}
		else
		{
			ensureAlwaysMsgf(false, TEXT("We do not support rebuilding the wall connectivity after deleting fully placed walls!"));
		}
	}
}

AWall* UEditManager::SpawnWall(const FVector& Origin)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWall* NewWall = GetWorld()->SpawnActor<AWall>(WallClass, FTransform(FQuat::Identity, Origin), SpawnParams);
	NewWall->StartNode = CreateNodeAtPoint(Origin);
	NewWall->StartNode->ConnectWall(NewWall, true);
	NewWall->EndNode = CreateNodeAtPoint(Origin);
	NewWall->EndNode->ConnectWall(NewWall, true);

	return NewWall;
}

bool UEditManager::GetIntersectingWalls(AWall* QueryWall, TArray<FWallIntersection>& OutIntersections)
{
	// TODO: reduce search space from all walls
	FWallIntersection TempIntersection;
	for (AWall* OtherWall : Walls)
	{
		if (OtherWall != QueryWall && QueryWall->GetWallIntersection2D(OtherWall, TempIntersection))
		{
			OutIntersections.Add(TempIntersection);
		}
	}

	OutIntersections.Sort([](const FWallIntersection& A, const FWallIntersection& B) {
		return A.DistAlongQueryWall < B.DistAlongQueryWall;
	});

	return (OutIntersections.Num() > 0);
}

void UEditManager::OnWallMoved(AWall* ChangedWall)
{
	// Search through all existing walls to see if any of them intersect with the changed wall.
	

	ResetWallConnectivity(ChangedWall);
}

bool UEditManager::ResetWallConnectivity(AWall* InWall)
{
	FVector InWallStart = InWall->GetWallStart();
	FVector InWallEnd = InWall->GetWallEnd();
	bool bChanged = false;

	ARoomNode* InWallStartNode = InWall->StartNode;
	if (ARoomNode* ExistingStartNode = FindNodeAtPoint(InWallStart, InWallStartNode))
	{
		if (ExistingStartNode->MergeNode(InWallStartNode))
		{
			RoomNodes.Remove(InWallStartNode);
			bChanged = true;
		}
	}

	ARoomNode* InWallEndNode = InWall->EndNode;
	if (ARoomNode* ExistingEndNode = FindNodeAtPoint(InWallEnd, InWallEndNode))
	{
		if (ExistingEndNode->MergeNode(InWallEndNode))
		{
			RoomNodes.Remove(InWallEndNode);
			bChanged = true;
		}
	}

	for (AWall* ConnectedWall : InWall->StartNode->SortedWalls)
	{
		if (ConnectedWall->SortConnectedWalls())
		{
			bChanged = true;
		}
	}

	for (AWall* ConnectedWall : InWall->EndNode->SortedWalls)
	{
		if (ConnectedWall->SortConnectedWalls())
		{
			bChanged = true;
		}
	}

	return bChanged;
}


void UEditManager::GenterateWall(UProceduralMeshComponent* WallMesh, float height, float thickness, AWall* PendingWallActor)
{

	WallMesh->bUseAsyncCooking = true;

	FVector WallDeltaStart = PendingWallActor->EndPoint - PendingWallActor->StartPoint;
	FVector crosL = FVector::CrossProduct(FVector::UpVector, WallDeltaStart);

	FVector crosLNorm = (crosL).GetUnsafeNormal();

	FVector LeftSideStart	= PendingWallActor->StartPoint	- (thickness * crosLNorm);
	FVector RightSideStart	= PendingWallActor->StartPoint	+ (thickness * crosLNorm);
	FVector LeftSideEnd		= PendingWallActor->EndPoint	- (thickness * crosLNorm);
	FVector RightSideEnd	= PendingWallActor->EndPoint	+ (thickness * crosLNorm);
	
	TArray<FVector> vertices;
	vertices.Add(LeftSideStart);
	vertices.Add(RightSideStart);
	vertices.Add(RightSideEnd);
	vertices.Add(LeftSideEnd);

	FWallBox currentWall;

	currentWall.b_LeftSideStart		= LeftSideStart;
	currentWall.b_RightSideStart	= RightSideStart;
	currentWall.b_LeftSideEnd		= LeftSideEnd;
	currentWall.b_RightSideEnd		= RightSideEnd;


	TArray<int32> Triangle;
	//bottomTriangle
	Triangle.Add(0);
	Triangle.Add(2);
	Triangle.Add(1);

	Triangle.Add(0);
	Triangle.Add(3);
	Triangle.Add(2);

	//TopTriangle
	Triangle.Add(4);
	Triangle.Add(5);
	Triangle.Add(6);

	Triangle.Add(4);
	Triangle.Add(6);
	Triangle.Add(7);


	for (int i = 0; i < vertices.Num(); i++)
	{
		if (i < vertices.Num() - 1)
		{
			Triangle.Add(i + (vertices.Num()));
			Triangle.Add(i);
			Triangle.Add(i + 1);

			Triangle.Add(i + (vertices.Num()));
			Triangle.Add(i + 1);
			Triangle.Add(i + (vertices.Num() + 1));
		}
		else
		{
			Triangle.Add(i + (vertices.Num()));
			Triangle.Add(i);
			Triangle.Add(i + 1);

			Triangle.Add(vertices.Num());
			Triangle.Add(i);
			Triangle.Add(0);
		}
	}

	//setup top verts
	vertices.Add(FVector(LeftSideStart.X,	LeftSideStart.Y,	height * 12.0f * 2.54f));
	vertices.Add(FVector(RightSideStart.X,	RightSideStart.Y,	height * 12.0f * 2.54f));
	vertices.Add(FVector(RightSideEnd.X,	RightSideEnd.Y,		height * 12.0f * 2.54f));
	vertices.Add(FVector(LeftSideEnd.X,		LeftSideEnd.Y,		height * 12.0f * 2.54f));

	currentWall.t_LeftSideStart = FVector(LeftSideStart.X, LeftSideStart.Y, height * 12.0f * 2.54f);
	currentWall.t_RightSideStart = FVector(RightSideStart.X, RightSideStart.Y, height * 12.0f * 2.54f);
	currentWall.t_RightSideEnd = FVector(RightSideEnd.X, RightSideEnd.Y, height * 12.0f * 2.54f);
	currentWall.t_LeftSideEnd = FVector(LeftSideEnd.X, LeftSideEnd.Y, height * 12.0f * 2.54f);

	currentWall.wallVertices = vertices;
	currentWall.Triangles = Triangle;
	PendingWallActor->WallBoxes.Add(currentWall);

	////setup the rest of the variables
	////TODO PEYVAN Figure out algo to fix \/

	TArray<FVector> normals;
	TArray<FVector2D> UV0;

	for (int i = 0; i < vertices.Num(); i++)
	{
		FVector Vertex = vertices[i];
		FVector DeltaFromStart = Vertex - PendingWallActor->StartPoint;

		normals.Add(DeltaFromStart.ProjectOnTo(crosLNorm));

		float distanceXY = DeltaFromStart.Size2D();
		float distanceZ = Vertex.Z;

		UV0.Add(FVector2D(
			FMath::Abs(0.01f * distanceXY),
			FMath::Abs(0.01f * distanceZ)
		));
	}
	TArray<FProcMeshTangent> tangents;
	TArray<FLinearColor> vertexColors;
	//for (int i = 0; i != Triangle.Num(); i++)
	//{
	//	FMath::Lerp(PendingWallActor->StartPoint, PendingWallActor->EndPoint, )
	//	vertexColors.Add(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	//}

	////TODO PEYVAN Figure out algo to fix /\

	tangents.Empty();
	vertexColors.Empty();

	PendingWallActor->wallThickness = thickness;
	PendingWallActor->wallVertices = vertices;
	WallMesh->CreateMeshSection_LinearColor(0, PendingWallActor->wallVertices, Triangle, normals, UV0, vertexColors, tangents, true);
	//CaseWorkLines.Empty();
	//CaseworkCompletes.Add(CaseworkGeneratedActor);
}

void UEditManager::CutWindowIntoWall(UProceduralMeshComponent* WallMesh, AWall* CurrentWallActor, FVector Origin, FVector BoxExtend, bool bIsDoor, bool isPreview)
{
	FVector WallDeltaStart = CurrentWallActor->EndPoint - CurrentWallActor->StartPoint;
	FVector crosL = FVector::CrossProduct(FVector::UpVector, WallDeltaStart);

	FVector crosLNorm = (crosL).GetUnsafeNormal();
	WallDeltaStart.Normalize();

	float HighestPoint = 0.0f;
	for (size_t i = 0; i < CurrentWallActor->wallVertices.Num(); i++)
	{
		if (HighestPoint < CurrentWallActor->wallVertices[i].Z)
		{
			HighestPoint = CurrentWallActor->wallVertices[i].Z;
		}
	}
	FVector NewOrigin = FVector::PointPlaneProject(Origin, CurrentWallActor->StartPoint, CurrentWallActor->GetActorRightVector());
	
	//Getnew verts
	//Centerpoint + Unit(0,0,1)*HeightBound + Unit(WallDir)*WidthBound
	//topright
	FVector TopEnd = NewOrigin + FVector::UpVector * BoxExtend.Z + WallDeltaStart * BoxExtend.X;
	//topleft
	FVector TopStart = NewOrigin + FVector::UpVector * BoxExtend.Z - WallDeltaStart * BoxExtend.X;
	//bottom right
	FVector BottomEnd = NewOrigin - FVector::UpVector * BoxExtend.Z + WallDeltaStart * BoxExtend.X;
	//bottom left
	FVector BottomStart = NewOrigin - FVector::UpVector * BoxExtend.Z - WallDeltaStart * BoxExtend.X;

	FIntersectionPoints Window;
	if (FVector::Dist(CurrentWallActor->EndPoint, CurrentWallActor->StartPoint) < FVector::Dist(CurrentWallActor->EndPoint, BottomStart) ||
		FVector::Dist(CurrentWallActor->EndPoint, CurrentWallActor->StartPoint) < FVector::Dist(CurrentWallActor->StartPoint, BottomEnd))
	{
		TArray<int32> Triangle;
		CurrentWallActor->wallVertices.Empty();

		for (size_t i = 0; i < CurrentWallActor->WallBoxes.Num(); i++)
		{
			TriangulateWallBoxes(CurrentWallActor->wallVertices.Num(), 4, Triangle);

			CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].b_LeftSideStart);
			CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].b_RightSideStart);
			CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].b_RightSideEnd);
			CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].b_LeftSideEnd);
			CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].t_LeftSideStart);
			CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].t_RightSideStart);
			CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].t_RightSideEnd);
			CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].t_LeftSideEnd);
		}


		TArray<FVector> normals;
		TArray<FVector2D> UV0;

		for (int i = 0; i < CurrentWallActor->wallVertices.Num(); i++)
		{
			FVector Vertex = CurrentWallActor->wallVertices[i];
			FVector DeltaFromStart = Vertex - CurrentWallActor->StartPoint;

			normals.Add(DeltaFromStart.ProjectOnTo(crosLNorm));

			float distanceXY = DeltaFromStart.Size2D();
			float distanceZ = Vertex.Z;

			UV0.Add(FVector2D(
				FMath::Abs(0.01f * distanceXY),
				FMath::Abs(0.01f * distanceZ)
			));
		}

		TArray<FProcMeshTangent> tangents;
		TArray<FLinearColor> vertexColors;
		//for (int i = 0; i != Triangle.Num(); i++)
		//{
		//	vertexColors.Add(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
		//}

		////TODO PEYVAN Figure out algo to fix /\

		tangents.Empty();
		vertexColors.Empty();

		WallMesh->CreateMeshSection_LinearColor(0, CurrentWallActor->wallVertices, Triangle, normals, UV0, vertexColors, tangents, true);

	}
	else
	{

		if (FVector::Dist(CurrentWallActor->StartPoint, BottomStart) < FVector::Dist(CurrentWallActor->StartPoint, BottomEnd))
		{
			Window.b_LeftSideStart = BottomStart - (CurrentWallActor->wallThickness * crosLNorm);
			Window.b_RightSideStart = BottomStart + (CurrentWallActor->wallThickness * crosLNorm);
			Window.b_LeftSideEnd = BottomEnd - (CurrentWallActor->wallThickness * crosLNorm);
			Window.b_RightSideEnd = BottomEnd + (CurrentWallActor->wallThickness * crosLNorm);

			Window.t_LeftSideStart = FVector(Window.b_LeftSideStart.X, Window.b_LeftSideStart.Y, (HighestPoint < Window.b_LeftSideStart.Z + BoxExtend.Z * 2) ? HighestPoint : Window.b_LeftSideStart.Z + BoxExtend.Z * 2);
			Window.t_RightSideStart = FVector(Window.b_RightSideStart.X, Window.b_RightSideStart.Y, (HighestPoint < Window.b_RightSideStart.Z + BoxExtend.Z * 2) ? HighestPoint : Window.b_RightSideStart.Z + BoxExtend.Z * 2);
			Window.t_LeftSideEnd = FVector(Window.b_LeftSideEnd.X, Window.b_LeftSideEnd.Y, (HighestPoint < Window.b_LeftSideEnd.Z + BoxExtend.Z * 2) ? HighestPoint : Window.b_LeftSideEnd.Z + BoxExtend.Z * 2);
			Window.t_RightSideEnd = FVector(Window.b_RightSideEnd.X, Window.b_RightSideEnd.Y, (HighestPoint < Window.b_RightSideEnd.Z + BoxExtend.Z * 2) ? HighestPoint : Window.b_RightSideEnd.Z + BoxExtend.Z * 2);
		}
		else
		{
			Window.b_LeftSideStart = BottomStart - (CurrentWallActor->wallThickness * crosLNorm);
			Window.b_RightSideStart = BottomStart + (CurrentWallActor->wallThickness * crosLNorm);
			Window.b_LeftSideEnd = BottomEnd - (CurrentWallActor->wallThickness * crosLNorm);
			Window.b_RightSideEnd = BottomEnd + (CurrentWallActor->wallThickness * crosLNorm);

			Window.t_LeftSideStart = FVector(Window.b_LeftSideStart.X, Window.b_LeftSideStart.Y, (HighestPoint < Window.b_LeftSideStart.Z + BoxExtend.Z * 2) ? HighestPoint : Window.b_LeftSideStart.Z + BoxExtend.Z * 2);
			Window.t_RightSideStart = FVector(Window.b_RightSideStart.X, Window.b_RightSideStart.Y, (HighestPoint < Window.b_RightSideStart.Z + BoxExtend.Z * 2) ? HighestPoint : Window.b_RightSideStart.Z + BoxExtend.Z * 2);
			Window.t_LeftSideEnd = FVector(Window.b_LeftSideEnd.X, Window.b_LeftSideEnd.Y, (HighestPoint < Window.b_LeftSideEnd.Z + BoxExtend.Z * 2) ? HighestPoint : Window.b_LeftSideEnd.Z + BoxExtend.Z * 2);
			Window.t_RightSideEnd = FVector(Window.b_RightSideEnd.X, Window.b_RightSideEnd.Y, (HighestPoint < Window.b_RightSideEnd.Z + BoxExtend.Z * 2) ? HighestPoint : Window.b_RightSideEnd.Z + BoxExtend.Z * 2);
		}

		//CurrentWallActor->intersections.Add(Window);

		/*TArray<FVector> BLintersectionPoints;
		TArray<FVector> BRintersectionPoints;
		TArray<FVector> TLintersectionPoints;
		TArray<FVector> TRintersectionPoints;*/
		TArray<int32> Triangle;

		CurrentWallActor->wallVertices.Empty();

		if (CurrentWallActor->WallBoxes.Num() > 1)
		{
			int i_currentLocalVertSize = 4;
			TriangulateWallBoxes(CurrentWallActor->wallVertices.Num(), i_currentLocalVertSize, Triangle);

			bool triggered = false;

			TArray<FWallBox> newWallBoxList;


			//for loop here
			for (size_t i = 0; i < CurrentWallActor->WallBoxes.Num(); i++)
			{
				if (triggered || FVector::Dist(CurrentWallActor->WallBoxes[i].b_LeftSideStart, CurrentWallActor->WallBoxes[i].b_LeftSideEnd) > FVector::Dist(CurrentWallActor->WallBoxes[i].b_LeftSideStart, Window.b_LeftSideEnd))
				{

					//get box effected and modify it into 4 boxes. 
					if (!triggered)
					{

#pragma region Starting Wall
						FWallBox _startingWall;

						_startingWall.b_LeftSideStart = CurrentWallActor->WallBoxes[i].b_LeftSideStart;
						_startingWall.b_RightSideStart = CurrentWallActor->WallBoxes[i].b_RightSideStart;
						_startingWall.b_LeftSideEnd = FVector(Window.b_LeftSideStart.X, Window.b_LeftSideStart.Y, 0);
						_startingWall.b_RightSideEnd = FVector(Window.b_RightSideStart.X, Window.b_RightSideStart.Y, 0);

						_startingWall.t_LeftSideStart = CurrentWallActor->WallBoxes[i].t_LeftSideStart;
						_startingWall.t_RightSideStart = CurrentWallActor->WallBoxes[i].t_RightSideStart;
						_startingWall.t_LeftSideEnd = FVector(Window.t_LeftSideStart.X, Window.t_LeftSideStart.Y, CurrentWallActor->WallBoxes[i].t_LeftSideStart.Z);
						_startingWall.t_RightSideEnd = FVector(Window.t_RightSideStart.X, Window.t_RightSideStart.Y, CurrentWallActor->WallBoxes[i].t_LeftSideStart.Z);

						_startingWall.AddVerts();

						//getTriangles here


						TriangulateWallBoxes(CurrentWallActor->wallVertices.Num(), i_currentLocalVertSize, Triangle);

						CurrentWallActor->wallVertices.Add(_startingWall.b_LeftSideStart);
						CurrentWallActor->wallVertices.Add(_startingWall.b_RightSideStart);
						CurrentWallActor->wallVertices.Add(_startingWall.b_RightSideEnd);
						CurrentWallActor->wallVertices.Add(_startingWall.b_LeftSideEnd);
						CurrentWallActor->wallVertices.Add(_startingWall.t_LeftSideStart);
						CurrentWallActor->wallVertices.Add(_startingWall.t_RightSideStart);
						CurrentWallActor->wallVertices.Add(_startingWall.t_RightSideEnd);
						CurrentWallActor->wallVertices.Add(_startingWall.t_LeftSideEnd);


#pragma endregion

#pragma region MiddleTop Box
						FWallBox _middleWallTop;
						_middleWallTop.b_LeftSideStart = Window.t_LeftSideStart;
						_middleWallTop.b_RightSideStart = Window.t_RightSideStart;
						_middleWallTop.b_LeftSideEnd = Window.t_LeftSideEnd;
						_middleWallTop.b_RightSideEnd = Window.t_RightSideEnd;

						_middleWallTop.t_LeftSideStart = FVector(Window.t_LeftSideStart.X, Window.t_LeftSideStart.Y, CurrentWallActor->WallBoxes[i].t_LeftSideStart.Z);
						_middleWallTop.t_RightSideStart = FVector(Window.t_RightSideStart.X, Window.t_RightSideStart.Y, CurrentWallActor->WallBoxes[i].t_LeftSideStart.Z);
						_middleWallTop.t_LeftSideEnd = FVector(Window.t_LeftSideEnd.X, Window.t_LeftSideEnd.Y, CurrentWallActor->WallBoxes[i].t_LeftSideEnd.Z);
						_middleWallTop.t_RightSideEnd = FVector(Window.t_RightSideEnd.X, Window.t_RightSideEnd.Y, CurrentWallActor->WallBoxes[i].t_LeftSideEnd.Z);

						_middleWallTop.AddVerts();

						//getTriangles here

						TriangulateWallBoxes(CurrentWallActor->wallVertices.Num(), i_currentLocalVertSize, Triangle);


						CurrentWallActor->wallVertices.Add(_middleWallTop.b_LeftSideStart);
						CurrentWallActor->wallVertices.Add(_middleWallTop.b_RightSideStart);
						CurrentWallActor->wallVertices.Add(_middleWallTop.b_RightSideEnd);
						CurrentWallActor->wallVertices.Add(_middleWallTop.b_LeftSideEnd);
						CurrentWallActor->wallVertices.Add(_middleWallTop.t_LeftSideStart);
						CurrentWallActor->wallVertices.Add(_middleWallTop.t_RightSideStart);
						CurrentWallActor->wallVertices.Add(_middleWallTop.t_RightSideEnd);
						CurrentWallActor->wallVertices.Add(_middleWallTop.t_LeftSideEnd);



#pragma endregion

#pragma region MiddleBottomBox
						FWallBox _middleWallBottom;
						if (!bIsDoor)
						{

							_middleWallBottom.b_LeftSideStart = FVector(Window.b_LeftSideStart.X, Window.b_LeftSideStart.Y, 0);
							_middleWallBottom.b_RightSideStart = FVector(Window.b_RightSideStart.X, Window.b_RightSideStart.Y, 0);
							_middleWallBottom.b_LeftSideEnd = FVector(Window.b_LeftSideEnd.X, Window.b_LeftSideEnd.Y, 0);
							_middleWallBottom.b_RightSideEnd = FVector(Window.b_RightSideEnd.X, Window.b_RightSideEnd.Y, 0);

							_middleWallBottom.t_LeftSideStart = Window.b_LeftSideStart;
							_middleWallBottom.t_RightSideStart = Window.b_RightSideStart;
							_middleWallBottom.t_LeftSideEnd = Window.b_LeftSideEnd;
							_middleWallBottom.t_RightSideEnd = Window.b_RightSideEnd;
							_middleWallBottom.AddVerts();

							//getTriangles here

							TriangulateWallBoxes(CurrentWallActor->wallVertices.Num(), i_currentLocalVertSize, Triangle);


							CurrentWallActor->wallVertices.Add(_middleWallBottom.b_LeftSideStart);
							CurrentWallActor->wallVertices.Add(_middleWallBottom.b_RightSideStart);
							CurrentWallActor->wallVertices.Add(_middleWallBottom.b_RightSideEnd);
							CurrentWallActor->wallVertices.Add(_middleWallBottom.b_LeftSideEnd);
							CurrentWallActor->wallVertices.Add(_middleWallBottom.t_LeftSideStart);
							CurrentWallActor->wallVertices.Add(_middleWallBottom.t_RightSideStart);
							CurrentWallActor->wallVertices.Add(_middleWallBottom.t_RightSideEnd);
							CurrentWallActor->wallVertices.Add(_middleWallBottom.t_LeftSideEnd);
						}

#pragma endregion

#pragma region EndWall

						FWallBox _endingWall;

						_endingWall.b_LeftSideStart = FVector(Window.b_LeftSideEnd.X, Window.b_LeftSideEnd.Y, 0);
						_endingWall.b_RightSideStart = FVector(Window.b_RightSideEnd.X, Window.b_RightSideEnd.Y, 0);
						_endingWall.b_LeftSideEnd = CurrentWallActor->WallBoxes[i].b_LeftSideEnd;
						_endingWall.b_RightSideEnd = CurrentWallActor->WallBoxes[i].b_RightSideEnd;


						_endingWall.t_LeftSideStart = FVector(Window.t_LeftSideEnd.X, Window.t_LeftSideEnd.Y, HighestPoint);
						_endingWall.t_RightSideStart = FVector(Window.t_RightSideEnd.X, Window.t_RightSideEnd.Y, HighestPoint);
						_endingWall.t_LeftSideEnd = CurrentWallActor->WallBoxes[i].t_LeftSideEnd;
						_endingWall.t_RightSideEnd = CurrentWallActor->WallBoxes[i].t_RightSideEnd;

						_endingWall.AddVerts();


						//getTriangles here

						TriangulateWallBoxes(CurrentWallActor->wallVertices.Num(), i_currentLocalVertSize, Triangle);


						CurrentWallActor->wallVertices.Add(_endingWall.b_LeftSideStart);
						CurrentWallActor->wallVertices.Add(_endingWall.b_RightSideStart);
						CurrentWallActor->wallVertices.Add(_endingWall.b_RightSideEnd);
						CurrentWallActor->wallVertices.Add(_endingWall.b_LeftSideEnd);
						CurrentWallActor->wallVertices.Add(_endingWall.t_LeftSideStart);
						CurrentWallActor->wallVertices.Add(_endingWall.t_RightSideStart);
						CurrentWallActor->wallVertices.Add(_endingWall.t_RightSideEnd);
						CurrentWallActor->wallVertices.Add(_endingWall.t_LeftSideEnd);

#pragma endregion 

						if (!isPreview)
						{
							newWallBoxList.Add(_startingWall);
							newWallBoxList.Add(_middleWallTop);
							if (!bIsDoor)
							{
								newWallBoxList.Add(_middleWallBottom);
							}
							newWallBoxList.Add(_endingWall);
						}
					}
					else
					{
						TriangulateWallBoxes(CurrentWallActor->wallVertices.Num(), i_currentLocalVertSize, Triangle);


						CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].b_LeftSideStart);
						CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].b_RightSideStart);
						CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].b_RightSideEnd);
						CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].b_LeftSideEnd);
						CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].t_LeftSideStart);
						CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].t_RightSideStart);
						CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].t_RightSideEnd);
						CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].t_LeftSideEnd);
						newWallBoxList.Add(CurrentWallActor->WallBoxes[i]);
					}


					//add back in the rest of the boxes with modified values

					//triangulate and add verts to 

					//break;
					triggered = true;
				}
				else
				{
					//add unmodified box to verts and triangulate

					TriangulateWallBoxes(CurrentWallActor->wallVertices.Num(), i_currentLocalVertSize, Triangle);


					CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].b_LeftSideStart);
					CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].b_RightSideStart);
					CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].b_RightSideEnd);
					CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].b_LeftSideEnd);
					CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].t_LeftSideStart);
					CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].t_RightSideStart);
					CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].t_RightSideEnd);
					CurrentWallActor->wallVertices.Add(CurrentWallActor->WallBoxes[i].t_LeftSideEnd);


					if (!isPreview)
					{
						newWallBoxList.Add(CurrentWallActor->WallBoxes[i]);
					}
				}


			}

			if (!isPreview)
			{
				CurrentWallActor->WallBoxes.Empty();

				for (size_t i = 0; i < newWallBoxList.Num(); i++)
				{
					CurrentWallActor->WallBoxes.Add(newWallBoxList[i]);
				}
			}

			//clearBoxList and add new stuff







		}
		else
		{

#pragma region Starting Wall
			FWallBox _startingWall;

			_startingWall.b_LeftSideStart = CurrentWallActor->WallBoxes[0].b_LeftSideStart;
			_startingWall.b_RightSideStart = CurrentWallActor->WallBoxes[0].b_RightSideStart;
			_startingWall.b_LeftSideEnd = FVector(Window.b_LeftSideStart.X, Window.b_LeftSideStart.Y, 0);
			_startingWall.b_RightSideEnd = FVector(Window.b_RightSideStart.X, Window.b_RightSideStart.Y, 0);

			_startingWall.t_LeftSideStart = CurrentWallActor->WallBoxes[0].t_LeftSideStart;
			_startingWall.t_RightSideStart = CurrentWallActor->WallBoxes[0].t_RightSideStart;
			_startingWall.t_LeftSideEnd = FVector(Window.t_LeftSideStart.X, Window.t_LeftSideStart.Y, CurrentWallActor->WallBoxes[0].t_LeftSideStart.Z);
			_startingWall.t_RightSideEnd = FVector(Window.t_RightSideStart.X, Window.t_RightSideStart.Y, CurrentWallActor->WallBoxes[0].t_LeftSideStart.Z);

			_startingWall.AddVerts();

			//getTriangles here
			//bottomTriangle

			int i_currentLocalVertSize = 4;
			TriangulateWallBoxes(CurrentWallActor->wallVertices.Num(), i_currentLocalVertSize, Triangle);


			CurrentWallActor->wallVertices.Add(_startingWall.b_LeftSideStart);
			CurrentWallActor->wallVertices.Add(_startingWall.b_RightSideStart);
			CurrentWallActor->wallVertices.Add(_startingWall.b_RightSideEnd);
			CurrentWallActor->wallVertices.Add(_startingWall.b_LeftSideEnd);
			CurrentWallActor->wallVertices.Add(_startingWall.t_LeftSideStart);
			CurrentWallActor->wallVertices.Add(_startingWall.t_RightSideStart);
			CurrentWallActor->wallVertices.Add(_startingWall.t_RightSideEnd);
			CurrentWallActor->wallVertices.Add(_startingWall.t_LeftSideEnd);


#pragma endregion

#pragma region MiddleTop Box
			FWallBox _middleWallTop;
			_middleWallTop.b_LeftSideStart = Window.t_LeftSideStart;
			_middleWallTop.b_RightSideStart = Window.t_RightSideStart;
			_middleWallTop.b_LeftSideEnd = Window.t_LeftSideEnd;
			_middleWallTop.b_RightSideEnd = Window.t_RightSideEnd;

			_middleWallTop.t_LeftSideStart = FVector(Window.t_LeftSideStart.X, Window.t_LeftSideStart.Y, HighestPoint);
			_middleWallTop.t_RightSideStart = FVector(Window.t_RightSideStart.X, Window.t_RightSideStart.Y, HighestPoint);
			_middleWallTop.t_LeftSideEnd = FVector(Window.t_LeftSideEnd.X, Window.t_LeftSideEnd.Y, HighestPoint);
			_middleWallTop.t_RightSideEnd = FVector(Window.t_RightSideEnd.X, Window.t_RightSideEnd.Y, HighestPoint);

			_middleWallTop.AddVerts();
			//getTriangles here
			TriangulateWallBoxes(CurrentWallActor->wallVertices.Num(), i_currentLocalVertSize, Triangle);


			CurrentWallActor->wallVertices.Add(_middleWallTop.b_LeftSideStart);
			CurrentWallActor->wallVertices.Add(_middleWallTop.b_RightSideStart);
			CurrentWallActor->wallVertices.Add(_middleWallTop.b_RightSideEnd);
			CurrentWallActor->wallVertices.Add(_middleWallTop.b_LeftSideEnd);
			CurrentWallActor->wallVertices.Add(_middleWallTop.t_LeftSideStart);
			CurrentWallActor->wallVertices.Add(_middleWallTop.t_RightSideStart);
			CurrentWallActor->wallVertices.Add(_middleWallTop.t_RightSideEnd);
			CurrentWallActor->wallVertices.Add(_middleWallTop.t_LeftSideEnd);



#pragma endregion

#pragma region MiddleBottomBox

			FWallBox _middleWallBottom;
			if (!bIsDoor)
			{
				_middleWallBottom.b_LeftSideStart = FVector(Window.b_LeftSideStart.X, Window.b_LeftSideStart.Y, 0);
				_middleWallBottom.b_RightSideStart = FVector(Window.b_RightSideStart.X, Window.b_RightSideStart.Y, 0);
				_middleWallBottom.b_LeftSideEnd = FVector(Window.b_LeftSideEnd.X, Window.b_LeftSideEnd.Y, 0);
				_middleWallBottom.b_RightSideEnd = FVector(Window.b_RightSideEnd.X, Window.b_RightSideEnd.Y, 0);

				_middleWallBottom.t_LeftSideStart = Window.b_LeftSideStart;
				_middleWallBottom.t_RightSideStart = Window.b_RightSideStart;
				_middleWallBottom.t_LeftSideEnd = Window.b_LeftSideEnd;
				_middleWallBottom.t_RightSideEnd = Window.b_RightSideEnd;
				_middleWallBottom.AddVerts();

				//getTriangles here

				TriangulateWallBoxes(CurrentWallActor->wallVertices.Num(), i_currentLocalVertSize, Triangle);


				CurrentWallActor->wallVertices.Add(_middleWallBottom.b_LeftSideStart);
				CurrentWallActor->wallVertices.Add(_middleWallBottom.b_RightSideStart);
				CurrentWallActor->wallVertices.Add(_middleWallBottom.b_RightSideEnd);
				CurrentWallActor->wallVertices.Add(_middleWallBottom.b_LeftSideEnd);
				CurrentWallActor->wallVertices.Add(_middleWallBottom.t_LeftSideStart);
				CurrentWallActor->wallVertices.Add(_middleWallBottom.t_RightSideStart);
				CurrentWallActor->wallVertices.Add(_middleWallBottom.t_RightSideEnd);
				CurrentWallActor->wallVertices.Add(_middleWallBottom.t_LeftSideEnd);
			}

#pragma endregion

#pragma region EndWall

			FWallBox _endingWall;

			_endingWall.b_LeftSideStart = FVector(Window.b_LeftSideEnd.X, Window.b_LeftSideEnd.Y, 0);
			_endingWall.b_RightSideStart = FVector(Window.b_RightSideEnd.X, Window.b_RightSideEnd.Y, 0);
			_endingWall.b_LeftSideEnd = CurrentWallActor->WallBoxes[0].b_LeftSideEnd;
			_endingWall.b_RightSideEnd = CurrentWallActor->WallBoxes[0].b_RightSideEnd;


			_endingWall.t_LeftSideStart = FVector(Window.t_LeftSideEnd.X, Window.t_LeftSideEnd.Y, HighestPoint);
			_endingWall.t_RightSideStart = FVector(Window.t_RightSideEnd.X, Window.t_RightSideEnd.Y, HighestPoint);
			_endingWall.t_LeftSideEnd = CurrentWallActor->WallBoxes[0].t_LeftSideEnd;
			_endingWall.t_RightSideEnd = CurrentWallActor->WallBoxes[0].t_RightSideEnd;

			_endingWall.AddVerts();


			//getTriangles here

			TriangulateWallBoxes(CurrentWallActor->wallVertices.Num(), i_currentLocalVertSize, Triangle);


			CurrentWallActor->wallVertices.Add(_endingWall.b_LeftSideStart);
			CurrentWallActor->wallVertices.Add(_endingWall.b_RightSideStart);
			CurrentWallActor->wallVertices.Add(_endingWall.b_RightSideEnd);
			CurrentWallActor->wallVertices.Add(_endingWall.b_LeftSideEnd);
			CurrentWallActor->wallVertices.Add(_endingWall.t_LeftSideStart);
			CurrentWallActor->wallVertices.Add(_endingWall.t_RightSideStart);
			CurrentWallActor->wallVertices.Add(_endingWall.t_RightSideEnd);
			CurrentWallActor->wallVertices.Add(_endingWall.t_LeftSideEnd);

#pragma endregion

			if (!isPreview)
			{
				CurrentWallActor->WallBoxes.Empty();
				CurrentWallActor->WallBoxes.Add(_startingWall);
				CurrentWallActor->WallBoxes.Add(_middleWallTop);
				if (!bIsDoor)
				{
					CurrentWallActor->WallBoxes.Add(_middleWallBottom);
				}
				CurrentWallActor->WallBoxes.Add(_endingWall);
			}
			//bottom intersection points




		}


		//setup other things
		TArray<FVector> normals;
		TArray<FVector2D> UV0;

		for (int i = 0; i < CurrentWallActor->wallVertices.Num(); i++)
		{
			FVector Vertex = CurrentWallActor->wallVertices[i];
			FVector DeltaFromStart = Vertex - CurrentWallActor->StartPoint;

			normals.Add(DeltaFromStart.ProjectOnTo(crosLNorm));

			float distanceXY = DeltaFromStart.Size2D();
			float distanceZ = Vertex.Z;

			UV0.Add(FVector2D(
				FMath::Abs(0.01f * distanceXY),
				FMath::Abs(0.01f * distanceZ)
			));
		}

		TArray<FProcMeshTangent> tangents;
		TArray<FLinearColor> vertexColors;
		//for (int i = 0; i != Triangle.Num(); i++)
		//{
		//	vertexColors.Add(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
		//}

		////TODO PEYVAN Figure out algo to fix /\

		tangents.Empty();
		vertexColors.Empty();

		WallMesh->CreateMeshSection_LinearColor(0, CurrentWallActor->wallVertices, Triangle, normals, UV0, vertexColors, tangents, true);
	}
}

void UEditManager::TriangulateWallBoxes(int i_currentWallVertSize, int i_currentLocalVertSize, TArray<int> &Triangle)
{
	
	Triangle.Add(i_currentWallVertSize + 0);
	Triangle.Add(i_currentWallVertSize + 2);
	Triangle.Add(i_currentWallVertSize + 1);

	Triangle.Add(i_currentWallVertSize + 0);
	Triangle.Add(i_currentWallVertSize + 3);
	Triangle.Add(i_currentWallVertSize + 2);

	//TopTriangle
	Triangle.Add(i_currentWallVertSize + 4);
	Triangle.Add(i_currentWallVertSize + 5);
	Triangle.Add(i_currentWallVertSize + 6);

	Triangle.Add(i_currentWallVertSize + 4);
	Triangle.Add(i_currentWallVertSize + 6);
	Triangle.Add(i_currentWallVertSize + 7);

	for (int i = 0; i < i_currentLocalVertSize; i++)
	{
		if (i < i_currentLocalVertSize - 1)
		{

			Triangle.Add(i_currentWallVertSize + i + (i_currentLocalVertSize));
			Triangle.Add(i_currentWallVertSize + i);
			Triangle.Add(i_currentWallVertSize + i + 1);
			Triangle.Add(i_currentWallVertSize + i + (i_currentLocalVertSize));
			Triangle.Add(i_currentWallVertSize + i + 1);
			Triangle.Add(i_currentWallVertSize + i + (i_currentLocalVertSize + 1));
		}
		else
		{
			Triangle.Add(i_currentWallVertSize + i + (i_currentLocalVertSize));
			Triangle.Add(i_currentWallVertSize + i);
			Triangle.Add(i_currentWallVertSize + i + 1);
			Triangle.Add(i_currentWallVertSize + i_currentLocalVertSize);
			Triangle.Add(i_currentWallVertSize + i);
			Triangle.Add(i_currentWallVertSize + 0);
		}
	}
}


/*******
ROOMS
*******/

void UEditManager::UpdateRoomsFromWalls()
{
	// Every planar graph has at least one region: the outer region, which encompasses the whole graph.
	// Every other region of the planar graph will be described by a list of edges, traversed counter-clockwise.
	// Each edge of the graph is therefore adjacent to two regions of the graph.

	// We need enough walls for a planar graph.
	if (Walls.Num() == 0)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Searching rooms..."));

	TSet<ARoom*> DirtyRooms(Rooms);				// The set of rooms that need to be updated, or removed if they are invalid.
	TSet<AWall*> DirtyWalls(Walls);				// The set of walls that need to be traversed in order to update walls.

	// Clear out room references for all walls that have been modified and need to recreate room data
	for (AWall* DirtyWall : DirtyWalls)
	{
		DirtyWall->LeftRoom = nullptr;
		DirtyWall->RightRoom = nullptr;
	}

	int32 CurrentWallIndex = 0;
	AWall* CurrentWall = Walls[CurrentWallIndex];
	bool bCurrentForwards = true;

	FRoomData CurrentRoomData;
	int32 CurrentRoomIndex = Rooms.Num();
	CurrentRoomData.ID = CurrentRoomIndex;

	while (CurrentWall)
	{
		// Add the current wall to the current room
		bool bAddedWall = CurrentRoomData.AddWall(CurrentWall, bCurrentForwards);
		if (!ensureAlways(bAddedWall))
		{
			break;
		}

		AWall* PreviousWall = CurrentWall;
		ARoomNode* NextNode = bCurrentForwards ? CurrentWall->EndNode : CurrentWall->StartNode;

		// Traverse counter-clockwise (for connectivity, actual angle may not be if we're exploring an exterior or concave room)
		CurrentWall = bCurrentForwards ? CurrentWall->DestLeftWall : CurrentWall->SourceRightWall;

		if (!ensureAlways(PreviousWall != CurrentWall && NextNode != nullptr))
		{
			break;
		}

		// If the previous wall and direction lead to a dead end, try to wrap back around.
		if (CurrentWall == nullptr)
		{
			CurrentWall = PreviousWall;
			bCurrentForwards = !bCurrentForwards;
		}
		// Otherwise figure out the next wall direction
		else
		{
			bool bNextComesFromCurrent = (CurrentWall->StartNode == NextNode);
			bool bNextGoesToCurrent = (CurrentWall->EndNode == NextNode);

			if (!ensureAlways(bNextComesFromCurrent != bNextGoesToCurrent))
			{
				break;
			}

			bCurrentForwards = bNextComesFromCurrent;
		}

		// See if our next wall has already been included in any other rooms
		bool bCurWallHasRoom = bCurrentForwards ? (CurrentWall->LeftRoom != nullptr) : (CurrentWall->RightRoom != nullptr);
		int32 CurrentRoomNextWallIndex = -1;
		if (bCurWallHasRoom)
		{
			ensureAlways(false);
			CurrentWall = nullptr;
			break;
		}
		// Check if we've finished the room by reaching the original wall
		else if (CurrentRoomData.ContainsWall(CurrentWall, bCurrentForwards, CurrentRoomNextWallIndex))
		{
			// Rooms should end with their starting wall and be closed by the time we reach it
			ensureAlways(CurrentRoomData.bClosed && CurrentRoomNextWallIndex == 0);

			// If this finished room is identical to an existing room, then don't create a new one
			ARoom* UpdatedRoom = FindRoomFromData(CurrentRoomData);
			if (UpdatedRoom == nullptr)
			{
				// Otherwise, try to find the existing room that shares the most of the current data's walls,
				// and update that room to use this one's data.
				int32 NumSharedWalls = 0;
				UpdatedRoom = FindMostSimilarRoom(CurrentRoomData, NumSharedWalls);
				if ((UpdatedRoom != nullptr) && (NumSharedWalls > 0) && DirtyRooms.Contains(UpdatedRoom))
				{
					CurrentRoomData.ID = UpdatedRoom->RoomData.ID;
					UpdatedRoom->UpdateRoomData(CurrentRoomData);
				}
				// If there is no room that shares enough walls, then create a new room.
				else
				{
					UpdatedRoom = CreateRoomFromData(CurrentRoomData);
				}

				
				if (!UpdatedRoom->RoomData.IsClockWise())
				{
					UpdatedRoom->DebugDraw();
				}
			}
			ensureAlways(UpdatedRoom != nullptr);

			// Update this room's wall associations
			for (int32 CurRoomWallIndex = 0; CurRoomWallIndex < UpdatedRoom->RoomData.WallsOrdered.Num(); ++CurRoomWallIndex)
			{
				AWall* CurRoomWall = UpdatedRoom->RoomData.WallsOrdered[CurRoomWallIndex];
				bool bCurRoomWallForwards = UpdatedRoom->RoomData.WallDirections[CurRoomWallIndex];

				if (bCurRoomWallForwards)
				{
					ensureAlways(CurRoomWall->LeftRoom == nullptr);
					CurRoomWall->LeftRoom = UpdatedRoom;
				}
				else
				{
					ensureAlways(CurRoomWall->RightRoom == nullptr);
					CurRoomWall->RightRoom = UpdatedRoom;
				}
			}

			// Mark the updated room as no longer being dirty
			if (DirtyRooms.Contains(UpdatedRoom))
			{
				DirtyRooms.Remove(UpdatedRoom);
			}

			// Clear out the current wall since we need to find a new starting point
			CurrentWall = nullptr;
		}

		if (!CurrentWall)
		{
			// Find the next wall to start traversing rooms from
			bCurrentForwards = true;
			for (int32 NextWallIndex = CurrentWallIndex; NextWallIndex < Walls.Num();)
			{
				AWall* NextWall = Walls[NextWallIndex];
				bool bNextWallHasRoom = bCurrentForwards ? (NextWall->LeftRoom != nullptr) : (NextWall->RightRoom != nullptr);

				if (bNextWallHasRoom)
				{
					if (bCurrentForwards)
					{
						bCurrentForwards = false;
					}
					else
					{
						++NextWallIndex;
						bCurrentForwards = true;
					}
				}
				else
				{
					// We've found an available starting wall that hasn't been in any rooms yet
					CurrentWall = NextWall;

					// Initialize the next room
					CurrentRoomData = FRoomData();
					CurrentRoomData.ID = ++CurrentRoomIndex;

					break;
				}
			}
		}
	}

	// TODO: if room data has changed, then update existing rooms that share data.
	// For now, just create new rooms if data has changed, and destroy rooms that are no longer valid.
	for (ARoom* DirtyRoom : DirtyRooms)
	{
		DirtyRoom->Destroy();
		Rooms.Remove(DirtyRoom);
	}

	UE_LOG(LogTemp, Log, TEXT("... Done searching rooms. There are now %d total rooms."), Rooms.Num());

}

ARoom* UEditManager::CreateRoomFromData(const FRoomData& RoomData)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ARoom* NewRoom = GetWorld()->SpawnActor<ARoom>(RoomClass, FTransform::Identity, SpawnParams);
	ensureAlways(NewRoom);

	if (NewRoom)
	{
		NewRoom->RoomData = RoomData;
		Rooms.Add(NewRoom);
	}

	return NewRoom;
}

ARoom* UEditManager::FindRoomFromData(const FRoomData& RoomData, bool bCreateIfNotFound)
{
	// TODO: make this not a naive linear search, but there are few enough rooms where it shouldn't matter for a while.
	// If RoomData could had a order-dependent hash of wall directions and IDs, then the rooms could be compared and stored quickly by that hash.
	for (ARoom* Room : Rooms)
	{
		if (Room && Room->RoomData.Equals(RoomData))
		{
			return Room;
		}
	}

	if (bCreateIfNotFound)
	{
		return CreateRoomFromData(RoomData);
	}
	else
	{
		return nullptr;
	}
}

ARoom* UEditManager::FindMostSimilarRoom(const FRoomData& RoomData, int32& NumSharedWalls)
{
	NumSharedWalls = 0;
	ARoom* MostSimilarRoom = nullptr;

	for (ARoom* Room : Rooms)
	{
		int CurSharedWalls = Room->RoomData.CompareWalls(RoomData);
		if (CurSharedWalls > NumSharedWalls)
		{
			NumSharedWalls = CurSharedWalls;
			MostSimilarRoom = Room;
		}
	}

	return MostSimilarRoom;
}

ARoomNode* UEditManager::CreateNodeAtPoint(const FVector& Position)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ARoomNode* NewRoomNode = GetWorld()->SpawnActor<ARoomNode>(RoomNodeClass, FTransform(FQuat::Identity, Position), SpawnParams);
	RoomNodes.Add(NewRoomNode);

	return NewRoomNode;
}

ARoomNode* UEditManager::FindNodeAtPoint(const FVector& Position, ARoomNode* IgnoreNode)
{
	// TODO: search in quad tree if we have lots of nodes and call this frequently.
	for (ARoomNode* RoomNode : RoomNodes)
	{
		if (RoomNode->GetActorLocation().Equals(Position, RoomNodeEpsilon) && RoomNode != IgnoreNode)
		{
			return RoomNode;
		}
	}

	return nullptr;
}

ARoomNode* UEditManager::FindOrCreateNodeAtPoint(const FVector& Position)
{
	if (ARoomNode* FoundNode = FindNodeAtPoint(Position))
	{
		return FoundNode;
	}

	// There's no existing node near the query position, so make a new one.
	return CreateNodeAtPoint(Position);
}

/*******
OTHER
*******/

TArray<int32> UEditManager::Triangulate(TArray<FVector> vertices)
{
	/** Decomposes the polygon into triangles with a naive ear-clipping algorithm. Does not handle internal holes in the polygon.
	Based on the implementation in Engine/Source/Runtime/Engine/Private/GeomTools.cpp, Copyright 1998-2017 Epic Games, Inc. **/

	bool bKeepColinearVertices = true;
	TArray<int32> SimpleIndices;
	SimpleIndices.Add(0);
	SimpleIndices.Add(1);
	SimpleIndices.Add(2);



	// Can't work if not enough verts for 1 triangle
	if (vertices.Num() < 3)
	{
		// Return true because poly is already a tri
		return SimpleIndices;
	}

	// Vertices of polygon in order - make a copy we are going to modify.
	TArray<FVector> PolyVerts;
	TArray<int32> OriginalVertIndices;
	TArray<int32> TriangleIndices;
	for (int i = 0; i < vertices.Num(); i++)
	{
		PolyVerts.Add(vertices[i]);
		OriginalVertIndices.Add(i);
	}

	// Keep iterating while there are still vertices
	while (true)
	{
		if (!bKeepColinearVertices)
		{
			// Cull redundant vertex edges from the polygon.
			for (int32 VertexIndex = 0; VertexIndex < PolyVerts.Num(); VertexIndex++)
			{
				const int32 I0 = (VertexIndex + 0) % PolyVerts.Num();
				const int32 I1 = (VertexIndex + 1) % PolyVerts.Num();
				const int32 I2 = (VertexIndex + 2) % PolyVerts.Num();
				if (FGraphUtils::AreEdgesMergeable(PolyVerts[I0], PolyVerts[I1], PolyVerts[I2]))
				{
					PolyVerts.RemoveAt(I1);
					OriginalVertIndices.RemoveAt(I1);
					VertexIndex--;
				}
			}
		}

		if (PolyVerts.Num() < 3)
		{
			break;
		}
		else
		{
			// Look for an 'ear' triangle
			bool bFoundEar = false;
			for (int32 EarVertexIndex = 0; EarVertexIndex < PolyVerts.Num(); EarVertexIndex++)
			{
				// Triangle is 'this' vert plus the one before and after it
				const int32 AIndex = (EarVertexIndex == 0) ? PolyVerts.Num() - 1 : EarVertexIndex - 1;
				const int32 BIndex = EarVertexIndex;
				const int32 CIndex = (EarVertexIndex + 1) % PolyVerts.Num();

				// Check that this vertex is convex (cross product must be positive)
				const FVector ABEdge = PolyVerts[BIndex] - PolyVerts[AIndex];
				const FVector ACEdge = PolyVerts[CIndex] - PolyVerts[AIndex];
				const float TriangleDeterminant = (ABEdge ^ ACEdge) | FVector::UpVector;
				if (!FMath::IsNegativeFloat(TriangleDeterminant))
				{
					continue;
				}

				bool bFoundVertInside = false;
				// Look through all verts before this in array to see if any are inside triangle
				for (int32 VertexIndex = 0; VertexIndex < PolyVerts.Num(); VertexIndex++)
				{
					if (VertexIndex != AIndex && VertexIndex != BIndex && VertexIndex != CIndex &&
						FGraphUtils::PointInTriangle(PolyVerts[AIndex], PolyVerts[BIndex], PolyVerts[CIndex], PolyVerts[VertexIndex]))
					{
						bFoundVertInside = true;
						break;
					}
				}

				// Triangle with no verts inside - its an 'ear'! 
				if (!bFoundVertInside)
				{
					// Add to output list..
					TriangleIndices.Add(OriginalVertIndices[AIndex]);
					TriangleIndices.Add(OriginalVertIndices[BIndex]);
					TriangleIndices.Add(OriginalVertIndices[CIndex]);

					// And remove vertex from polygon
					PolyVerts.RemoveAt(EarVertexIndex);
					OriginalVertIndices.RemoveAt(EarVertexIndex);

					bFoundEar = true;
					break;
				}
			}

			// If we couldn't find an 'ear' it indicates something is bad with this polygon - discard triangles and return.
			if (!bFoundEar)
			{
				UE_LOG(LogTemp, Warning, TEXT("Triangulation of poly failed."));
				TriangleIndices.Empty();
				return TriangleIndices;
			}
		}
	}

	return TriangleIndices;
}

bool  UEditManager::IsWithinBoxBounds(FVector origin, FVector bounds, AWall *CurrentWall)
{
	float objBounds = bounds.X;
	if (bounds.Y > objBounds)
	{
		objBounds = bounds.Y;
	}
	/*if (bounds.Z > objBounds)
	{
		objBounds = bounds.Z;
	}*/

	for (int i = 0; i < CurrentWall->WallBoxes.Num(); i++)
	{
		float Dist = FVector::Distance(CurrentWall->WallBoxes[i].t_LeftSideStart, CurrentWall->WallBoxes[i].b_LeftSideEnd);
		float first = FVector::Distance(origin, CurrentWall->WallBoxes[i].b_LeftSideStart);
		float second = FVector::Distance(origin, CurrentWall->WallBoxes[i].b_LeftSideEnd);
		float thirt = FVector::Distance(origin, CurrentWall->WallBoxes[i].t_LeftSideStart);
		float fourth = FVector::Distance(origin, CurrentWall->WallBoxes[i].t_LeftSideEnd);
		int distcount = 0;
		
		objBounds *= 1.1f;

		if (Dist > FVector::Distance(origin, CurrentWall->WallBoxes[i].b_LeftSideStart) + objBounds)
		{
			distcount++;
		}
		
		if (Dist > FVector::Distance(origin, CurrentWall->WallBoxes[i].b_LeftSideEnd) + objBounds)
		{
			distcount++;

		}
		
		if (Dist > FVector::Distance(origin, CurrentWall->WallBoxes[i].t_LeftSideStart) + objBounds)
		{
			distcount++;

		}
		
		if (Dist > FVector::Distance(origin, CurrentWall->WallBoxes[i].t_LeftSideEnd) + objBounds)
		{
			distcount++;

		}

		if (distcount == 4)
		{
			return true;
		}


	}


	return false;
}
/*
============================================================================================================================================
============================================================================================================================================
============================================================================================================================================
============================================================================================================================================
*/

void UEditManager::UpdateDimensionStringsForInteriorWalls()
{
	//remember, y goes left, x goes down, z goes up.
	FVector VerticalNorm(1, 0, 0);
	FVector HorizontalNorm(0, 1, 0);
	//lists for room nodes and walls interior and exterior.
	//all external have to be grounded.
	TArray<AWall*> InteriorHorzWalls;
	//TArray<AWall*> ExteriorHorzWalls;
	TArray<AWall*> InteriorVertWalls;
	//TArray<AWall*> ExteriorVertWalls;
	TArray<ARoomNode*> InteriorDiagonalWalls;
	//TArray<ARoomNode*> ExteriorDiagonalWalls;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;


	//erase all existing dim strings to spawn new ones.
	for (int i = 0; i < InteriorDimensionStrings.Num(); i++)
	{
		InteriorDimensionStrings.Pop(true)->Destroy();
	}

	//set up the 4 TArrays containing Interior and Exterior Horizontal and Vertical Walls
	for (AWall * Wall : Walls)
	{
		//reset visited
		Wall->bVisited = false;
		
		if (Wall->LeftRoom->IsInterior() && Wall->RightRoom->IsInterior())
		{
			//interior horizontal wall
			if (Wall->GetActorRightVector().GetAbs().Equals(HorizontalNorm, 0.0001f))
			{
				InteriorHorzWalls.Add(Wall);
				
			}
			//interior vertical wall
			else if (Wall->GetActorRightVector().GetAbs().Equals(VerticalNorm, 0.0001f))
			{
				InteriorVertWalls.Add(Wall);
			}
			//interior diagonal wall
			else
			{
				InteriorDiagonalWalls.Add(Wall->StartNode);
				InteriorDiagonalWalls.Add(Wall->EndNode);
			}

			Wall->WidthDimensionString->SetInvisible();
			Wall->HeightDimensionString->SetInvisible();

			Wall->LeftChainedWall = nullptr;
			Wall->RightChainedWall = nullptr;
			Wall->LeftChainedNode = nullptr;
			Wall->RightChainedNode = nullptr;
			Wall->bGrounded = false;

			Wall->StartNode->LeftChainedWall = nullptr;
			Wall->StartNode->RightChainedWall = nullptr;
			Wall->StartNode->LeftChainedNode = nullptr;
			Wall->StartNode->RightChainedNode = nullptr;
			Wall->StartNode->bGrounded = false;
			Wall->StartNode->AssociatedWall = Wall;

			Wall->EndNode->LeftChainedWall = nullptr;
			Wall->EndNode->RightChainedWall = nullptr;
			Wall->EndNode->LeftChainedNode = nullptr;
			Wall->EndNode->RightChainedNode = nullptr;
			Wall->EndNode->bGrounded = false;
			Wall->EndNode->AssociatedWall = Wall;
		}
		else
		{

			/*
			if (Wall->GetActorRightVector().GetAbs().Equals(HorizontalNorm, 0.0001f))
			{
				ExteriorHorzWalls.Add(Wall);

			}
			//exterior vertical wall
			else if (Wall->GetActorRightVector().GetAbs().Equals(VerticalNorm, 0.0001f))
			{
				ExteriorVertWalls.Add(Wall);
			}
			//exterior diagonal wall
			else
			{
				ExteriorDiagonalWalls.Add(Wall->StartNode);
				ExteriorDiagonalWalls.Add(Wall->EndNode);
			}
			*/
			//exterior wall
			Wall->WidthDimensionString->SetVisible();
			Wall->HeightDimensionString->SetVisible();

			Wall->LeftChainedWall = nullptr;
			Wall->RightChainedWall = nullptr;
			Wall->LeftChainedNode = nullptr;
			Wall->RightChainedNode = nullptr;
			Wall->bGrounded = true;

			Wall->StartNode->LeftChainedWall = nullptr;
			Wall->StartNode->RightChainedWall = nullptr;
			Wall->StartNode->LeftChainedNode = nullptr;
			Wall->StartNode->RightChainedNode = nullptr;
			Wall->StartNode->bGrounded = true;
			Wall->StartNode->AssociatedWall = Wall;

			Wall->EndNode->LeftChainedWall = nullptr;
			Wall->EndNode->RightChainedWall = nullptr;
			Wall->EndNode->LeftChainedNode = nullptr;
			Wall->EndNode->RightChainedNode = nullptr;
			Wall->EndNode->bGrounded = true;
			Wall->EndNode->AssociatedWall = Wall;
		}
	}
	/*
	Interior Horizontal Walls
	*/
	for (AWall * Wall : InteriorHorzWalls)
	{
		FVector MidPoint = Wall->StartPoint + 0.5f*(Wall->EndPoint - Wall->StartPoint);
		FVector WallNormal = Wall->GetActorRightVector();
		TArray<AWall*> WallsInLeftSide;
		TArray<AWall*> WallsInRightSide;
		TArray<ARoomNode*> NodesInLeftSide;
		TArray<ARoomNode*> NodesInRightSide;
		int WhichWasConnected = 0; //1 = left wall, 2 = right wall, 3 = left node, 4 = right node, 0 = nothing

		for (AWall * PotOppoWall : Wall->LeftRoom->RoomData.WallsOrdered)
		{
			if (PotOppoWall != Wall && Wall->LeftChainedWall != PotOppoWall && PotOppoWall->GetActorRightVector().GetAbs().Equals(WallNormal.GetAbs(), 0.0001f))
			{
				WallsInLeftSide.Add(PotOppoWall);
			}
		}

		for (ARoomNode * RoomNode : Wall->LeftRoom->RoomData.Nodes)
		{
			if (RoomNode->AssociatedWall != Wall && Wall->LeftChainedNode != RoomNode)
			{
				NodesInLeftSide.Add(RoomNode);
			}
		}

		for (AWall * PotOppoWall : Wall->RightRoom->RoomData.WallsOrdered)
		{
			if (PotOppoWall != Wall && Wall->RightChainedWall != PotOppoWall && PotOppoWall->GetActorRightVector().GetAbs().Equals(WallNormal.GetAbs(), 0.0001f))
			{
				WallsInRightSide.Add(PotOppoWall);
			}
		}

		for (ARoomNode * RoomNode : Wall->RightRoom->RoomData.Nodes)
		{
			if (RoomNode->AssociatedWall != Wall && Wall->RightChainedNode != RoomNode)
			{
				NodesInRightSide.Add(RoomNode);
			}
		}

		float LeftSideBest = INFINITY;
		AWall* LeftSide = nullptr;
		for (AWall * WallCheck : WallsInLeftSide)
		{
			FVector OtherWallHit = FVector::PointPlaneProject(MidPoint, WallCheck->StartPoint, WallNormal);
			float t;
			float LeftSideVal;

			if (FMath::IsNearlyEqual(WallNormal.X, 0.0f, 0.0001f))
			{
				t = (MidPoint.Y - OtherWallHit.Y);
				LeftSideVal = FMath::Abs(t*WallNormal.Y);
			}
			else
			{
				t = (MidPoint.X - OtherWallHit.X);
				LeftSideVal = FMath::Abs(t*WallNormal.X);
			}

			if (t > 0)
			{
				if (!LeftSide || (WallCheck->bGrounded && LeftSide->bGrounded && LeftSideVal < LeftSideBest)
					|| (WallCheck->bGrounded && !LeftSide->bGrounded) || (!WallCheck->bGrounded && !LeftSide->bGrounded && LeftSideVal < LeftSideBest))
				{
					LeftSide = WallCheck;
					LeftSideBest = LeftSideVal;
				}
			}
		}

		float LeftNodeBest = INFINITY;
		ARoomNode *  LeftNode = nullptr;
		for (ARoomNode * NodeCheck : NodesInLeftSide)
		{
			FVector NodeProjectToThisWall = FVector::PointPlaneProject(MidPoint, NodeCheck->GetActorLocation(), WallNormal);
			float t;
			float LeftNodeVal;

			if (FMath::IsNearlyEqual(NodeProjectToThisWall.X, MidPoint.X, 0.0001f))
			{
				t = (MidPoint.Y - NodeProjectToThisWall.Y);
				LeftNodeVal = FMath::Abs(t*WallNormal.Y);
			}
			else
			{
				t = (MidPoint.X - NodeProjectToThisWall.X);
				LeftNodeVal = FMath::Abs(t*WallNormal.X);
			}

			if (t > 0)
			{
				if (!LeftNode || (NodeCheck->bGrounded && LeftNode->bGrounded && LeftNodeVal < LeftNodeBest)
					|| (NodeCheck->bGrounded && !LeftNode->bGrounded) || (!NodeCheck->bGrounded && !LeftNode->bGrounded && LeftNodeVal < LeftNodeBest))
				{
					LeftNode = NodeCheck;
					LeftNodeBest = LeftNodeVal;
				}
			}
		}

		float RightSideBest = INFINITY;
		AWall* RightSide = nullptr;
		for (AWall * WallCheck : WallsInRightSide)
		{
			FVector OtherWallHit = FVector::PointPlaneProject(MidPoint, WallCheck->StartPoint, -1 * WallNormal);
			float t;
			float RightSideVal;

			if (FMath::IsNearlyEqual(WallNormal.X, 0.0f, 0.0001f) == 0)
			{
				t = -1*(MidPoint.Y - OtherWallHit.Y);
				RightSideVal = FMath::Abs(t*WallNormal.Y);
			}
			else
			{
				t = -1*(MidPoint.X - OtherWallHit.X);
				RightSideVal = FMath::Abs(t*WallNormal.X);
			}

			if (t > 0)
			{
				if (!RightSide || (WallCheck->bGrounded && RightSide->bGrounded && RightSideVal < RightSideBest)
					|| (WallCheck->bGrounded && !RightSide->bGrounded) || (!WallCheck->bGrounded && !RightSide->bGrounded && RightSideVal < RightSideBest))
				{
					RightSide = WallCheck;
					RightSideBest = RightSideVal;
				}
			}
		}

		float RightNodeBest = INFINITY;
		ARoomNode *  RightNode = nullptr;
		for (ARoomNode * NodeCheck : NodesInRightSide)
		{
			float RightNodeVal;
			FVector NodeProjectToThisWall = FVector::PointPlaneProject(MidPoint, NodeCheck->GetActorLocation(), -1*WallNormal);
			float t;

			if (FMath::IsNearlyEqual(NodeProjectToThisWall.X, MidPoint.X, 0.0001f))
			{
				t = -1 * (NodeProjectToThisWall.Y - MidPoint.Y);
				RightNodeVal = FMath::Abs(t*WallNormal.Y);
			}
			else
			{
				t = -1 * (NodeProjectToThisWall.X - MidPoint.X);
				RightNodeVal = FMath::Abs(t*WallNormal.X);
			}

			if (t > 0)
			{
				if (!RightNode || (NodeCheck->bGrounded && RightNode->bGrounded && RightNodeVal < RightNodeBest)
					|| (NodeCheck->bGrounded && !RightNode->bGrounded) || (!NodeCheck->bGrounded && !RightNode->bGrounded && RightNodeVal < RightNodeBest))
				{
					RightNode = NodeCheck;
					RightNodeBest = RightNodeVal;
				}
			}
		}



		if (LeftSide == nullptr && RightSide == nullptr) // no side connections exist.
		{
			if (LeftNode == nullptr && RightNode == nullptr)
			{
				// do nothing, no connections exist.
			}
			else if (LeftNode == nullptr || RightNode == nullptr)
			{
				if (LeftNode != nullptr)
				{
					//choose left node bc it is the only connection that exists.
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					if (MidPoint.Y < LeftNode->GetActorLocation().Y) // node is higher than wall 
					{
						if (LeftNode->GetActorLocation().X <= MidPoint.X) // node is right of wall midpoint
						{
							DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
								LeftNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //node is left of wall midpoint
						{
							DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
								LeftNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
					}
					else // node is lower than wall
					{
						if (LeftNode->GetActorLocation().X <= MidPoint.X) // node is right of wall midpoint
						{

							DMInput->SetDimensionString(MidPoint + FVector(1, 0, 1), LeftNode->GetActorLocation() + FVector(1, 0, 1),
								FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), LeftNode->GetActorLocation() + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //node is left of wall midpoint
						{
							DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), LeftNode->GetActorLocation() + FVector(-1, 0, 1),
								FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), LeftNode->GetActorLocation() + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
					}
					WhichWasConnected = 3;
					InteriorDimensionStrings.Push(DMInput);
				}
				else
				{
					//choose right node bc it is the only connection to exist.
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					if (MidPoint.Y < RightNode->GetActorLocation().Y)  // node is higher than wall 
					{
						if (RightNode->GetActorLocation().X <= MidPoint.X) // node is right of wall midpoint
						{
							DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
								RightNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //node is left of wall midpoint
						{
							DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
								RightNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					else // node is lower than wall
					{
						if (RightNode->GetActorLocation().X <= MidPoint.X) // node is right of wall midpoint
						{

							DMInput->SetDimensionString(MidPoint + FVector(1, 0, 1), RightNode->GetActorLocation() + FVector(1, 0, 1),
								FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), RightNode->GetActorLocation() + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //node is left of wall midpoint
						{
							DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), RightNode->GetActorLocation() + FVector(-1, 0, 1),
								FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), RightNode->GetActorLocation() + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
					}
					WhichWasConnected = 4;
					InteriorDimensionStrings.Push(DMInput);
				}
			}
			else //Left and Right Node only exist
			{
				//grounded check for existing connections (only nodes)
				if (LeftNode->bGrounded && RightNode->bGrounded)
				{
					//if both are grounded, choose closest one.
					if (LeftNodeBest <= RightNodeBest)
					{
						//choose left node, closest grounded node
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (MidPoint.Y < LeftNode->GetActorLocation().Y) // node is higher than wall 
						{
							if (LeftNode->GetActorLocation().X <= MidPoint.X) // node is right of wall midpoint
							{
								DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
									LeftNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is left of wall midpoint
							{
								DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
									LeftNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}

						}
						else // node is lower than wall
						{
							if (LeftNode->GetActorLocation().X <= MidPoint.X) // node is right of wall midpoint
							{

								DMInput->SetDimensionString(MidPoint + FVector(1, 0, 1), LeftNode->GetActorLocation() + FVector(1, 0, 1),
									FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), LeftNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is left of wall midpoint
							{
								DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), LeftNode->GetActorLocation() + FVector(-1, 0, 1),
									FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), LeftNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						WhichWasConnected = 3;
						InteriorDimensionStrings.Push(DMInput);
					}
					else
					{
						//choose right node, closest grounded node
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (MidPoint.Y < RightNode->GetActorLocation().Y)  // node is higher than wall 
						{
							if (RightNode->GetActorLocation().X <= MidPoint.X) // node is right of wall midpoint
							{
								DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
									RightNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is left of wall midpoint
							{
								DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
									RightNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}

						}
						else  // node is lower than wall 
						{
							if (RightNode->GetActorLocation().X <= MidPoint.X) // node is right of wall midpoint
							{

								DMInput->SetDimensionString(MidPoint + FVector(1, 0, 1), RightNode->GetActorLocation() + FVector(1, 0, 1),
									FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), RightNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is left of wall midpoint
							{
								DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), RightNode->GetActorLocation() + FVector(-1, 0, 1),
									FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), RightNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						WhichWasConnected = 4;
						InteriorDimensionStrings.Push(DMInput);
					}
				}
				else if (LeftNode->bGrounded || RightNode->bGrounded)
				{
					if (LeftNode->bGrounded)
					{
						//choose left node, only node that is grounded
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (MidPoint.Y < LeftNode->GetActorLocation().Y) // node is higher than wall 
						{
							if (LeftNode->GetActorLocation().X <= MidPoint.X) // node is right of wall midpoint
							{
								DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
									LeftNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is left of wall midpoint
							{
								DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
									LeftNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}

						}
						else // node is lower than wall
						{
							if (LeftNode->GetActorLocation().X <= MidPoint.X) // node is right of wall midpoint
							{

								DMInput->SetDimensionString(MidPoint + FVector(1, 0, 1), LeftNode->GetActorLocation() + FVector(1, 0, 1),
									FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), LeftNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is left of wall midpoint
							{
								DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), LeftNode->GetActorLocation() + FVector(-1, 0, 1),
									FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), LeftNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						WhichWasConnected = 3;
						InteriorDimensionStrings.Push(DMInput);
					}
					else
					{
						//choose right node, only node that is grounded
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (MidPoint.Y < RightNode->GetActorLocation().Y)  // node is higher than wall 
						{
							if (RightNode->GetActorLocation().X <= MidPoint.X) // node is right of wall midpoint
							{
								DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
									RightNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is left of wall midpoint
							{
								DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
									RightNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}

						}
						else  // node is lower than wall 
						{
							if (RightNode->GetActorLocation().X <= MidPoint.X) // node is right of wall midpoint
							{

								DMInput->SetDimensionString(MidPoint + FVector(1, 0, 1), RightNode->GetActorLocation() + FVector(1, 0, 1),
									FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), RightNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is left of wall midpoint
							{
								DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), RightNode->GetActorLocation() + FVector(-1, 0, 1),
									FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), RightNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						WhichWasConnected = 4;
						InteriorDimensionStrings.Push(DMInput);
					}
				}
				else
				{
					if (LeftNodeBest <= RightNodeBest)
					{
						//choose left node, closest ungrounded node.
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (MidPoint.Y < LeftNode->GetActorLocation().Y) // node is higher than wall 
						{
							if (LeftNode->GetActorLocation().X <= MidPoint.X) // node is right of wall midpoint
							{
								DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
									LeftNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is left of wall midpoint
							{
								DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
									LeftNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}

						}
						else // node is lower than wall
						{
							if (LeftNode->GetActorLocation().X <= MidPoint.X) // node is right of wall midpoint
							{

								DMInput->SetDimensionString(MidPoint + FVector(1, 0, 1), LeftNode->GetActorLocation() + FVector(1, 0, 1),
									FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), LeftNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is left of wall midpoint
							{
								DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), LeftNode->GetActorLocation() + FVector(-1, 0, 1),
									FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), LeftNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						WhichWasConnected = 3;
						InteriorDimensionStrings.Push(DMInput);
					}
					else
					{
						//choose right node, closest ungrounded node.
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (MidPoint.Y < RightNode->GetActorLocation().Y)  // node is higher than wall 
						{
							if (RightNode->GetActorLocation().X <= MidPoint.X) // node is right of wall midpoint
							{
								DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
									RightNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is left of wall midpoint
							{
								DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
									RightNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}

						}
						else  // node is lower than wall 
						{
							if (RightNode->GetActorLocation().X <= MidPoint.X) // node is right of wall midpoint
							{

								DMInput->SetDimensionString(MidPoint + FVector(1, 0, 1), RightNode->GetActorLocation() + FVector(1, 0, 1),
									FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), RightNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is left of wall midpoint
							{
								DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), RightNode->GetActorLocation() + FVector(-1, 0, 1),
									FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), RightNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						WhichWasConnected = 4;
						InteriorDimensionStrings.Push(DMInput);
					}
				}
			}
		}
		else if (LeftSide == nullptr || RightSide == nullptr)

		{
			if (LeftSide != nullptr)
			{
				//choose left side because it's better to choose a side over room nodes.
				ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
				FVector LeftMidPoint = LeftSide->StartPoint + 0.5f*(LeftSide->EndPoint - LeftSide->StartPoint);
				float LeftSize = FVector::Distance(LeftSide->EndPoint, LeftSide->StartPoint);
				float WallSize = FVector::Distance(Wall->EndPoint, Wall->StartPoint);
				if (MidPoint.Y < LeftMidPoint.Y) // left wall is BELOW this wall.
				{
					if (LeftSize >= WallSize) // left wall is bigger than this wall.
					{
						DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(-1, 0, 1),
							MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}
					else //left wall is smaller than this wall.
					{
						DMInput->SetDimensionString(FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(-1, 0, 1), LeftMidPoint + FVector(-1, 0, 1),
							FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1), LeftMidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}

				}
				else // node is ABOVE this wall
				{
					if (LeftSize >= WallSize) // left wall is bigger than this wall.
					{
						DMInput->SetDimensionString(FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
							FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}
					else //left wall is smaller than this wall.
					{
						DMInput->SetDimensionString(LeftMidPoint + FVector(-1, 0, 1), FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(-1, 0, 1),
							LeftMidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}

				}
				WhichWasConnected = 1;
				InteriorDimensionStrings.Push(DMInput);
			}
			else
			{
				//choose right side because it's better to choose a side over roomnodes.
				ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
				FVector RightMidPoint = RightSide->StartPoint + 0.5f*(RightSide->EndPoint - RightSide->StartPoint);
				float RightSize = FVector::Distance(RightSide->EndPoint, RightSide->StartPoint);
				float WallSize = FVector::Distance(Wall->EndPoint, Wall->StartPoint);
				if (MidPoint.Y < RightMidPoint.Y) // right wall is ABOVE this wall.
				{
					if (RightSize >= WallSize) // Right wall is bigger than this wall.
					{
						DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(-1, 0, 1),
							MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}
					else //Right wall is smaller than this wall.
					{
						DMInput->SetDimensionString(FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(-1, 0, 1), RightMidPoint + FVector(-1, 0, 1),
							FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1), RightMidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}

				}
				else //right wall is BELOW this wall
				{
					if (RightSize >= WallSize) // Right wall is bigger than this wall.
					{
						DMInput->SetDimensionString(FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
							FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}
					else //Right wall is smaller than this wall.
					{
						DMInput->SetDimensionString(RightMidPoint + FVector(-1, 0, 1), FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(-1, 0, 1),
							RightMidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}

				}
				WhichWasConnected = 2;
				InteriorDimensionStrings.Push(DMInput);
			}
		}
		else //both sides exist, so we ignore room nodes.
		{
			//grounded check for existing connections (only walls)
			if (LeftSide->bGrounded && RightSide->bGrounded)
			{
				//if both are grounded, choose closest one.
				if (LeftSideBest <= RightSideBest)
				{
					//choose left wall, closest grounded wall
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector LeftMidPoint = LeftSide->StartPoint + 0.5f*(LeftSide->EndPoint - LeftSide->StartPoint);
					float LeftSize = FVector::Distance(LeftSide->EndPoint, LeftSide->StartPoint);
					float WallSize = FVector::Distance(Wall->EndPoint, Wall->StartPoint);
					if (MidPoint.Y < LeftMidPoint.Y) // left wall is BELOW this wall.
					{
						if (LeftSize >= WallSize) // left wall is bigger than this wall.
						{
							DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(-1, 0, 1),
								MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //left wall is smaller than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(-1, 0, 1), LeftMidPoint + FVector(-1, 0, 1),
								FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1), LeftMidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					else // node is ABOVE this wall
					{
						if (LeftSize >= WallSize) // left wall is bigger than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
								FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //left wall is smaller than this wall.
						{
							DMInput->SetDimensionString(LeftMidPoint + FVector(-1, 0, 1), FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(-1, 0, 1),
								LeftMidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					WhichWasConnected = 1;
					InteriorDimensionStrings.Push(DMInput);
				}
				else
				{
					//choose right wall, closest grounded wall
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector RightMidPoint = RightSide->StartPoint + 0.5f*(RightSide->EndPoint - RightSide->StartPoint);
					float RightSize = FVector::Distance(RightSide->EndPoint, RightSide->StartPoint);
					float WallSize = FVector::Distance(Wall->EndPoint, Wall->StartPoint);
					if (MidPoint.Y < RightMidPoint.Y) // right wall is ABOVE this wall.
					{
						if (RightSize >= WallSize) // Right wall is bigger than this wall.
						{
							DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(-1, 0, 1),
								MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //Right wall is smaller than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(-1, 0, 1), RightMidPoint + FVector(-1, 0, 1),
								FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1), RightMidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					else //right wall is BELOW this wall
					{
						if (RightSize >= WallSize) // Right wall is bigger than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
								FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //Right wall is smaller than this wall.
						{
							DMInput->SetDimensionString(RightMidPoint + FVector(-1, 0, 1), FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(-1, 0, 1),
								RightMidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					WhichWasConnected = 2;
					InteriorDimensionStrings.Push(DMInput);
				}
			}
			else if (LeftSide->bGrounded || RightSide->bGrounded)
			{
				if (LeftSide->bGrounded)
				{
					//choose left wall, only wall that is grounded
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector LeftMidPoint = LeftSide->StartPoint + 0.5f*(LeftSide->EndPoint - LeftSide->StartPoint);
					float LeftSize = FVector::Distance(LeftSide->EndPoint, LeftSide->StartPoint);
					float WallSize = FVector::Distance(Wall->EndPoint, Wall->StartPoint);
					if (MidPoint.Y < LeftMidPoint.Y) // left wall is BELOW this wall.
					{
						if (LeftSize >= WallSize) // left wall is bigger than this wall.
						{
							DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(-1, 0, 1),
								MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //left wall is smaller than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(-1, 0, 1), LeftMidPoint + FVector(-1, 0, 1),
								FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1), LeftMidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					else // node is ABOVE this wall
					{
						if (LeftSize >= WallSize) // left wall is bigger than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
								FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //left wall is smaller than this wall.
						{
							DMInput->SetDimensionString(LeftMidPoint + FVector(-1, 0, 1), FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(-1, 0, 1),
								LeftMidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					WhichWasConnected = 1;
					InteriorDimensionStrings.Push(DMInput);
				}
				else
				{
					//choose right wall, only wall that is grounded
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector RightMidPoint = RightSide->StartPoint + 0.5f*(RightSide->EndPoint - RightSide->StartPoint);
					float RightSize = FVector::Distance(RightSide->EndPoint, RightSide->StartPoint);
					float WallSize = FVector::Distance(Wall->EndPoint, Wall->StartPoint);
					if (MidPoint.Y < RightMidPoint.Y) // right wall is ABOVE this wall.
					{
						if (RightSize >= WallSize) // Right wall is bigger than this wall.
						{
							DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(-1, 0, 1),
								MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //Right wall is smaller than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(-1, 0, 1), RightMidPoint + FVector(-1, 0, 1),
								FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1), RightMidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					else //right wall is BELOW this wall
					{
						if (RightSize >= WallSize) // Right wall is bigger than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
								FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //Right wall is smaller than this wall.
						{
							DMInput->SetDimensionString(RightMidPoint + FVector(-1, 0, 1), FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(-1, 0, 1),
								RightMidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					WhichWasConnected = 2;
					InteriorDimensionStrings.Push(DMInput);
				}
			}
			else
			{
				if (LeftSideBest <= RightSideBest)
				{
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector LeftMidPoint = LeftSide->StartPoint + 0.5f*(LeftSide->EndPoint - LeftSide->StartPoint);
					float LeftSize = FVector::Distance(LeftSide->EndPoint, LeftSide->StartPoint);
					float WallSize = FVector::Distance(Wall->EndPoint, Wall->StartPoint);
					if (MidPoint.Y < LeftMidPoint.Y) // left wall is BELOW this wall.
					{
						if (LeftSize >= WallSize) // left wall is bigger than this wall.
						{
							DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(-1, 0, 1),
								MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //left wall is smaller than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(-1, 0, 1), LeftMidPoint + FVector(-1, 0, 1),
								FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1), LeftMidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					else // node is ABOVE this wall
					{
						if (LeftSize >= WallSize) // left wall is bigger than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
								FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //left wall is smaller than this wall.
						{
							DMInput->SetDimensionString(LeftMidPoint + FVector(-1, 0, 1), FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(-1, 0, 1),
								LeftMidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					WhichWasConnected = 1;
					InteriorDimensionStrings.Push(DMInput);
				}
				else
				{
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector RightMidPoint = RightSide->StartPoint + 0.5f*(RightSide->EndPoint - RightSide->StartPoint);
					float RightSize = FVector::Distance(RightSide->EndPoint, RightSide->StartPoint);
					float WallSize = FVector::Distance(Wall->EndPoint, Wall->StartPoint);
					if (MidPoint.Y < RightMidPoint.Y) // right wall is ABOVE this wall.
					{
						if (RightSize >= WallSize) // Right wall is bigger than this wall.
						{
							DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(-1, 0, 1),
								MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //Right wall is smaller than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(-1, 0, 1), RightMidPoint + FVector(-1, 0, 1),
								FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1), RightMidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					else //right wall is BELOW this wall
					{
						if (RightSize >= WallSize) // Right wall is bigger than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
								FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //Right wall is smaller than this wall.
						{
							DMInput->SetDimensionString(RightMidPoint + FVector(-1, 0, 1), FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(-1, 0, 1),
								RightMidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					WhichWasConnected = 2;
					InteriorDimensionStrings.Push(DMInput);
				}
			}
		}

		//chain walls and room nodes here.
		//if a wall is grounded so is its roomnodes.
		FoundGrounded = false;
		if (WhichWasConnected == 1)
		{
			Wall->LeftChainedWall = LeftSide;
			LeftSide->RightChainedWall = Wall;
			 UpdateGrounded(Wall);
		}
		else if (WhichWasConnected == 2)
		{
			Wall->RightChainedWall = RightSide;
			RightSide->LeftChainedWall = Wall;
			 UpdateGrounded(Wall);
		}
		else if (WhichWasConnected == 3)
		{
			Wall->LeftChainedNode = LeftNode;
			LeftNode->RightChainedWall = Wall;
			 UpdateGrounded(Wall);
		}
		else if (WhichWasConnected == 4)
		{
			Wall->RightChainedNode = RightNode;
			RightNode->LeftChainedWall = Wall;
			 UpdateGrounded(Wall);
		}
	}
	/*
	Interior Vertical Walls
	*/
	//set up the 4 TArrays containing Interior and Exterior Horizontal and Vertical Walls
	for (AWall * Wall : Walls)
	{
		//reset visited
		Wall->bVisited = false;
	}
	for (AWall * Wall : InteriorVertWalls)
	{
		FVector MidPoint = Wall->StartPoint + 0.5f*(Wall->EndPoint - Wall->StartPoint);
		FVector WallNormal = Wall->GetActorRightVector();
		TArray<AWall*> WallsInLeftSide;
		TArray<AWall*> WallsInRightSide;
		TArray<ARoomNode*> NodesInLeftSide;
		TArray<ARoomNode*> NodesInRightSide;
		int WhichWasConnected = 0; //1 = left wall, 2 = right wall, 3 = left node, 4 = right node, 0 = nothing

		for (AWall * PotOppoWall : Wall->LeftRoom->RoomData.WallsOrdered)
		{
			if (PotOppoWall != Wall && Wall->LeftChainedWall != PotOppoWall && PotOppoWall->GetActorRightVector().GetAbs().Equals(Wall->GetActorRightVector().GetAbs(), 0.0001f))
			{
				WallsInLeftSide.Add(PotOppoWall);
			}
		}

		for (ARoomNode * RoomNode : Wall->LeftRoom->RoomData.Nodes)
		{
			if (RoomNode->AssociatedWall != Wall && Wall->LeftChainedNode != RoomNode)
			{
				NodesInLeftSide.Add(RoomNode);
			}
		}

		for (AWall * PotOppoWall : Wall->RightRoom->RoomData.WallsOrdered)
		{
			if (PotOppoWall != Wall && Wall->RightChainedWall != PotOppoWall && PotOppoWall->GetActorRightVector().GetAbs().Equals(Wall->GetActorRightVector().GetAbs(), 0.0001f))
			{
				WallsInRightSide.Add(PotOppoWall);
			}
		}

		for (ARoomNode * RoomNode : Wall->RightRoom->RoomData.Nodes)
		{
			if (RoomNode->AssociatedWall != Wall && Wall->RightChainedNode != RoomNode)
			{
				NodesInRightSide.Add(RoomNode);
			}
		}

		float LeftSideBest = INFINITY;
		AWall* LeftSide = nullptr;
		for (AWall * WallCheck : WallsInLeftSide)
		{
			FVector OtherWallHit = FVector::PointPlaneProject(MidPoint, WallCheck->StartPoint, WallNormal);
			float t;
			float LeftSideVal;

			if (FMath::IsNearlyEqual(WallNormal.X, 0.0f, 0.0001f))
			{
				t = (MidPoint.Y - OtherWallHit.Y);
				LeftSideVal = FMath::Abs(t*WallNormal.Y);
			}
			else
			{
				t = (MidPoint.X - OtherWallHit.X);
				LeftSideVal = FMath::Abs(t*WallNormal.X);
			}

			if (t > 0)
			{
				if (!LeftSide || (WallCheck->bGrounded && LeftSide->bGrounded && LeftSideVal < LeftSideBest)
					|| (WallCheck->bGrounded && !LeftSide->bGrounded) || (!WallCheck->bGrounded && !LeftSide->bGrounded && LeftSideVal < LeftSideBest))
				{
					LeftSide = WallCheck;
					LeftSideBest = LeftSideVal;
				}
			}
		}

		float LeftNodeBest = INFINITY;
		ARoomNode *  LeftNode = nullptr;
		for (ARoomNode * NodeCheck : NodesInLeftSide)
		{
			FVector NodeProjectToThisWall = FVector::PointPlaneProject(MidPoint, NodeCheck->GetActorLocation(), WallNormal);
			float t;
			float LeftNodeVal;

			if (FMath::IsNearlyEqual(NodeProjectToThisWall.X, MidPoint.X, 0.0001f))
			{
				t = (MidPoint.Y - NodeProjectToThisWall.Y);
				LeftNodeVal = FMath::Abs(t*WallNormal.Y);
			}
			else
			{
				t = (MidPoint.X - NodeProjectToThisWall.X);
				LeftNodeVal = FMath::Abs(t*WallNormal.X);
			}

			if (t > 0)
			{
				if (!LeftNode || (NodeCheck->bGrounded && LeftNode->bGrounded && LeftNodeVal < LeftNodeBest)
					|| (NodeCheck->bGrounded && !LeftNode->bGrounded) || (!NodeCheck->bGrounded && !LeftNode->bGrounded && LeftNodeVal < LeftNodeBest))
				{
					LeftNode = NodeCheck;
					LeftNodeBest = LeftNodeVal;
				}
			}
		}

		float RightSideBest = INFINITY;
		AWall* RightSide = nullptr;
		for (AWall * WallCheck : WallsInRightSide)
		{
			FVector OtherWallHit = FVector::PointPlaneProject(MidPoint, WallCheck->StartPoint, -1 * WallNormal);
			float t;
			float RightSideVal;

			if (FMath::IsNearlyEqual(WallNormal.X, 0.0f, 0.0001f) == 0)
			{
				t = -1 * (MidPoint.Y - OtherWallHit.Y);
				RightSideVal = FMath::Abs(t*WallNormal.Y);
			}
			else
			{
				t = -1 * (MidPoint.X - OtherWallHit.X);
				RightSideVal = FMath::Abs(t*WallNormal.X);
			}

			if (t > 0)
			{
				if (!RightSide || (WallCheck->bGrounded && RightSide->bGrounded && RightSideVal < RightSideBest)
					|| (WallCheck->bGrounded && !RightSide->bGrounded) || (!WallCheck->bGrounded && !RightSide->bGrounded && RightSideVal < RightSideBest))
				{
					RightSide = WallCheck;
					RightSideBest = RightSideVal;
				}
			}
		}

		float RightNodeBest = INFINITY;
		ARoomNode *  RightNode = nullptr;
		for (ARoomNode * NodeCheck : NodesInRightSide)
		{
			float RightNodeVal;
			FVector NodeProjectToThisWall = FVector::PointPlaneProject(MidPoint, NodeCheck->GetActorLocation(), -1 * WallNormal);
			float t;

			if (FMath::IsNearlyEqual(NodeProjectToThisWall.X, MidPoint.X, 0.0001f))
			{
				t = -1 * (NodeProjectToThisWall.Y - MidPoint.Y);
				RightNodeVal = FMath::Abs(t*WallNormal.Y);
			}
			else
			{
				t = -1 * (NodeProjectToThisWall.X - MidPoint.X);
				RightNodeVal = FMath::Abs(t*WallNormal.X);
			}

			if (t > 0)
			{
				if (!RightNode || (NodeCheck->bGrounded && RightNode->bGrounded && RightNodeVal < RightNodeBest)
					|| (NodeCheck->bGrounded && !RightNode->bGrounded) || (!NodeCheck->bGrounded && !RightNode->bGrounded && RightNodeVal < RightNodeBest))
				{
					RightNode = NodeCheck;
					RightNodeBest = RightNodeVal;
				}
			}
		}



		if (LeftSide == nullptr && RightSide == nullptr) // no side connections exist.
		{
			if (LeftNode == nullptr && RightNode == nullptr)
			{
				// do nothing, no connections exist.
			}
			else if (LeftNode == nullptr || RightNode == nullptr)
			{
				if (LeftNode != nullptr)
				{
					//choose left node bc it is the only connection that exists.
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					if (MidPoint.X < LeftNode->GetActorLocation().X) // node is left of wall 
					{
						if (LeftNode->GetActorLocation().Y <= MidPoint.Y) // node is above the midpoint of wall
						{
							DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(0 , 1, 1), MidPoint + FVector(0, 1, 1),
								LeftNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //node is below the wall midpoint
						{
							DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(0, -1, 1), MidPoint + FVector(0, -1, 1),
								LeftNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
					}
					else // node is right of wall
					{
						if (LeftNode->GetActorLocation().X <= MidPoint.X) // node is above the midpoint of wall
						{

							DMInput->SetDimensionString( MidPoint + FVector(0, 1, 1), LeftNode->GetActorLocation() + FVector(0, 1, 1),
								FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), LeftNode->GetActorLocation() + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else  //node is below the wall midpoint
						{
							DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), LeftNode->GetActorLocation() + FVector(0, 1, 1),
								FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), LeftNode->GetActorLocation() + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
					}
					WhichWasConnected = 3;
					InteriorDimensionStrings.Push(DMInput);
				}
				else
				{
					//choose right node bc it is the only connection to exist.
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					if (MidPoint.X < RightNode->GetActorLocation().X) // node is left of wall 
					{
						if (RightNode->GetActorLocation().Y <= MidPoint.Y) // node is above the midpoint of wall
						{
							DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
								RightNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //node is below the wall midpoint
						{
							DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(0, -1, 1), MidPoint + FVector(0, -1, 1),
								RightNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
					}
					else // node is right of wall
					{
						if (RightNode->GetActorLocation().X <= MidPoint.X) // node is above the midpoint of wall
						{

							DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), RightNode->GetActorLocation() + FVector(0, 1, 1),
								FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), RightNode->GetActorLocation() + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else  //node is below the wall midpoint
						{
							DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), RightNode->GetActorLocation() + FVector(0, 1, 1),
								FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), RightNode->GetActorLocation() + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
					}
					WhichWasConnected = 4;
					InteriorDimensionStrings.Push(DMInput);
				}
			}
			else //Left and Right Node only exist
			{
				//grounded check for existing connections (only nodes)
				if (LeftNode->bGrounded && RightNode->bGrounded)
				{
					//if both are grounded, choose closest one.
					if (LeftNodeBest <= RightNodeBest)
					{
						//choose left node, closest grounded node
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (MidPoint.X < LeftNode->GetActorLocation().X) // node is left of wall 
						{
							if (LeftNode->GetActorLocation().Y <= MidPoint.Y) // node is above the midpoint of wall
							{
								DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
									LeftNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is below the wall midpoint
							{
								DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(0, -1, 1), MidPoint + FVector(0, -1, 1),
									LeftNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						else // node is right of wall
						{
							if (LeftNode->GetActorLocation().X <= MidPoint.X) // node is above the midpoint of wall
							{

								DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), LeftNode->GetActorLocation() + FVector(0, 1, 1),
									FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), LeftNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else  //node is below the wall midpoint
							{
								DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), LeftNode->GetActorLocation() + FVector(0, 1, 1),
									FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), LeftNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						WhichWasConnected = 3;
						InteriorDimensionStrings.Push(DMInput);
					}
					else
					{
						//choose right node, closest grounded node
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (MidPoint.X < RightNode->GetActorLocation().X) // node is left of wall 
						{
							if (RightNode->GetActorLocation().Y <= MidPoint.Y) // node is above the midpoint of wall
							{
								DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
									RightNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is below the wall midpoint
							{
								DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(0, -1, 1), MidPoint + FVector(0, -1, 1),
									RightNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						else // node is right of wall
						{
							if (RightNode->GetActorLocation().X <= MidPoint.X) // node is above the midpoint of wall
							{

								DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), RightNode->GetActorLocation() + FVector(0, 1, 1),
									FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), RightNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else  //node is below the wall midpoint
							{
								DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), RightNode->GetActorLocation() + FVector(0, 1, 1),
									FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), RightNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						WhichWasConnected = 4;
						InteriorDimensionStrings.Push(DMInput);
					}
				}
				else if (LeftNode->bGrounded || RightNode->bGrounded)
				{
					if (LeftNode->bGrounded)
					{
						//choose left node, only node that is grounded
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (MidPoint.X < LeftNode->GetActorLocation().X) // node is left of wall 
						{
							if (LeftNode->GetActorLocation().Y <= MidPoint.Y) // node is above the midpoint of wall
							{
								DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
									LeftNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is below the wall midpoint
							{
								DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(0, -1, 1), MidPoint + FVector(0, -1, 1),
									LeftNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						else // node is right of wall
						{
							if (LeftNode->GetActorLocation().X <= MidPoint.X) // node is above the midpoint of wall
							{

								DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), LeftNode->GetActorLocation() + FVector(0, 1, 1),
									FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), LeftNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else  //node is below the wall midpoint
							{
								DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), LeftNode->GetActorLocation() + FVector(0, 1, 1),
									FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), LeftNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						WhichWasConnected = 3;
						InteriorDimensionStrings.Push(DMInput);
					}
					else
					{
						//choose right node, only node that is grounded
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (MidPoint.X < RightNode->GetActorLocation().X) // node is left of wall 
						{
							if (RightNode->GetActorLocation().Y <= MidPoint.Y) // node is above the midpoint of wall
							{
								DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
									RightNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is below the wall midpoint
							{
								DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(0, -1, 1), MidPoint + FVector(0, -1, 1),
									RightNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						else // node is right of wall
						{
							if (RightNode->GetActorLocation().X <= MidPoint.X) // node is above the midpoint of wall
							{

								DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), RightNode->GetActorLocation() + FVector(0, 1, 1),
									FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), RightNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else  //node is below the wall midpoint
							{
								DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), RightNode->GetActorLocation() + FVector(0, 1, 1),
									FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), RightNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						WhichWasConnected = 4;
						InteriorDimensionStrings.Push(DMInput);
					}
				}
				else
				{
					if (LeftNodeBest <= RightNodeBest)
					{
						//choose left node, closest ungrounded node.
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (MidPoint.X < LeftNode->GetActorLocation().X) // node is left of wall 
						{
							if (LeftNode->GetActorLocation().Y <= MidPoint.Y) // node is above the midpoint of wall
							{
								DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
									LeftNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is below the wall midpoint
							{
								DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(0, -1, 1), MidPoint + FVector(0, -1, 1),
									LeftNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						else // node is right of wall
						{
							if (LeftNode->GetActorLocation().X <= MidPoint.X) // node is above the midpoint of wall
							{

								DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), LeftNode->GetActorLocation() + FVector(0, 1, 1),
									FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), LeftNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else  //node is below the wall midpoint
							{
								DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), LeftNode->GetActorLocation() + FVector(0, 1, 1),
									FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), LeftNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						WhichWasConnected = 3;
						InteriorDimensionStrings.Push(DMInput);
					}
					else
					{
						//choose right node, closest ungrounded node.
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (MidPoint.X < RightNode->GetActorLocation().X) // node is left of wall 
						{
							if (RightNode->GetActorLocation().Y <= MidPoint.Y) // node is above the midpoint of wall
							{
								DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
									RightNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else //node is below the wall midpoint
							{
								DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(0, -1, 1), MidPoint + FVector(0, -1, 1),
									RightNode->GetActorLocation() + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						else // node is right of wall
						{
							if (RightNode->GetActorLocation().X <= MidPoint.X) // node is above the midpoint of wall
							{

								DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), RightNode->GetActorLocation() + FVector(0, 1, 1),
									FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), RightNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else  //node is below the wall midpoint
							{
								DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), RightNode->GetActorLocation() + FVector(0, 1, 1),
									FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, WallNormal) + FVector(0, 0, 1), RightNode->GetActorLocation() + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						WhichWasConnected = 4;
						InteriorDimensionStrings.Push(DMInput);
					}
				}
			}
		}
		else if (LeftSide == nullptr || RightSide == nullptr)

		{
			if (LeftSide != nullptr)
			{
				//choose left side because it's better to choose a side over room nodes.
				ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
				FVector LeftMidPoint = LeftSide->StartPoint + 0.5f*(LeftSide->EndPoint - LeftSide->StartPoint);
				float LeftSize = FVector::Distance(LeftSide->EndPoint, LeftSide->StartPoint);
				float WallSize = FVector::Distance(Wall->EndPoint, Wall->StartPoint);
				if (MidPoint.X < LeftMidPoint.X) // left wall is RIGHT of this wall.
				{
					if (LeftSize >= WallSize) // left wall is bigger than this wall.
					{
						DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(-1, 0, 1),
							MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}
					else //left wall is smaller than this wall.
					{
						DMInput->SetDimensionString(FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 1, 1), LeftMidPoint + FVector(0, 1, 1),
							FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1), LeftMidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}

				}
				else // node is LEFT of this wall
				{
					if (LeftSize >= WallSize) // left wall is bigger than this wall.
					{
						DMInput->SetDimensionString(FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
							FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}
					else //left wall is smaller than this wall.
					{
						DMInput->SetDimensionString(LeftMidPoint + FVector(0, 1, 1), FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 1, 1),
							LeftMidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}

				}
				WhichWasConnected = 1;
				InteriorDimensionStrings.Push(DMInput);
			}
			else
			{
				//choose right side because it's better to choose a side over roomnodes.
				ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
				FVector RightMidPoint = RightSide->StartPoint + 0.5f*(RightSide->EndPoint - RightSide->StartPoint);
				float RightSize = FVector::Distance(RightSide->EndPoint, RightSide->StartPoint);
				float WallSize = FVector::Distance(Wall->EndPoint, Wall->StartPoint);
				if (MidPoint.X < RightMidPoint.X) //right wall is RIGHT of this wall.
				{
					if (RightSize >= WallSize) //right wall is bigger than this wall.
					{
						DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(-1, 0, 1),
							MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}
					else //right wall is smaller than this wall.
					{
						DMInput->SetDimensionString(FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 1, 1), RightMidPoint + FVector(0, 1, 1),
							FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1), RightMidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}

				}
				else // node is LEFT of this wall
				{
					if (RightSize >= WallSize) // right wall is bigger than this wall.
					{
						DMInput->SetDimensionString(FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
							FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}
					else //right wall is smaller than this wall.
					{
						DMInput->SetDimensionString(RightMidPoint + FVector(0, 1, 1), FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 1, 1),
							RightMidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}

				}
				WhichWasConnected = 2;
				InteriorDimensionStrings.Push(DMInput);
			}
		}
		else //both sides exist, so we ignore room nodes.
		{
			//grounded check for existing connections (only walls)
			if (LeftSide->bGrounded && RightSide->bGrounded)
			{
				//if both are grounded, choose closest one.
				if (LeftSideBest <= RightSideBest)
				{
					//choose left wall, closest grounded wall
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector LeftMidPoint = LeftSide->StartPoint + 0.5f*(LeftSide->EndPoint - LeftSide->StartPoint);
					float LeftSize = FVector::Distance(LeftSide->EndPoint, LeftSide->StartPoint);
					float WallSize = FVector::Distance(Wall->EndPoint, Wall->StartPoint);
					if (MidPoint.X < LeftMidPoint.X) // left wall is RIGHT of this wall.
					{
						if (LeftSize >= WallSize) // left wall is bigger than this wall.
						{
							DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(-1, 0, 1),
								MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //left wall is smaller than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 1, 1), LeftMidPoint + FVector(0, 1, 1),
								FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1), LeftMidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					else // node is LEFT of this wall
					{
						if (LeftSize >= WallSize) // left wall is bigger than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
								FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //left wall is smaller than this wall.
						{
							DMInput->SetDimensionString(LeftMidPoint + FVector(0, 1, 1), FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 1, 1),
								LeftMidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					WhichWasConnected = 1;
					InteriorDimensionStrings.Push(DMInput);
				}
				else
				{
					//choose right wall, closest grounded wall
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector RightMidPoint = RightSide->StartPoint + 0.5f*(RightSide->EndPoint - RightSide->StartPoint);
					float RightSize = FVector::Distance(RightSide->EndPoint, RightSide->StartPoint);
					float WallSize = FVector::Distance(Wall->EndPoint, Wall->StartPoint);
					if (MidPoint.X < RightMidPoint.X) //right wall is RIGHT of this wall.
					{
						if (RightSize >= WallSize) //right wall is bigger than this wall.
						{
							DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(-1, 0, 1),
								MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //right wall is smaller than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 1, 1), RightMidPoint + FVector(0, 1, 1),
								FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1), RightMidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					else // node is LEFT of this wall
					{
						if (RightSize >= WallSize) // right wall is bigger than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
								FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //right wall is smaller than this wall.
						{
							DMInput->SetDimensionString(RightMidPoint + FVector(0, 1, 1), FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 1, 1),
								RightMidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					WhichWasConnected = 2;
					InteriorDimensionStrings.Push(DMInput);
				}
			}
			else if (LeftSide->bGrounded || RightSide->bGrounded)
			{
				if (LeftSide->bGrounded)
				{
					//choose left wall, only wall that is grounded
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector LeftMidPoint = LeftSide->StartPoint + 0.5f*(LeftSide->EndPoint - LeftSide->StartPoint);
					float LeftSize = FVector::Distance(LeftSide->EndPoint, LeftSide->StartPoint);
					float WallSize = FVector::Distance(Wall->EndPoint, Wall->StartPoint);
					if (MidPoint.X < LeftMidPoint.X) // left wall is RIGHT of this wall.
					{
						if (LeftSize >= WallSize) // left wall is bigger than this wall.
						{
							DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(-1, 0, 1),
								MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //left wall is smaller than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 1, 1), LeftMidPoint + FVector(0, 1, 1),
								FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1), LeftMidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					else // node is LEFT of this wall
					{
						if (LeftSize >= WallSize) // left wall is bigger than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
								FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //left wall is smaller than this wall.
						{
							DMInput->SetDimensionString(LeftMidPoint + FVector(0, 1, 1), FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 1, 1),
								LeftMidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					WhichWasConnected = 1;
					InteriorDimensionStrings.Push(DMInput);
				}
				else
				{
					//choose right wall, only wall that is grounded
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector RightMidPoint = RightSide->StartPoint + 0.5f*(RightSide->EndPoint - RightSide->StartPoint);
					float RightSize = FVector::Distance(RightSide->EndPoint, RightSide->StartPoint);
					float WallSize = FVector::Distance(Wall->EndPoint, Wall->StartPoint);
					if (MidPoint.X < RightMidPoint.X) //right wall is RIGHT of this wall.
					{
						if (RightSize >= WallSize) //right wall is bigger than this wall.
						{
							DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(-1, 0, 1),
								MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //right wall is smaller than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 1, 1), RightMidPoint + FVector(0, 1, 1),
								FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1), RightMidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					else // node is LEFT of this wall
					{
						if (RightSize >= WallSize) // right wall is bigger than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
								FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //right wall is smaller than this wall.
						{
							DMInput->SetDimensionString(RightMidPoint + FVector(0, 1, 1), FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 1, 1),
								RightMidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					WhichWasConnected = 2;
					InteriorDimensionStrings.Push(DMInput);
				}
			}
			else
			{
				if (LeftSideBest <= RightSideBest)
				{
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector LeftMidPoint = LeftSide->StartPoint + 0.5f*(LeftSide->EndPoint - LeftSide->StartPoint);
					float LeftSize = FVector::Distance(LeftSide->EndPoint, LeftSide->StartPoint);
					float WallSize = FVector::Distance(Wall->EndPoint, Wall->StartPoint);
					if (MidPoint.X < LeftMidPoint.X) // left wall is RIGHT of this wall.
					{
						if (LeftSize >= WallSize) // left wall is bigger than this wall.
						{
							DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(-1, 0, 1),
								MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //left wall is smaller than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 1, 1), LeftMidPoint + FVector(0, 1, 1),
								FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1), LeftMidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					else // node is LEFT of this wall
					{
						if (LeftSize >= WallSize) // left wall is bigger than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
								FVector::PointPlaneProject(MidPoint, LeftMidPoint, WallNormal) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //left wall is smaller than this wall.
						{
							DMInput->SetDimensionString(LeftMidPoint + FVector(0, 1, 1), FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 1, 1),
								LeftMidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					WhichWasConnected = 1;
					InteriorDimensionStrings.Push(DMInput);
				}
				else
				{
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector RightMidPoint = RightSide->StartPoint + 0.5f*(RightSide->EndPoint - RightSide->StartPoint);
					float RightSize = FVector::Distance(RightSide->EndPoint, RightSide->StartPoint);
					float WallSize = FVector::Distance(Wall->EndPoint, Wall->StartPoint);
					if (MidPoint.X < RightMidPoint.X) //right wall is RIGHT of this wall.
					{
						if (RightSize >= WallSize) //right wall is bigger than this wall.
						{
							DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(-1, 0, 1),
								MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //right wall is smaller than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 1, 1), RightMidPoint + FVector(0, 1, 1),
								FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1), RightMidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					else // node is LEFT of this wall
					{
						if (RightSize >= WallSize) // right wall is bigger than this wall.
						{
							DMInput->SetDimensionString(FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
								FVector::PointPlaneProject(MidPoint, RightMidPoint, WallNormal) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}
						else //right wall is smaller than this wall.
						{
							DMInput->SetDimensionString(RightMidPoint + FVector(0, 1, 1), FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 1, 1),
								RightMidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightMidPoint, MidPoint, WallNormal) + FVector(0, 0, 1),
								FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
						}

					}
					WhichWasConnected = 2;
					InteriorDimensionStrings.Push(DMInput);
				}
			}
		}

		//chain walls and room nodes here.
		//if a wall is grounded so is its roomnodes.
		FoundGrounded = false;
		if (WhichWasConnected == 1)
		{
			Wall->LeftChainedWall = LeftSide;
			LeftSide->RightChainedWall = Wall;
			 UpdateGrounded(Wall);
		}
		else if (WhichWasConnected == 2)
		{
			Wall->RightChainedWall = RightSide;
			RightSide->LeftChainedWall = Wall;
			 UpdateGrounded(Wall);
		}
		else if (WhichWasConnected == 3)
		{
			Wall->LeftChainedNode = LeftNode;
			LeftNode->RightChainedWall = Wall;
			 UpdateGrounded(Wall);
		}
		else if (WhichWasConnected == 4)
		{
			Wall->RightChainedNode = RightNode;
			RightNode->LeftChainedWall = Wall;
			 UpdateGrounded(Wall);
		}
	}
	/*
	Interior Nodes
	*/
	//set up the 4 TArrays containing Interior and Exterior Horizontal and Vertical Walls
	for (AWall * Wall : Walls)
	{
		//reset visited
		Wall->bVisited = false;
	}
	for (ARoomNode * Node : InteriorDiagonalWalls)
	{
		FVector MidPoint = Node->GetActorLocation();
		TArray<AWall*> WallsInLeftSide;
		TArray<AWall*> WallsInRightSide;
		TArray<ARoomNode*> NodesInLeftSide;
		TArray<ARoomNode*> NodesInRightSide;
		int WhichWasConnected = 0; //1 = left wall, 2 = right wall, 3 = left node, 4 = right node, 0 = nothing

		for (AWall * PotOppoWall : Node->AssociatedWall->LeftRoom->RoomData.WallsOrdered)
		{
			if (PotOppoWall != Node->AssociatedWall && Node->LeftChainedWall != PotOppoWall && 
				(PotOppoWall->GetActorRightVector().GetAbs().Equals(VerticalNorm, 0.0001f) || PotOppoWall->GetActorRightVector().GetAbs().Equals(HorizontalNorm, 0.0001f)))
			{
				WallsInLeftSide.Add(PotOppoWall);
			}
		}

		for (ARoomNode * RoomNode : Node->AssociatedWall->LeftRoom->RoomData.Nodes)
		{
			if (RoomNode != Node && Node->LeftChainedNode != RoomNode)
			{
				NodesInLeftSide.Add(RoomNode);
			}
		}

		for (AWall * PotOppoWall : Node->AssociatedWall->RightRoom->RoomData.WallsOrdered)
		{
			if (PotOppoWall != Node->AssociatedWall && Node->RightChainedWall != PotOppoWall &&
				(PotOppoWall->GetActorRightVector().GetAbs().Equals(VerticalNorm, 0.0001f) || PotOppoWall->GetActorRightVector().GetAbs().Equals(HorizontalNorm, 0.0001f)))
			{
				WallsInRightSide.Add(PotOppoWall);
			}
		}

		for (ARoomNode * RoomNode : Node->AssociatedWall->RightRoom->RoomData.Nodes)
		{
			if (RoomNode != Node && Node->RightChainedNode != RoomNode)
			{
				NodesInRightSide.Add(RoomNode);
			}
		}

		float LeftSideBest = INFINITY;
		AWall* LeftSide = nullptr;
		for (AWall * WallCheck : WallsInLeftSide)
		{
			FVector WallCheckNormal = WallCheck->GetActorRightVector();
			FVector OtherWallHit = FVector::PointPlaneProject(MidPoint, WallCheck->StartPoint, WallCheckNormal);
			float t;
			float LeftSideVal;

			if (FMath::IsNearlyEqual(WallCheckNormal.X, 0.0f, 0.0001f) == 0)
			{
				//t = (MidPoint.Y - OtherWallHit.Y);
				//LeftSideVal = FMath::Abs(t*WallCheckNormal.Y);
				t = (OtherWallHit.Y - WallCheck->StartPoint.Y) / (WallCheck->EndPoint.Y - WallCheck->StartPoint.Y);
				LeftSideVal = FMath::Abs(t*WallCheckNormal.X);
			}
			else
			{
				//t = (MidPoint.X - OtherWallHit.X);
				//LeftSideVal = FMath::Abs(t*WallCheckNormal.X);
				t = (OtherWallHit.X - WallCheck->StartPoint.X) / (WallCheck->EndPoint.X - WallCheck->StartPoint.X);
				LeftSideVal = FMath::Abs(t*WallCheckNormal.X);
			}

			if (t >= 0 || t <= 1)
			{
				if (!LeftSide || (WallCheck->bGrounded && LeftSide->bGrounded && LeftSideVal < LeftSideBest)
					|| (WallCheck->bGrounded && !LeftSide->bGrounded) || (!WallCheck->bGrounded && !LeftSide->bGrounded && LeftSideVal < LeftSideBest))
				{
					LeftSide = WallCheck;
					LeftSideBest = LeftSideVal;
				}
			}
		}

		float LeftNodeBest = INFINITY;
		ARoomNode *  LeftNode = nullptr;
		FVector tLeftCompBest = FVector::ZeroVector;
		int LeftOptBest = 0;
		for (ARoomNode * NodeCheck : NodesInLeftSide)
		{
			FVector tx;
			FVector ty;
			FVector tcomp;
			float t;
			float LeftNodeVal;
			int LeftOpt;

			tx = FVector::PointPlaneProject(MidPoint, NodeCheck->GetActorLocation(), VerticalNorm);
			ty = FVector::PointPlaneProject(MidPoint, NodeCheck->GetActorLocation(), HorizontalNorm);

			if (tx.Size() >= ty.Size())
			{
				t = ty.Size();
				LeftNodeVal = FMath::Abs(t);
				tx.Normalize(); //normalize the to-be normal
				tcomp = tx; //WE WANT THE NORMAL
				LeftOpt = 2;
			}
			else
			{
				t = tx.Size();
				LeftNodeVal = FMath::Abs(t);
				ty.Normalize(); //normalize the to-be normal
				tcomp = ty; //WE WANT THE NORMAL
				LeftOpt = 1;
			}

			if (t > 0)
			{
				if (!LeftNode || (NodeCheck->bGrounded && LeftNode->bGrounded && LeftNodeVal < LeftNodeBest)
					|| (NodeCheck->bGrounded && !LeftNode->bGrounded) || (!NodeCheck->bGrounded && !LeftNode->bGrounded && LeftNodeVal < LeftNodeBest))
				{
					LeftNode = NodeCheck;
					LeftNodeBest = LeftNodeVal;
					tLeftCompBest = tcomp;
					LeftOptBest = LeftOpt;
				}
			}
		}

		float RightSideBest = INFINITY;
		AWall* RightSide = nullptr;
		for (AWall * WallCheck : WallsInRightSide)
		{
			FVector WallCheckNormal = WallCheck->GetActorRightVector();
			FVector OtherWallHit = FVector::PointPlaneProject(MidPoint, WallCheck->StartPoint, -1*WallCheckNormal);
			float t;
			float RightSideVal;

			if (FMath::IsNearlyEqual(WallCheckNormal.X, 0.0f, 0.0001f) == 0)
			{
				//t = (MidPoint.Y - OtherWallHit.Y);
				//RightSideVal = FMath::Abs(t*WallCheckNormal.Y);
				t = -1*(OtherWallHit.Y - WallCheck->StartPoint.Y) / (WallCheck->EndPoint.Y - WallCheck->StartPoint.Y);
				RightSideVal = FMath::Abs(t*WallCheckNormal.X);
			}
			else
			{
				//t = (MidPoint.X - OtherWallHit.X);
				//RightSideVal = FMath::Abs(t*WallCheckNormal.X);
				t = -1*(OtherWallHit.X - WallCheck->StartPoint.X) / (WallCheck->EndPoint.X - WallCheck->StartPoint.X);
				RightSideVal = FMath::Abs(t*WallCheckNormal.X);
			}

			if (t >= 0 || t <= 1)
			{
				if (!RightSide || (WallCheck->bGrounded && RightSide->bGrounded && RightSideVal < RightSideBest)
					|| (WallCheck->bGrounded && !RightSide->bGrounded) || (!WallCheck->bGrounded && !RightSide->bGrounded && RightSideVal < RightSideBest))
				{
					RightSide = WallCheck;
					RightSideBest = RightSideVal;
				}
			}
		}

		float RightNodeBest = INFINITY;
		ARoomNode *  RightNode = nullptr;
		FVector tRightCompBest = FVector::ZeroVector;
		int RightOptBest = 0;
		for (ARoomNode * NodeCheck : NodesInRightSide)
		{
			FVector tx;
			FVector ty;
			FVector tcomp;
			float t;
			float RightNodeVal;
			int RightOpt;

			tx = FVector::PointPlaneProject(MidPoint, NodeCheck->GetActorLocation(), VerticalNorm);
			ty = FVector::PointPlaneProject(MidPoint, NodeCheck->GetActorLocation(), HorizontalNorm);

			if (tx.Size() >= ty.Size())
			{
				t = ty.Size();
				RightNodeVal = FMath::Abs(t);
				tx.Normalize(); //normalize the to-be normal
				tcomp = tx; //WE WANT THE NORMAL
				RightOpt = 2;
			}
			else
			{
				t = tx.Size();
				RightNodeVal = FMath::Abs(t);
				ty.Normalize(); //normalize the to-be normal
				tcomp = ty; //WE WANT THE NORMAL
				RightOpt = 1;
			}

			if (t > 0)
			{
				if (!RightNode || (NodeCheck->bGrounded && RightNode->bGrounded && RightNodeVal < RightNodeBest)
					|| (NodeCheck->bGrounded && !RightNode->bGrounded) || (!NodeCheck->bGrounded && !RightNode->bGrounded && RightNodeVal < RightNodeBest))
				{
					RightNode = NodeCheck;
					RightNodeBest = RightNodeVal;
					tRightCompBest = tcomp;
					RightOptBest = RightOpt;
				}
			}
		}



		if (LeftSide == nullptr && RightSide == nullptr) // no side connections exist.
		{
			if (LeftNode == nullptr && RightNode == nullptr)
			{
				// do nothing, no connections exist.
			}
			else if (LeftNode == nullptr || RightNode == nullptr)
			{
				if (LeftNode != nullptr)
				{
					//choose left node bc it is the only connection that exists.
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					if (LeftOptBest == 1) //roomnode is shorter in the x direction, so make the dm horizontal
					{
						if (MidPoint.Y > LeftNode->GetActorLocation().Y) // curr node is above our node
						{
							if (LeftNode->GetActorLocation().X > MidPoint.X) // curr node is left of our node
							{
								DMInput->SetDimensionString(MidPoint + FVector(0, -1, 1), LeftNode->GetActorLocation() + FVector(0, -1, 1),
									MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else // curr node is right of our node
							{
								DMInput->SetDimensionString( LeftNode->GetActorLocation() + FVector(0, -1, 1), MidPoint + FVector(0, -1, 1),
									FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						else // curr node is below our node
						{
							if (LeftNode->GetActorLocation().X <= MidPoint.X) // curr node is left of our node
							{

								DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
									FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else // curr node is right of our node
							{
								DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), LeftNode->GetActorLocation() + FVector(0, 1, 1),
									MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
					}
					else if (RightOptBest == 2) //roomnode is shorter in the y direction, so make the dm vertical
					{
						if (MidPoint.Y > LeftNode->GetActorLocation().Y) // curr node is above our node
						{
							if (LeftNode->GetActorLocation().X > MidPoint.X) // curr node is left of our node
							{
								DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
									FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else // curr node is right of our node
							{
								DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
									FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						else // curr node is below our node
						{
							if (LeftNode->GetActorLocation().X <= MidPoint.X) // curr node is left of our node
							{
								DMInput->SetDimensionString(MidPoint + FVector(1, 0, 1), LeftNode->GetActorLocation() + FVector(1, 0, 1),
									MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else // curr node is right of our node
							{
								DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), LeftNode->GetActorLocation() + FVector(-1, 0, 1),
									MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
					}
					WhichWasConnected = 3;
					InteriorDimensionStrings.Push(DMInput);
				}
				else
				{
					//choose right node bc it is the only connection to exist.
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					if (RightOptBest == 1) //roomnode is shorter in the x direction, so make the dm horizontal
					{
						if (MidPoint.Y > RightNode->GetActorLocation().Y) // curr node is above our node
						{
							if (RightNode->GetActorLocation().X > MidPoint.X) // curr node is left of our node
							{
								DMInput->SetDimensionString(MidPoint + FVector(0, -1, 1), RightNode->GetActorLocation() + FVector(0, -1, 1),
									MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else // curr node is right of our node
							{
								DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(0, -1, 1), MidPoint + FVector(0, -1, 1),
									FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						else // curr node is below our node
						{
							if (RightNode->GetActorLocation().X <= MidPoint.X) // curr node is left of our node
							{

								DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
									FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else // curr node is right of our node
							{
								DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), RightNode->GetActorLocation() + FVector(0, 1, 1),
									MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
					}
					else if (RightOptBest == 2) //roomnode is shorter in the y direction, so make the dm vertical
					{
						if (MidPoint.Y > RightNode->GetActorLocation().Y) // curr node is above our node
						{
							if (RightNode->GetActorLocation().X > MidPoint.X) // curr node is left of our node
							{
								DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
									FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else // curr node is right of our node
							{
								DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
									FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
						else // curr node is below our node
						{
							if (RightNode->GetActorLocation().X <= MidPoint.X) // curr node is left of our node
							{
								DMInput->SetDimensionString(MidPoint + FVector(1, 0, 1), RightNode->GetActorLocation() + FVector(1, 0, 1),
									MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
							else // curr node is right of our node
							{
								DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), RightNode->GetActorLocation() + FVector(-1, 0, 1),
									MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
									FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
							}
						}
					}
					WhichWasConnected = 4;
					InteriorDimensionStrings.Push(DMInput);
				}
			}
			else //Left and Right Node only exist
			{
				//grounded check for existing connections (only nodes)
				if (LeftNode->bGrounded && RightNode->bGrounded)
				{
					//if both are grounded, choose closest one.
					if (LeftNodeBest <= RightNodeBest)
					{
						//choose left node, closest grounded node
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (LeftOptBest == 1) //roomnode is shorter in the x direction, so make the dm horizontal
						{
							if (MidPoint.Y > LeftNode->GetActorLocation().Y) // curr node is above our node
							{
								if (LeftNode->GetActorLocation().X > MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(0, -1, 1), LeftNode->GetActorLocation() + FVector(0, -1, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(0, -1, 1), MidPoint + FVector(0, -1, 1),
										FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
							else // curr node is below our node
							{
								if (LeftNode->GetActorLocation().X <= MidPoint.X) // curr node is left of our node
								{

									DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
										FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), LeftNode->GetActorLocation() + FVector(0, 1, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
						}
						else if (RightOptBest == 2) //roomnode is shorter in the y direction, so make the dm vertical
						{
							if (MidPoint.Y > LeftNode->GetActorLocation().Y) // curr node is above our node
							{
								if (LeftNode->GetActorLocation().X > MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
										FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
										FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
							else // curr node is below our node
							{
								if (LeftNode->GetActorLocation().X <= MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(1, 0, 1), LeftNode->GetActorLocation() + FVector(1, 0, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), LeftNode->GetActorLocation() + FVector(-1, 0, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
						}
						WhichWasConnected = 3;
						InteriorDimensionStrings.Push(DMInput);
					}
					else
					{
						//choose right node, closest grounded node
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (RightOptBest == 1) //roomnode is shorter in the x direction, so make the dm horizontal
						{
							if (MidPoint.Y > RightNode->GetActorLocation().Y) // curr node is above our node
							{
								if (RightNode->GetActorLocation().X > MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(0, -1, 1), RightNode->GetActorLocation() + FVector(0, -1, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(0, -1, 1), MidPoint + FVector(0, -1, 1),
										FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
							else // curr node is below our node
							{
								if (RightNode->GetActorLocation().X <= MidPoint.X) // curr node is left of our node
								{

									DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
										FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), RightNode->GetActorLocation() + FVector(0, 1, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
						}
						else if (RightOptBest == 2) //roomnode is shorter in the y direction, so make the dm vertical
						{
							if (MidPoint.Y > RightNode->GetActorLocation().Y) // curr node is above our node
							{
								if (RightNode->GetActorLocation().X > MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
										FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
										FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
							else // curr node is below our node
							{
								if (RightNode->GetActorLocation().X <= MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(1, 0, 1), RightNode->GetActorLocation() + FVector(1, 0, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), RightNode->GetActorLocation() + FVector(-1, 0, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
						}
						WhichWasConnected = 4;
						InteriorDimensionStrings.Push(DMInput);
					}
				}
				else if (LeftNode->bGrounded || RightNode->bGrounded)
				{
					if (LeftNode->bGrounded)
					{
						//choose left node, only node that is grounded
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (LeftOptBest == 1) //roomnode is shorter in the x direction, so make the dm horizontal
						{
							if (MidPoint.Y > LeftNode->GetActorLocation().Y) // curr node is above our node
							{
								if (LeftNode->GetActorLocation().X > MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(0, -1, 1), LeftNode->GetActorLocation() + FVector(0, -1, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(0, -1, 1), MidPoint + FVector(0, -1, 1),
										FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
							else // curr node is below our node
							{
								if (LeftNode->GetActorLocation().X <= MidPoint.X) // curr node is left of our node
								{

									DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
										FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), LeftNode->GetActorLocation() + FVector(0, 1, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
						}
						else if (RightOptBest == 2) //roomnode is shorter in the y direction, so make the dm vertical
						{
							if (MidPoint.Y > LeftNode->GetActorLocation().Y) // curr node is above our node
							{
								if (LeftNode->GetActorLocation().X > MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
										FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
										FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
							else // curr node is below our node
							{
								if (LeftNode->GetActorLocation().X <= MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(1, 0, 1), LeftNode->GetActorLocation() + FVector(1, 0, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), LeftNode->GetActorLocation() + FVector(-1, 0, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
						}
						WhichWasConnected = 3;
						InteriorDimensionStrings.Push(DMInput);
					}
					else
					{
						//choose right node, only node that is grounded
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (RightOptBest == 1) //roomnode is shorter in the x direction, so make the dm horizontal
						{
							if (MidPoint.Y > RightNode->GetActorLocation().Y) // curr node is above our node
							{
								if (RightNode->GetActorLocation().X > MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(0, -1, 1), RightNode->GetActorLocation() + FVector(0, -1, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(0, -1, 1), MidPoint + FVector(0, -1, 1),
										FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
							else // curr node is below our node
							{
								if (RightNode->GetActorLocation().X <= MidPoint.X) // curr node is left of our node
								{

									DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
										FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), RightNode->GetActorLocation() + FVector(0, 1, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
						}
						else if (RightOptBest == 2) //roomnode is shorter in the y direction, so make the dm vertical
						{
							if (MidPoint.Y > RightNode->GetActorLocation().Y) // curr node is above our node
							{
								if (RightNode->GetActorLocation().X > MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
										FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
										FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
							else // curr node is below our node
							{
								if (RightNode->GetActorLocation().X <= MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(1, 0, 1), RightNode->GetActorLocation() + FVector(1, 0, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), RightNode->GetActorLocation() + FVector(-1, 0, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
						}
						WhichWasConnected = 4;
						InteriorDimensionStrings.Push(DMInput);
					}
				}
				else
				{
					if (LeftNodeBest <= RightNodeBest)
					{
						//choose left node, closest ungrounded node.
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (LeftOptBest == 1) //roomnode is shorter in the x direction, so make the dm horizontal
						{
							if (MidPoint.Y > LeftNode->GetActorLocation().Y) // curr node is above our node
							{
								if (LeftNode->GetActorLocation().X > MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(0, -1, 1), LeftNode->GetActorLocation() + FVector(0, -1, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(0, -1, 1), MidPoint + FVector(0, -1, 1),
										FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
							else // curr node is below our node
							{
								if (LeftNode->GetActorLocation().X <= MidPoint.X) // curr node is left of our node
								{

									DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
										FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), LeftNode->GetActorLocation() + FVector(0, 1, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
						}
						else if (RightOptBest == 2) //roomnode is shorter in the y direction, so make the dm vertical
						{
							if (MidPoint.Y > LeftNode->GetActorLocation().Y) // curr node is above our node
							{
								if (LeftNode->GetActorLocation().X > MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
										FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(LeftNode->GetActorLocation() + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
										FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
							else // curr node is below our node
							{
								if (LeftNode->GetActorLocation().X <= MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(1, 0, 1), LeftNode->GetActorLocation() + FVector(1, 0, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), LeftNode->GetActorLocation() + FVector(-1, 0, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(LeftNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
						}
						WhichWasConnected = 3;
						InteriorDimensionStrings.Push(DMInput);
					}
					else
					{
						//choose right node, closest ungrounded node.
						ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
						if (RightOptBest == 1) //roomnode is shorter in the x direction, so make the dm horizontal
						{
							if (MidPoint.Y > RightNode->GetActorLocation().Y) // curr node is above our node
							{
								if (RightNode->GetActorLocation().X > MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(0, -1, 1), RightNode->GetActorLocation() + FVector(0, -1, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(0, -1, 1), MidPoint + FVector(0, -1, 1),
										FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
							else // curr node is below our node
							{
								if (RightNode->GetActorLocation().X <= MidPoint.X) // curr node is left of our node
								{

									DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
										FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(0, 1, 1), RightNode->GetActorLocation() + FVector(0, 1, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
						}
						else if (RightOptBest == 2) //roomnode is shorter in the y direction, so make the dm vertical
						{
							if (MidPoint.Y > RightNode->GetActorLocation().Y) // curr node is above our node
							{
								if (RightNode->GetActorLocation().X > MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
										FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(RightNode->GetActorLocation() + FVector(-1, 0, 1), MidPoint + FVector(-1, 0, 1),
										FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
							else // curr node is below our node
							{
								if (RightNode->GetActorLocation().X <= MidPoint.X) // curr node is left of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(1, 0, 1), RightNode->GetActorLocation() + FVector(1, 0, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
								else // curr node is right of our node
								{
									DMInput->SetDimensionString(MidPoint + FVector(-1, 0, 1), RightNode->GetActorLocation() + FVector(-1, 0, 1),
										MidPoint + FVector(0, 0, 1), FVector::PointPlaneProject(RightNode->GetActorLocation(), MidPoint, tRightCompBest) + FVector(0, 0, 1),
										FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
								}
							}
						}
						WhichWasConnected = 4;
						InteriorDimensionStrings.Push(DMInput);
					}
				}
			}
		}
		else if (LeftSide == nullptr || RightSide == nullptr)

		{
			if (LeftSide != nullptr)
			{
				//choose left side because it's better to choose a side over room nodes.
				ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
				FVector LeftWallNormal = LeftSide->GetActorRightVector();
				FVector LeftCollidePoint = FVector::PointPlaneProject(MidPoint, LeftSide->StartPoint, LeftWallNormal);

				if (FMath::IsNearlyEqual(MidPoint.Y, LeftCollidePoint.Y, 0.0001f)) // is the wall colliding point colinear on y axis.
				{
					DMInput->SetDimensionString(LeftCollidePoint + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
						LeftCollidePoint + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
						FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					
				}
				else // left wall is upper than node
				{
					DMInput->SetDimensionString(LeftCollidePoint + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
						LeftCollidePoint + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
						FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
				}
				
				WhichWasConnected = 1;
				InteriorDimensionStrings.Push(DMInput);
			}
			else
			{
				//choose right side because it's better to choose a side over roomnodes.
				ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
				FVector RightWallNormal = RightSide->GetActorRightVector();
				FVector RightCollidePoint = FVector::PointPlaneProject(MidPoint, RightSide->StartPoint, RightWallNormal);

				if (FMath::IsNearlyEqual(MidPoint.Y, RightCollidePoint.Y, 0.0001f)) // is the wall colliding point colinear on y axis.
				{
					DMInput->SetDimensionString(RightCollidePoint + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
						RightCollidePoint + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
						FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));

				}
				else // left wall is upper than node
				{
					DMInput->SetDimensionString(RightCollidePoint + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
						RightCollidePoint + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
						FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
				}

				WhichWasConnected = 2;
				InteriorDimensionStrings.Push(DMInput);
			}
		}
		else //both sides exist, so we ignore room nodes.
		{
			//grounded check for existing connections (only walls)
			if (LeftSide->bGrounded && RightSide->bGrounded)
			{
				//if both are grounded, choose closest one.
				if (LeftSideBest <= RightSideBest)
				{
					//choose left wall, closest grounded wall
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector LeftWallNormal = LeftSide->GetActorRightVector();
					FVector LeftCollidePoint = FVector::PointPlaneProject(MidPoint, LeftSide->StartPoint, LeftWallNormal);

					if (FMath::IsNearlyEqual(MidPoint.Y, LeftCollidePoint.Y, 0.0001f)) // is the wall colliding point colinear on y axis.
					{
						DMInput->SetDimensionString(LeftCollidePoint + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
							LeftCollidePoint + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));

					}
					else // left wall is upper than node
					{
						DMInput->SetDimensionString(LeftCollidePoint + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
							LeftCollidePoint + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}

					WhichWasConnected = 1;
					InteriorDimensionStrings.Push(DMInput);
				}
				else
				{
					//choose right wall, closest grounded wall
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector RightWallNormal = RightSide->GetActorRightVector();
					FVector RightCollidePoint = FVector::PointPlaneProject(MidPoint, RightSide->StartPoint, RightWallNormal);

					if (FMath::IsNearlyEqual(MidPoint.Y, RightCollidePoint.Y, 0.0001f)) // is the wall colliding point colinear on y axis.
					{
						DMInput->SetDimensionString(RightCollidePoint + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
							RightCollidePoint + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));

					}
					else // left wall is upper than node
					{
						DMInput->SetDimensionString(RightCollidePoint + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
							RightCollidePoint + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}

					WhichWasConnected = 2;
					InteriorDimensionStrings.Push(DMInput);
				}
			}
			else if (LeftSide->bGrounded || RightSide->bGrounded)
			{
				if (LeftSide->bGrounded)
				{
					//choose left wall, only wall that is grounded
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector LeftWallNormal = LeftSide->GetActorRightVector();
					FVector LeftCollidePoint = FVector::PointPlaneProject(MidPoint, LeftSide->StartPoint, LeftWallNormal);

					if (FMath::IsNearlyEqual(MidPoint.Y, LeftCollidePoint.Y, 0.0001f)) // is the wall colliding point colinear on y axis.
					{
						DMInput->SetDimensionString(LeftCollidePoint + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
							LeftCollidePoint + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));

					}
					else // left wall is upper than node
					{
						DMInput->SetDimensionString(LeftCollidePoint + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
							LeftCollidePoint + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}

					WhichWasConnected = 1;
					InteriorDimensionStrings.Push(DMInput);
				}
				else
				{
					//choose right wall, only wall that is grounded
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector RightWallNormal = RightSide->GetActorRightVector();
					FVector RightCollidePoint = FVector::PointPlaneProject(MidPoint, RightSide->StartPoint, RightWallNormal);

					if (FMath::IsNearlyEqual(MidPoint.Y, RightCollidePoint.Y, 0.0001f)) // is the wall colliding point colinear on y axis.
					{
						DMInput->SetDimensionString(RightCollidePoint + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
							RightCollidePoint + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));

					}
					else // left wall is upper than node
					{
						DMInput->SetDimensionString(RightCollidePoint + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
							RightCollidePoint + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}

					WhichWasConnected = 2;
					InteriorDimensionStrings.Push(DMInput);
				}
			}
			else
			{
				if (LeftSideBest <= RightSideBest)
				{
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector LeftWallNormal = LeftSide->GetActorRightVector();
					FVector LeftCollidePoint = FVector::PointPlaneProject(MidPoint, LeftSide->StartPoint, LeftWallNormal);

					if (FMath::IsNearlyEqual(MidPoint.Y, LeftCollidePoint.Y, 0.0001f)) // is the wall colliding point colinear on y axis.
					{
						DMInput->SetDimensionString(LeftCollidePoint + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
							LeftCollidePoint + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));

					}
					else // left wall is upper than node
					{
						DMInput->SetDimensionString(LeftCollidePoint + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
							LeftCollidePoint + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}

					WhichWasConnected = 1;
					InteriorDimensionStrings.Push(DMInput);
				}
				else
				{
					ADimensionStringBase * DMInput = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
					FVector RightWallNormal = RightSide->GetActorRightVector();
					FVector RightCollidePoint = FVector::PointPlaneProject(MidPoint, RightSide->StartPoint, RightWallNormal);

					if (FMath::IsNearlyEqual(MidPoint.Y, RightCollidePoint.Y, 0.0001f)) // is the wall colliding point colinear on y axis.
					{
						DMInput->SetDimensionString(RightCollidePoint + FVector(1, 0, 1), MidPoint + FVector(1, 0, 1),
							RightCollidePoint + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));

					}
					else // left wall is upper than node
					{
						DMInput->SetDimensionString(RightCollidePoint + FVector(0, 1, 1), MidPoint + FVector(0, 1, 1),
							RightCollidePoint + FVector(0, 0, 1), MidPoint + FVector(0, 0, 1),
							FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 0, 1));
					}

					WhichWasConnected = 2;
					InteriorDimensionStrings.Push(DMInput);
				}
			}
		}

		//chain walls and room nodes here.
		//if a wall is grounded so is its roomnodes.
		FoundGrounded = false;
		if (WhichWasConnected == 1)
		{
			Node->LeftChainedWall = LeftSide;
			LeftSide->RightChainedNode = Node;
			 UpdateGrounded(Node);
		}
		else if (WhichWasConnected == 2)
		{
			Node->RightChainedWall = RightSide;
			RightSide->LeftChainedNode = Node;
			 UpdateGrounded(Node);
		}
		else if (WhichWasConnected == 3)
		{
			Node->LeftChainedNode = LeftNode;
			LeftNode->RightChainedNode = Node;
			 UpdateGrounded(Node);
		}
		else if (WhichWasConnected == 4)
		{
			Node->RightChainedNode = RightNode;
			RightNode->LeftChainedNode = Node;
			 UpdateGrounded(Node);
		}
	}
}

void UEditManager::UpdateGrounded(AWall * Wall)
{
	if (Wall && !Wall->bVisited)
	{
		if (FoundGrounded)
		{
			Wall->bGrounded = true;
			Wall->StartNode->bGrounded = true;
			Wall->EndNode->bGrounded = true;
		}
		else if (Wall->bGrounded)
		{
			Wall->bGrounded = true;
			Wall->StartNode->bGrounded = true;
			Wall->EndNode->bGrounded = true;
			FoundGrounded = true;
		}

		Wall->bVisited = true;

		UpdateGrounded(Wall->LeftChainedWall);
		UpdateGrounded(Wall->RightChainedWall);
		UpdateGrounded(Wall->LeftChainedNode);
		UpdateGrounded(Wall->RightChainedNode);
		
	}
}

void UEditManager::UpdateGrounded(ARoomNode * Node)
{
	if(Node && !Node->bVisited)
	{
		if (FoundGrounded)
		{
			Node->bGrounded = true;
			if (Node->AssociatedWall->StartNode->bGrounded && Node->AssociatedWall->EndNode->bGrounded)
				Node->AssociatedWall->bGrounded = true;
		}
		else if (Node->bGrounded)
		{
			Node->bGrounded = true;
			if (Node->AssociatedWall->StartNode->bGrounded && Node->AssociatedWall->EndNode->bGrounded)
				Node->AssociatedWall->bGrounded = true;
			FoundGrounded = true;
		}

		Node->bVisited = true;

		UpdateGrounded(Node->LeftChainedWall);
		UpdateGrounded(Node->RightChainedWall);
		UpdateGrounded(Node->LeftChainedNode);
		UpdateGrounded(Node->RightChainedNode);

	}
}

void UEditManager::MakeHeightDMInvisible()
{
	for (AWall * Wall : Walls)
	{
		Wall->HeightDimensionString->SetInvisible();
	}
}
void UEditManager::MakeHeightDMVisible()
{
	for (AWall * Wall : Walls)
	{
		Wall->HeightDimensionString->SetVisible();
	}
}