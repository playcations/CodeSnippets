///UEditManager : The Edit Manager is the interface between the rpimitives and the editor. 
///Here is where you will impliment features for the primitives that require user interface.
///
#include "EditManager.h"

#include "Serialization/JsonTypes.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "ModumateGameInstance.h"
#include "DrawDebugHelpers.h"
#include "Wall.h"
#include "Window.h"
#include "Floor.h"
#include "CaseWorkLine.h"
#include "ProceduralMeshComponent.h"


UEditManager::UEditManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, WallClass(AWall::StaticClass())
	, WindowClass(AWindow::StaticClass())
	, FloorClass(AWindow::StaticClass())
	, CaseWorkLineClass(ACaseWorkLine::StaticClass())
	, PendingWall(nullptr)
	, PendingFloorLine(nullptr)
	, PendingCaseWorkLine(nullptr)
{}

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

#pragma region Windows

///PlaceWindow
///Spawns window on wall at location
///
///AWall* Wall: wall to attach to
///FVector& WindowPos: position
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

#pragma endregion

#pragma region Floors

///StartFloor
///Spawns first floor location
///
///FVector& floorOrigin: Starting location. Mouse hit location
AFloor* UEditManager::StartFloor(const FVector& floorOrigin)
{
	if (!ensureAlways(!PendingFloorLine))
	{
		return nullptr;
	}

	PendingFloorLine = SpawnFloor(floorOrigin);
	return PendingFloorLine;
}

///FinishFloorLine
///adds finished floor side and adds it to a list
///
///FVector& floorEnd: mouse hit location for end of current side 
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

///SpawnFloor
///Spawns floor side. Starting point in mouse hit location, end is mopuse location
///
///FVector& Origin:Mouse hit location
AFloor* UEditManager::SpawnFloor(const FVector& Origin)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	return GetWorld()->SpawnActor<AFloor>(FloorClass, FTransform(FQuat::Identity, Origin), SpawnParams);
}

///RemoveFloor
///clears all floor points made so far
///
///AFloor* Floor: selected floor
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

///GenerateFloorBase
///Once all points have been made and floor is ready to be drawn, this will take the procedural mesh component and generate a floor to the specs asked for
///
///UProceduralMeshComponent* FloorBase: takes in floor mesh component for the floor and add points necessary for it's creation
///float depth: The depth the floor is reaching CM is converted to inches ahead of time.
void UEditManager::GenerateFloorBase(UProceduralMeshComponent* FloorBase, float depth)
{
	FloorBase->bUseAsyncCooking = true;

	TArray<FVector> vertices;

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

	//generate 
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

	//setup the rest of the variables, not necessarry right now, but will be eventually
	TArray<FVector> normals;
	TArray<FVector2D> UV0;
	TArray<FProcMeshTangent> tangents;
	TArray<FLinearColor> vertexColors;

	FloorBase->CreateMeshSection_LinearColor(0, vertices, triangles, normals, UV0, vertexColors, tangents, true);
}

#pragma endregion

#pragma region Casework

///StartCaseWorkLine
///Spawns first casework location
///
///FVector& CaseWorkLineOrigin: Starting location. Mouse hit location
ACaseWorkLine* UEditManager::StartCaseWorkLine(const FVector& CaseWorkLineOrigin)
{
	if (!ensureAlways(!PendingCaseWorkLine))
	{
		return nullptr;
	}

	PendingCaseWorkLine = SpawnCaseWorkLine(CaseWorkLineOrigin);
	return PendingCaseWorkLine;
}

///FinishCaseWorkLine
///adds finished casework side and adds it to a list
///
///FVector& CaseWorkLineEnd: mouse hit location for end of current side 
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

///SpawnCaseWorkLine
///Spawns casework side. Starting point in mouse hit location, end is mouse location
///
///FVector& Origin:Mouse hit location
ACaseWorkLine* UEditManager::SpawnCaseWorkLine(const FVector& Origin)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	return GetWorld()->SpawnActor<ACaseWorkLine>(CaseWorkLineClass, FTransform(FQuat::Identity, Origin), SpawnParams);
}

///RemoveCaseWorkLine
///clears all casework points made so far
///
///ACaseWorkLine* CaseWorkLine: selected CaseWork
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

///GenerateFloorBase
///Once all points have been made and casework is ready to be drawn, this will take the procedural mesh component and generate a casework to the specs asked for
///
///UProceduralMeshComponent* CaseWorkBaseBase: takes in casework mesh component for the casework and add points necessary for it's creation
///float height: The height the casework is reaching CM is converted to inches ahead of time.
///AActor* CaseworkGeneratedActor: Adds created casework to Casework list for later editing
void UEditManager::GenerateCaseWork(UProceduralMeshComponent* CaseWorkBaseBase, float height, AActor* CaseworkGeneratedActor)
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

	CaseWorkBaseBase->CreateMeshSection_LinearColor(0, AllVerts, triangles, normals, UV0, vertexColors, tangents, true);
	CaseWorkLines.Empty();
	CaseworkCompletes.Add(CaseworkGeneratedActor);
}

#pragma endregion

#pragma region Walls

///StartWall
///Spawns first wall location
///FVector& WallOrigin: Starting location. Mouse hit location
AWall* UEditManager::StartWall(const FVector& WallOrigin)
{
	if (!ensureAlways(!PendingWall))
	{
		return nullptr;
	}

	PendingWall = SpawnWall(WallOrigin);
	return PendingWall;
}

///FinishWall
///finalize wall info and save it to wall list. 
AWall* UEditManager::FinishWall(const FVector& WallEnd)
{
	if (!ensureAlways(PendingWall))
	{
		return nullptr;
	}

	PendingWall->ID = Walls.Num();
	PendingWall->SetEndPoint(WallEnd);
	PendingWall->bPlaced = true;

	Walls.Add(PendingWall);

	AWall* NewWall = PendingWall;
	PendingWall = nullptr;

	OnWallMoved(NewWall);
	UpdateDimensionStringsForInteriorWalls();

	return NewWall;
}

///RemoveWall
///Remove selected wall Actor
///AWall: wall actor
void UEditManager::RemoveWall(class AWall* Wall)
{
	if (ensureAlways(Wall))
	{
		if (PendingWall == Wall)
		{
			PendingWall = nullptr;

			Wall->StartNode->Destroy();
			Wall->WidthDimensionString->Destroy();
			Wall->HeightDimensionString->Destroy();
			for (ADimensionStringBase* String : Wall->FixtureDimensionStrings)
			{
				String->Destroy();
			}
			Wall->FixtureDimensionStrings.Empty();

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

///SpawnWall
///spawns wall in location
///FVector& Origin: Center of wall.
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

///GenerateWall
///Once all points have been made and wall is ready to be drawn, this will take the procedural mesh component and generate a wall to the specs asked for
///
///UProceduralMeshComponent* WallMesh: takes in mesh component for the wall and add points necessary for it's creation
///float height: The height the wall in CM. Is converted from inches ahead of time.
///float thickness:The thickness of the wall in CM. Is converted from inches ahead of time.
///AWall* PendingWallActor: Current wall being drawn
void UEditManager::GenerateWall(UProceduralMeshComponent* WallMesh, float height, float thickness, AWall* PendingWallActor)
{

	WallMesh->bUseAsyncCooking = true;

	FVector WallDeltaStart = PendingWallActor->EndPoint - PendingWallActor->StartPoint;
	FVector crosL = FVector::CrossProduct(FVector::UpVector, WallDeltaStart);

	FVector crosLNorm = (crosL).GetUnsafeNormal();

	FVector LeftSideStart = PendingWallActor->StartPoint - (thickness * crosLNorm);
	FVector RightSideStart = PendingWallActor->StartPoint + (thickness * crosLNorm);
	FVector LeftSideEnd = PendingWallActor->EndPoint - (thickness * crosLNorm);
	FVector RightSideEnd = PendingWallActor->EndPoint + (thickness * crosLNorm);

	TArray<FVector> vertices;
	vertices.Add(LeftSideStart);
	vertices.Add(RightSideStart);
	vertices.Add(RightSideEnd);
	vertices.Add(LeftSideEnd);

	FWallBox currentWall;

	currentWall.b_LeftSideStart = LeftSideStart;
	currentWall.b_RightSideStart = RightSideStart;
	currentWall.b_LeftSideEnd = LeftSideEnd;
	currentWall.b_RightSideEnd = RightSideEnd;


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
	vertices.Add(FVector(LeftSideStart.X, LeftSideStart.Y, height * 12.0f * 2.54f));
	vertices.Add(FVector(RightSideStart.X, RightSideStart.Y, height * 12.0f * 2.54f));
	vertices.Add(FVector(RightSideEnd.X, RightSideEnd.Y, height * 12.0f * 2.54f));
	vertices.Add(FVector(LeftSideEnd.X, LeftSideEnd.Y, height * 12.0f * 2.54f));

	currentWall.t_LeftSideStart = FVector(LeftSideStart.X, LeftSideStart.Y, height * 12.0f * 2.54f);
	currentWall.t_RightSideStart = FVector(RightSideStart.X, RightSideStart.Y, height * 12.0f * 2.54f);
	currentWall.t_RightSideEnd = FVector(RightSideEnd.X, RightSideEnd.Y, height * 12.0f * 2.54f);
	currentWall.t_LeftSideEnd = FVector(LeftSideEnd.X, LeftSideEnd.Y, height * 12.0f * 2.54f);

	currentWall.wallVertices = vertices;
	currentWall.Triangles = Triangle;
	PendingWallActor->WallBoxes.Add(currentWall);

	////setup the rest of the variables
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

	PendingWallActor->wallThickness = thickness;
	PendingWallActor->wallVertices = vertices;
	WallMesh->CreateMeshSection_LinearColor(0, PendingWallActor->wallVertices, Triangle, normals, UV0, vertexColors, tangents, true);

}

///CutWindowIntoWall
///TODO PEYVAN: edit name and modify in BP
///Cuts box hole in wall for windows. Name in place for now due to time contraint. 
///UProceduralMeshComponent* WallMesh: Drawn wall mesh
///AWall* CurrentWallActor: Wall Actor with all needed info
///FVector Origin: Window origin
///FVector BoxExtend: window height/ width
///bool bIsDoor: if door, only cuts 3 boxes vs 4
///bool isPreview: Is being drawn on the fly when mouse is hovering over wall
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

///TriangulateWallBoxes
///Decomposes the polygon into triangles with a naive ear-clipping algorithm. Does not handle internal holes in the polygon.
///
///int i_currentWallVertSize: number of verts currently on the wall. This grows every time a new window is added
///int i_currentLocalVertSize: wall side count
///TArray<int> &Triangle: points used to triangulate
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

#pragma endregion

#pragma region other

///Triangulate
///Decomposes the polygon into triangles with a naive ear-clipping algorithm. Does not handle internal holes in the polygon.
///
///TArray<FVector> vertices: points used to triangulate
///
TArray<int32> UEditManager::Triangulate(TArray<FVector> vertices)
{

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

///IsWithinBoxBounds 
///Checks to make sure that the window/ door is within the bounds of the walls. 
///
///FVector origin: Component center point
///FVector bounds: Component bounds
///AWall *CurrentWall: Current wall that the component is trying to be placed on
bool  UEditManager::IsWithinBoxBounds(FVector origin, FVector bounds, AWall *CurrentWall)
{
	float objBounds = bounds.X;
	if (bounds.Y > objBounds)
	{
		objBounds = bounds.Y;
	}

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

#pragma endregion
