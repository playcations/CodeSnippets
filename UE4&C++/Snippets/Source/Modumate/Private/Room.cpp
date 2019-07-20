// Fill out your copyright notice in the Description page of Project Settings.

#include "Room.h"

#include "Serialization/JsonTypes.h"
#include "KismetMathLibrary.generated.h"
#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h"
#include "Wall.h"
#include "RoomNode.h"


/***************************FGraphUtils Implementation*******************/

bool FGraphUtils::AreEdgesMergeable(const FVector& V0, const FVector& V1, const FVector& V2)
{
	const FVector MergedEdgeVector = V2 - V0;
	const float MergedEdgeLengthSquared = MergedEdgeVector.SizeSquared();
	if (MergedEdgeLengthSquared > DELTA)
	{
		// Find the vertex closest to A1/B0 that is on the hypothetical merged edge formed by A0-B1.
		const float IntermediateVertexEdgeFraction = ((V2 - V0) | (V1 - V0)) / MergedEdgeLengthSquared;
		const FVector InterpolatedVertex = FMath::Lerp(V0, V2, IntermediateVertexEdgeFraction);

		// The edges are merge-able if the interpolated vertex is close enough to the intermediate vertex.
		return InterpolatedVertex.Equals(V1, THRESH_POINTS_ARE_SAME);
	}
	else
	{
		return true;
	}
}

bool FGraphUtils::VectorsOnSameSide(const FVector& Vec, const FVector& A, const FVector& B)
{
	const FVector CrossA = Vec ^ A;
	const FVector CrossB = Vec ^ B;
	return !FMath::IsNegativeFloat(CrossA | CrossB);
}

bool FGraphUtils::PointInTriangle(const FVector& A, const FVector& B, const FVector& C, const FVector& P)
{
	// Cross product indicates which 'side' of the vector the point is on
	// If its on the same side as the remaining vert for all edges, then its inside.	
	if (VectorsOnSameSide(B - A, P - A, C - A) &&
		VectorsOnSameSide(C - B, P - B, A - B) &&
		VectorsOnSameSide(A - C, P - C, B - C))
	{
		return true;
	}
	else
	{
		return false;
	}
}


/***************************FRoomData Implementation*********************/

FRoomData::FRoomData()
	: ID(-1)
	, Normal(FVector::UpVector)
	, Winding(0.0f)
	, bClosed(false)
	, MinWallIDIndex(0)
{ }

bool FRoomData::AddWall(class AWall* NewWall, bool bWallForward)
{
	if (NewWall == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Must pass in a non-null wall!"));
		return false;
	}

	TMap<AWall*, int32>& WallIndexMap = bWallForward ? ForwardWallIndices : BackwardWallIndices;
	if (WallIndexMap.Contains(NewWall))
	{
		UE_LOG(LogTemp, Warning, TEXT("Room #%d already contains wall %s%d!"), ID, bWallForward ? TEXT("+") : TEXT("-"), NewWall->ID);
		return false;
	}

	ARoomNode* NewStartNode = bWallForward ? NewWall->StartNode : NewWall->EndNode;
	bool bShouldClose = false;

	if (WallsOrdered.Num() > 0)
	{
		AWall* LastWall = WallsOrdered.Last();
		bool bLastWallForward = WallDirections.Last();
		ARoomNode* LastEndNode = bLastWallForward ? LastWall->EndNode : LastWall->StartNode;

		FVector LastWallDir = LastWall->GetWallDir() * (bLastWallForward ? 1.0f : -1.0f);
		FVector NewWallDir = NewWall->GetWallDir() * (bWallForward ? 1.0f : -1.0f);

		// Make sure the new wall is connected to the last wall
		if (LastEndNode != NewStartNode)
		{
			UE_LOG(LogTemp, Warning, TEXT("Room #%d : new wall %s%d must be connected to the last wall %s%d!"), ID,
				bWallForward ? TEXT("+") : TEXT("-"), NewWall->ID, bLastWallForward ? TEXT("+") : TEXT("-"), LastWall->ID);
			return false;
		}

		// See if the room is closed yet
		AWall* FirstWall = WallsOrdered[0];
		bool bFirstWallForward = WallDirections[0];

		ARoomNode* FirstStartNode = bFirstWallForward ? FirstWall->StartNode : FirstWall->EndNode;
		ARoomNode* NewEndNode = bWallForward ? NewWall->EndNode : NewWall->StartNode;

		if (FirstStartNode == NewEndNode)
		{
			bShouldClose = true;
		}
	}

	Nodes.Add(NewStartNode);
	WallIndexMap.Emplace(NewWall, WallsOrdered.Num());
	WallsOrdered.Add(NewWall);
	WallDirections.Add(bWallForward);

	if (bShouldClose)
	{
		Close();

		UE_LOG(LogTemp, Log, TEXT("Room #%d is now closed with %d walls, %d nodes, and winding %.2f"), ID, WallsOrdered.Num(), Nodes.Num(), Winding);

		if (!IsClockWise())
		{
			Triangulate();
		}
	}

	return true;
}

bool FRoomData::Close()
{
	if (!bClosed)
	{
		// Update the index of the minimum wall ID, for fast comparisons with other RoomData.
		int32 MinWallID = INT_MAX;
		MinWallIDIndex = 0;
		for (int32 WallIndex = 0; WallIndex < WallsOrdered.Num(); ++WallIndex)
		{
			AWall* Wall = WallsOrdered[WallIndex];
			if (Wall->ID < MinWallID)
			{
				MinWallIDIndex = WallIndex;
				MinWallID = Wall->ID;
			}
		}

		// Then, find the walls that form a strict loop (no walls are adjacent to the polygon on both sides)
		LoopWallIndices.Empty();
		for (int32 WallIndex = 0; WallIndex < WallsOrdered.Num(); ++WallIndex)
		{	
			AWall* Wall = WallsOrdered[WallIndex];
			bool bWallForward = WallDirections[WallIndex];

			int32 ForwardIndex, BackwardIndex;
			if (!ContainsWall(Wall, true, ForwardIndex) || !ContainsWall(Wall, false, BackwardIndex))
			{
				LoopWallIndices.Add(WallIndex);
			}
		}

		// Now, accumulate the winding accurately using the strict loop iteration
		Winding = 0.0f;
		for (int32 LoopWallIndexIndex = 0; LoopWallIndexIndex < LoopWallIndices.Num(); ++LoopWallIndexIndex)
		{
			int32 PrevLoopWallIndexIndex = (LoopWallIndexIndex == 0) ? (LoopWallIndices.Num() - 1) : (LoopWallIndexIndex - 1);
			int32 PrevLoopWallIndex = LoopWallIndices[PrevLoopWallIndexIndex];
			int32 CurLoopWallIndex = LoopWallIndices[LoopWallIndexIndex];

			AWall* PrevLoopWall = WallsOrdered[PrevLoopWallIndex];
			bool bPrevLoopWallForward = WallDirections[PrevLoopWallIndex];
			AWall* CurLoopWall = WallsOrdered[CurLoopWallIndex];
			bool bCurLoopWallForward = WallDirections[CurLoopWallIndex];

			Winding += GetWinding((bPrevLoopWallForward ? 1.0f : -1.0f) * PrevLoopWall->GetWallDir(),
				(bCurLoopWallForward ? 1.0f : -1.0f) * CurLoopWall->GetWallDir());
		}

		bClosed = true;
	}

	return bClosed;
}

bool FRoomData::ContainsWall(class AWall* Wall, bool bWallForward, int32& WallIndex) const
{
	const TMap<AWall*, int32>& WallIndexMap = bWallForward ? ForwardWallIndices : BackwardWallIndices;
	const int32* WallIndexPtr = WallIndexMap.Find(Wall);

	if (WallIndexPtr)
	{
		WallIndex = *WallIndexPtr;
		return true;
	}
	else
	{
		return false;
	}
}

int32 FRoomData::CompareWalls(const FRoomData& Other) const
{
#if 0
	int32 NumDifferentWalls = 0;

	for (auto& ThisForwardKVP : ForwardWallIndices)
	{
		if (!Other.ForwardWallIndices.Contains(ThisForwardKVP.Key))
		{
			++NumDifferentWalls;
		}
	}

	for (auto& OtherForwardKVP : Other.ForwardWallIndices)
	{
		if (!ForwardWallIndices.Contains(OtherForwardKVP.Key))
		{
			++NumDifferentWalls;
		}
	}

	for (auto& ThisBackwardKVP : BackwardWallIndices)
	{
		if (!Other.BackwardWallIndices.Contains(ThisBackwardKVP.Key))
		{
			++NumDifferentWalls;
		}
	}

	for (auto& OtherBackwardKVP : Other.BackwardWallIndices)
	{
		if (!BackwardWallIndices.Contains(OtherBackwardKVP.Key))
		{
			++NumDifferentWalls;
		}
	}

	return NumDifferentWalls;
#else
	int32 NumSharedWalls = 0;

	for (auto& ThisForwardKVP : ForwardWallIndices)
	{
		if (Other.ForwardWallIndices.Contains(ThisForwardKVP.Key))
		{
			++NumSharedWalls;
		}
	}

	for (auto& OtherForwardKVP : Other.ForwardWallIndices)
	{
		if (ForwardWallIndices.Contains(OtherForwardKVP.Key))
		{
			++NumSharedWalls;
		}
	}

	for (auto& ThisBackwardKVP : BackwardWallIndices)
	{
		if (Other.BackwardWallIndices.Contains(ThisBackwardKVP.Key))
		{
			++NumSharedWalls;
		}
	}

	for (auto& OtherBackwardKVP : Other.BackwardWallIndices)
	{
		if (BackwardWallIndices.Contains(OtherBackwardKVP.Key))
		{
			++NumSharedWalls;
		}
	}

	return NumSharedWalls;
#endif
}

bool FRoomData::Equals(const FRoomData& Other, bool bCompareIDs) const
{
	// Make sure the rooms have the same number of walls
	int32 NumWalls = WallsOrdered.Num();
	if (NumWalls != Other.WallsOrdered.Num())
	{
		return false;
	}

	// Potentially compare the room IDs (not useful if comparing temporarily-constructed RoomData)
	if (bCompareIDs && ID != Other.ID)
	{
		return false;
	}

	// Starting at the wall of each RoomData that has the minimum ID, compare that the same walls are used
	for (int32 WallIndex = 0; WallIndex < NumWalls; ++WallIndex)
	{
		int32 ThisIndex = (MinWallIDIndex + WallIndex) % NumWalls;
		int32 OtherIndex = (Other.MinWallIDIndex + WallIndex) % NumWalls;

		AWall* ThisWall = WallsOrdered[ThisIndex];
		bool bThisWallForward = WallDirections[ThisIndex];
		AWall* OtherWall = Other.WallsOrdered[OtherIndex];
		bool bOtherWallForward = Other.WallDirections[OtherIndex];

		if ((ThisWall != OtherWall) || (bThisWallForward != bOtherWallForward))
		{
			return false;
		}
	}

	return true;
}

float FRoomData::GetWinding(const FVector& FromDir, const FVector& ToDir) const
{
	FVector Cross = FromDir ^ ToDir;
	float AbsAngle = FMath::RadiansToDegrees(FMath::Acos(FromDir | ToDir));
	return AbsAngle * FMath::Sign(Cross | Normal);
}

bool FRoomData::Triangulate()
{
	/** Decomposes the polygon into triangles with a naive ear-clipping algorithm. Does not handle internal holes in the polygon.
	    Based on the implementation in Engine/Source/Runtime/Engine/Private/GeomTools.cpp, Copyright 1998-2017 Epic Games, Inc. **/

	bool bKeepColinearVertices = true;

	// Can't work if not enough verts for 1 triangle
	if (Nodes.Num() < 3)
	{
		// Return true because poly is already a tri
		return true;
	}

	// Vertices of polygon in order - make a copy we are going to modify.
	TArray<FVector> PolyVerts;
	TArray<int32> OriginalVertIndices;

	// Only iterate through the walls that form a strict loop, since only those satisfy
	// this implementation's constraints on polygons.
	// This would need to be updated to support triangulating polygons with holes.
	for (int32 LoopWallIndex : LoopWallIndices)
	{
		AWall* Wall = WallsOrdered[LoopWallIndex];
		bool bWallForward = WallDirections[LoopWallIndex];
		ARoomNode* LoopNode = bWallForward ? Wall->StartNode : Wall->EndNode;
		PolyVerts.Add(LoopNode->GetActorLocation());
		OriginalVertIndices.Add(LoopWallIndex);
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
				const float TriangleDeterminant = (ABEdge ^ ACEdge) | Normal;
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
				return false;
			}
		}
	}

	return true;
}


ARoom::ARoom(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void ARoom::BeginPlay()
{
	Super::BeginPlay();

}

void ARoom::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool ARoom::IsInterior() const
{
	return !RoomData.IsClockWise();
}

void ARoom::DebugDraw(float Duration) const
{
	TArray<FString> DebugWallStrings;
	for (int32 WallIndex = 0; WallIndex < RoomData.WallsOrdered.Num(); ++WallIndex)
	{
		DebugWallStrings.Add(FString::Printf(TEXT("%s%d"), RoomData.WallDirections[WallIndex] ? TEXT("+") : TEXT("-"), RoomData.WallsOrdered[WallIndex]->ID));
	}
	FString DebugRoomString = FString::Join(DebugWallStrings, TEXT(", "));
	UE_LOG(LogTemp, Log, TEXT("  Room #%d: %s"), RoomData.ID, *DebugRoomString);

	if (RoomData.TriangleIndices.Num() > 0)
	{
		TArray<FVector> DebugVertices;
		for (ARoomNode* RoomNode : RoomData.Nodes)
		{
			DebugVertices.Add(RoomNode->GetActorLocation());
		}
		FLinearColor DebugRoomHSVColor(360.0f * FMath::Fmod(PI * RoomData.ID, 1.0f), 1.0f, 1.0f);
		FColor DebugRoomColor = DebugRoomHSVColor.HSVToLinearRGB().ToFColor(false);
		DrawDebugMesh(GetWorld(), DebugVertices, RoomData.TriangleIndices, DebugRoomColor, Duration > 0.0f, Duration, 0);
	}
}

void ARoom::UpdateRoomData(const FRoomData& NewRoomData)
{
	RoomData = NewRoomData;
}

TSharedPtr<FJsonObject> ARoom::SerializeToJson() const
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());

	//TArray<TSharedPtr<FJsonValue>> StartPointJson;
	//StartPointJson.Add(MakeShareable(new FJsonValueNumber(StartPoint.X)));
	//StartPointJson.Add(MakeShareable(new FJsonValueNumber(StartPoint.Y)));
	//StartPointJson.Add(MakeShareable(new FJsonValueNumber(StartPoint.Z)));
	//ResultJson->SetArrayField(GET_MEMBER_NAME_STRING_CHECKED(ARoom, StartPoint), StartPointJson);
	//
	//TArray<TSharedPtr<FJsonValue>> EndPointJson;
	//EndPointJson.Add(MakeShareable(new FJsonValueNumber(EndPoint.X)));
	//EndPointJson.Add(MakeShareable(new FJsonValueNumber(EndPoint.Y)));
	//EndPointJson.Add(MakeShareable(new FJsonValueNumber(EndPoint.Z)));
	//ResultJson->SetArrayField(GET_MEMBER_NAME_STRING_CHECKED(ARoom, EndPoint), EndPointJson);

	return ResultJson;
}

bool ARoom::DeserializeFromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	//auto StartPointJson = JsonObject->GetArrayField(GET_MEMBER_NAME_STRING_CHECKED(ARoom, StartPoint));
	//StartPoint.Set(StartPointJson[0]->AsNumber(), StartPointJson[1]->AsNumber(), StartPointJson[2]->AsNumber());
	//
	//auto EndPointJson = JsonObject->GetArrayField(GET_MEMBER_NAME_STRING_CHECKED(ARoom, EndPoint));
	//EndPoint.Set(EndPointJson[0]->AsNumber(), EndPointJson[1]->AsNumber(), EndPointJson[2]->AsNumber());
	//
	//SetEndPoint(EndPoint);
	//bPlaced = true;

	return true;
}
