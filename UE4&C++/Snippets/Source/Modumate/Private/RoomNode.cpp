// Fill out your copyright notice in the Description page of Project Settings.

#include "RoomNode.h"

#include "Serialization/JsonTypes.h"
#include "KismetMathLibrary.generated.h"
#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h"
#include "Components/TextRenderComponent.h"

#include "ModumateGameInstance.h"
#include "EditManager.h"
#include "Wall.h"


ARoomNode::ARoomNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bGrounded(false)
	, bVisited(false)
	, LeftChainedWall(nullptr)
	, RightChainedWall(nullptr)
	, LeftChainedNode(nullptr)
	, RightChainedNode(nullptr)
	, AssociatedWall(nullptr)
	, bConnectionsDirty(false)
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(USceneComponent::GetDefaultSceneRootVariableName());
}

void ARoomNode::BeginPlay()
{
	Super::BeginPlay();
}

void ARoomNode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool ARoomNode::ConnectWall(AWall* Wall, bool bAutoUpdate)
{
	UModumateGameInstance* ModGameInstance = Cast<UModumateGameInstance>(GetWorld()->GetGameInstance());
	UEditManager* EditManager = ModGameInstance ? ModGameInstance->EditManager : nullptr;

	if (EditManager)
	{
		FVector NodePos = GetActorLocation();
		bool bConnected = false;

		// TODO: actually adjust the wall's start and end positions, not just the node references, even though it's within epsilon
		if ((Wall->StartNode == this) || (Wall->EndNode == this))
		{
			bConnected = true;
		}
		else if (Wall->GetWallStart().Equals(NodePos, EditManager->RoomNodeEpsilon))
		{
			Wall->StartNode = this;
			bConnected = true;
		}
		else if (Wall->GetWallEnd().Equals(NodePos, EditManager->RoomNodeEpsilon))
		{
			Wall->EndNode = this;
			bConnected = true;
		}

		if (bConnected)
		{
			SortedWalls.Add(Wall);

			if (bAutoUpdate)
			{
				SortConnectedWalls();
			}
		}
	}

	return false;
}

void ARoomNode::SortConnectedWalls()
{
	WallIndexMap.Empty(SortedWalls.Num());

	SortedWalls.Sort([this](AWall& WallA, AWall& WallB) {
		FVector WallADir = WallA.GetWallDirFrom(this);
		FVector WallBDir = WallB.GetWallDirFrom(this);

		return FMath::Atan2(WallADir.Y, WallADir.X) < FMath::Atan2(WallBDir.Y, WallBDir.X);
	});

	for (int32 iWall = 0; iWall < SortedWalls.Num(); ++iWall)
	{
		AWall* Wall = SortedWalls[iWall];
		WallIndexMap.Emplace(Wall, iWall);
	}
}

bool ARoomNode::FindAdjacentWalls(const AWall* SourceWall, AWall*& LeftWall, AWall*& RightWall) const
{
	const int32* WallIndexPtr = WallIndexMap.Find(SourceWall);
	if (SourceWall && WallIndexPtr)
	{
		int32 WallIndex = *WallIndexPtr;
		int32 NumWalls = SortedWalls.Num();
		if (WallIndex >= 0 && WallIndex < NumWalls && NumWalls > 1)
		{
			RightWall = SortedWalls[(WallIndex + 1) % NumWalls];
			LeftWall = SortedWalls[(WallIndex + NumWalls - 1) % NumWalls];

			return true;
		}
	}

	return false;
}

bool ARoomNode::MergeNode(ARoomNode* NodeToMerge)
{
	UModumateGameInstance* ModGameInstance = Cast<UModumateGameInstance>(GetWorld()->GetGameInstance());
	UEditManager* EditManager = ModGameInstance ? ModGameInstance->EditManager : nullptr;
	bool bCanMerge = EditManager && NodeToMerge && (this != NodeToMerge) &&
		GetActorLocation().Equals(NodeToMerge->GetActorLocation(), EditManager->RoomNodeEpsilon);

	if (bCanMerge)
	{
		for (AWall* ConnectedWall : NodeToMerge->SortedWalls)
		{
			ConnectWall(ConnectedWall);
		}

		SortConnectedWalls();
		NodeToMerge->Destroy();
	}

	return bCanMerge;
}

static FString LocationJsonKey(TEXT("Location"));

TSharedPtr<FJsonObject> ARoomNode::SerializeToJson() const
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());
	FVector Location = GetActorLocation();

	TArray<TSharedPtr<FJsonValue>> LocationJson;
	LocationJson.Add(MakeShareable(new FJsonValueNumber(Location.X)));
	LocationJson.Add(MakeShareable(new FJsonValueNumber(Location.Y)));
	LocationJson.Add(MakeShareable(new FJsonValueNumber(Location.Z)));
	ResultJson->SetArrayField(LocationJsonKey, LocationJson);

	return ResultJson;
}

bool ARoomNode::DeserializeFromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	auto LocationJson = JsonObject->GetArrayField(LocationJsonKey);
	if (LocationJson.Num() == 3)
	{
		SetActorLocationAndRotation(FVector(LocationJson[0]->AsNumber(), LocationJson[1]->AsNumber(), LocationJson[2]->AsNumber()), FQuat::Identity);
		return true;
	}

	return false;
}
