// Fill out your copyright notice in the Description page of Project Settings.

#include "Wall.h"

#include "Serialization/JsonTypes.h"
#include "KismetMathLibrary.generated.h"
#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h"
#include "Components/TextRenderComponent.h"
#include "DimensionStringBase.h"
#include "ModumateGameInstance.h"
#include "EditManager.h"
#include "RoomNode.h"
#include "Room.h"


FWallIntersection::FWallIntersection()
{
	Init();
}

void FWallIntersection::Init()
{
	FMemory::Memzero(this, sizeof(FWallIntersection));
}


void FWallBox::AddVerts()
{
	if (wallVertices.Num() < 1)
	{
		wallVertices.Add(b_LeftSideStart);
		wallVertices.Add(b_RightSideStart);
		wallVertices.Add(b_RightSideEnd);
		wallVertices.Add(b_LeftSideEnd);
		wallVertices.Add(t_LeftSideStart);
		wallVertices.Add(t_RightSideStart);
		wallVertices.Add(t_RightSideEnd);
		wallVertices.Add(t_LeftSideEnd);
	}

}

AWall::AWall(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ID(-1)
	, DimensionTextOffset(0.0f, 120.0f, 1.0f)
	, FixtureTextOffset(0.0f, 75.0f, 1.0f)
	//, DimensionLineColor(FColor::Transparent)
	//, DimensionLineWeight(2.0f)
	, StartPoint(FVector::ZeroVector)
	, EndPoint(FVector::ZeroVector)
	, bPlaced(false)
	, bGrounded(false)
	, bVisited(false)
	, LeftChainedWall(nullptr)
	, RightChainedWall(nullptr)
	, LeftChainedNode(nullptr)
	, RightChainedNode(nullptr)
	, OriginalBounds(EForceInit::ForceInitToZero)
	//, DimensionTextComponent(nullptr)
	, DimensionStringClass(ADimensionStringBase::StaticClass())
	, WidthDimensionString(nullptr)
	, HeightDimensionString(nullptr)
	, SourceLeftWall(nullptr)
	, SourceRightWall(nullptr)
	, DestLeftWall(nullptr)
	, DestRightWall(nullptr)
	, LeftRoom(nullptr)
	, RightRoom(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	//DimensionTextComponent = CreateDefaultSubobject<UTextRenderComponent>(FName(TEXT("DimensionText")));

}

void AWall::BeginPlay()
{
	Super::BeginPlay();
	/*
	if (DimensionLineColor.A == 0)
	{
		DimensionLineColor = DimensionTextComponent->TextRenderColor;
	}
	*/

	for (const UActorComponent* ActorComponent : GetComponents())
	{
		const UPrimitiveComponent* PrimComp = Cast<const UPrimitiveComponent>(ActorComponent);
		if (PrimComp && PrimComp->IsRegistered() && PrimComp->IsCollisionEnabled())
		{
			OriginalBounds += PrimComp->Bounds.GetBox();
		}
	}

	StartPoint = GetActorLocation();
	EndPoint = StartPoint;


	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	WidthDimensionString = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
	HeightDimensionString = GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams);
}
void AWall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();

	FVector FloorNormal = FVector::UpVector;
	FVector WallDelta = EndPoint - StartPoint;
	float WallLength = WallDelta.Size();
	FVector WallMidPoint = 0.5f * (StartPoint + EndPoint);
	float WallLengthScale = GetActorRelativeScale3D().X;


	// layout the general dimensions
	FVector DimensionHeightOffset = DimensionTextOffset.Z * FloorNormal;

	/*
	DrawDebugLine(World, DimensionHeightOffset + StartPoint, DimensionHeightOffset + StartPoint + DimensionTextOffset.Y * 1.25f * WallRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);
	DrawDebugLine(World, DimensionHeightOffset + EndPoint, DimensionHeightOffset + EndPoint + DimensionTextOffset.Y * 1.25f * WallRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);
	DrawDebugLine(World, DimensionHeightOffset + StartPoint + DimensionTextOffset.Y * WallRight, DimensionHeightOffset + EndPoint + DimensionTextOffset.Y * WallRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);
	*/
	int ExtWallDirection = (bPlaced && RightRoom->IsInterior() && !LeftRoom->IsInterior()) ? -1 : 1;
	FVector DimStartPoint = (bPlaced && RightRoom->IsInterior() && !LeftRoom->IsInterior()) ? EndPoint : StartPoint;
	FVector DimEndPoint = (bPlaced && RightRoom->IsInterior() && !LeftRoom->IsInterior()) ? StartPoint : EndPoint;
	FVector WallDir = ExtWallDirection*GetActorForwardVector();
	FVector WallRight = ExtWallDirection*GetActorRightVector();
	//make the fixture strings start from the left if we flipped
	//TODO Change strings to start on the left if wall is flipped. Members need to be changed as well for the windows and doors to be seen.
	
	
	WidthDimensionString->SetDimensionString(DimensionHeightOffset + DimStartPoint, DimensionHeightOffset + DimEndPoint,
		DimensionHeightOffset + DimStartPoint + DimensionTextOffset.Y * 1.25f * WallRight,
		DimensionHeightOffset + DimEndPoint + DimensionTextOffset.Y * 1.25f * WallRight,
		-1 * ExtWallDirection * WallRight, -1 * ExtWallDirection * WallDir, FVector(0, 0, 1));

	HeightDimensionString->SetDimensionString(DimensionHeightOffset + DimStartPoint + 243.84f*FloorNormal, DimensionHeightOffset + DimStartPoint,
		DimensionHeightOffset + DimStartPoint + 243.84f*FloorNormal + -100.0f*WallDir, DimensionHeightOffset + DimStartPoint + -100.0f*WallDir,
		FVector(0, 0, 1), -1 * ExtWallDirection * WallDir, ExtWallDirection * WallRight);
	
	
	
	//GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Red, FString::FromInt(FixtureDimensionStrings.Num()));
	// layout the fixture dimensions
	if (SortedAttachedFixtures.Num() > 0)
	{
		int32 NumFixtures = SortedAttachedFixtures.Num();
		if (ensureAlways(4*NumFixtures == FixtureDimensionStrings.Num()))
		{
			FVector FixtureHeightOffset = FixtureTextOffset.Z * FloorNormal;
			int32 i;
			for (i = 0; i < NumFixtures; ++i)
			{
				AStaticMeshActor* Fixture = SortedAttachedFixtures[i];
				//UTextRenderComponent* FixtureText = FixtureDimensionStrings[i];
				//FVector FixturePosInPlane = FVector::PointPlaneProject(Fixture->GetActorLocation(), StartPoint, FloorNormal);
				//FVector FixtureLocalBounds = Fixture->GetActorTransform().InverseTransformVector(Fixture->GetComponentsBoundingBox(true).GetSize());
				//FVector FixtureStartPos = FixturePosInPlane - 1.0f * FixtureLocalBounds.Y * WallDir;
				FVector Origin;
				FVector Bounds;
				Fixture->GetActorBounds(false, Origin, Bounds);
				FVector LeftOfFixture = Origin - 1.0f * (93.98/2)* WallDir;
				FVector RightOfFixture = Origin + 1.0 * (93.98/2)* WallDir;
				FVector BottomOfFixture = LeftOfFixture - 1.0f * Bounds.Z * FloorNormal;
				FVector TopLeftOfFixture = LeftOfFixture + 1.0f * Bounds.Z * FloorNormal;
				FVector TopRightOfFixture = RightOfFixture + 1.0f * Bounds.Z * FloorNormal;
				//if the fixture's bottom of fixture is not coplanar with the wall plane, force it.
				BottomOfFixture = FVector::PointPlaneProject(BottomOfFixture, DimStartPoint, WallRight);
				//find the floor pos underneath the fixture
				FVector FixtureStartPos = FVector::PointPlaneProject(BottomOfFixture, DimStartPoint, FloorNormal);
				//if the fixture's start pos is not coplanar with the wall plane, force it.
				//FixtureStartPos = FVector::PointPlaneProject(FixtureStartPos, StartPoint, WallRight);
				
				
				//GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Green, Origin.ToString());
				//GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Blue, Bounds.ToString());
				//float FixtureDist = (Fixture->GetRootComponent()->RelativeLocation.X * WallLengthScale) + 0.5f * WallLength;

				FixtureDimensionStrings[i * 4 + 1]->SetDimensionString(FVector::PointPlaneProject(BottomOfFixture, DimStartPoint, WallRight) + (wallThickness * 2.5)*WallRight,
					FixtureStartPos + (wallThickness * 2.5)*WallRight,
					FVector::PointPlaneProject(BottomOfFixture, DimStartPoint, WallRight) + (wallThickness * 2.5)*WallRight - 20*WallDir,
					FixtureStartPos + (wallThickness * 2.5)*WallRight - 20*WallDir,
					FVector(0, 0, 1), -1 * WallDir, WallRight);
				FixtureDimensionStrings[i * 4 + 2]->SetDimensionString(FVector::PointPlaneProject(TopLeftOfFixture, DimStartPoint, WallRight) + (wallThickness * 2.5)*WallRight,
					FVector::PointPlaneProject(BottomOfFixture, DimStartPoint, WallRight) + (wallThickness * 2.5)*WallRight,
					FVector::PointPlaneProject(TopLeftOfFixture, DimStartPoint, WallRight) + (wallThickness * 2.5)*WallRight - 20*WallDir,
					FVector::PointPlaneProject(BottomOfFixture, DimStartPoint, WallRight) + (wallThickness * 2.5)*WallRight - 20*WallDir,
					FVector(0, 0, 1), -1 * WallDir, WallRight);
				FixtureDimensionStrings[i * 4 + 3]->SetDimensionString(FVector::PointPlaneProject(LeftOfFixture, DimStartPoint, WallRight) + (wallThickness * 2.5)*WallRight,
					FVector::PointPlaneProject(RightOfFixture, DimStartPoint, WallRight) + (wallThickness * 2.5)*WallRight,
					FVector::PointPlaneProject(TopLeftOfFixture, DimStartPoint, WallRight) + (wallThickness * 2.5)*WallRight + 20*FloorNormal,
					FVector::PointPlaneProject(TopRightOfFixture, DimStartPoint, WallRight) + (wallThickness * 2.5)*WallRight + 20*FloorNormal,
					FVector(0, 0, 1), -1 * WallDir, WallRight);
				
				//GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Red, FixtureStartPos.ToString());
				 //GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Red, FString::FromInt(i));
				//GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Red, FString::FromInt(I));
				
				if (i == 0)
				{
					// draw line from start point to this fixture's start
					/*
					DrawDebugLine(World, FixtureHeightOffset + StartPoint, FixtureHeightOffset + StartPoint + FixtureTextOffset.Y * 1.25f * WallRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// out from wall start
					DrawDebugLine(World, FixtureHeightOffset + FixtureStartPos, FixtureHeightOffset + FixtureStartPos + FixtureTextOffset.Y * 1.25f * WallRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// out from fixture start
					DrawDebugLine(World, FixtureHeightOffset + StartPoint + FixtureTextOffset.Y * WallRight, FixtureHeightOffset + FixtureStartPos + FixtureTextOffset.Y * WallRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// between hatch marks

					TextPosition = 0.5f * (StartPoint + FixtureStartPos) + FixtureTextOffset.Y * WallRight + FixtureTextOffset.Z * FloorNormal;
					*/
					FixtureDimensionStrings[0]->SetDimensionString(FixtureHeightOffset + DimStartPoint, FixtureHeightOffset + FixtureStartPos,
						FixtureHeightOffset + DimStartPoint + FixtureTextOffset.Y * 1.25f * WallRight, 
						FixtureHeightOffset + FixtureStartPos + FixtureTextOffset.Y * 1.25f * WallRight,
						-1* ExtWallDirection*WallRight, -1* ExtWallDirection*WallDir, FVector(0, 0, 1));
					
				}
				else
				{
					// draw line from previous fixture's start to this fixture's start
					AStaticMeshActor* PrevFixture = SortedAttachedFixtures[i - 1];
					FVector PrevOrigin;
					FVector PrevBounds;
					PrevFixture->GetActorBounds(false, PrevOrigin, PrevBounds);
					FVector PrevLeftOfFixture = PrevOrigin - 1.0f * (93.98 / 2) * WallDir;
					FVector PrevBottomOfFixture = PrevLeftOfFixture - 1.0f * PrevBounds.Z * FloorNormal;
					//FVector PrevFixturePosInPlane = FVector::PointPlaneProject(PrevFixture->GetActorLocation(), StartPoint, FloorNormal);
					//FVector PrevFixtureLocalBounds = PrevFixture->GetActorTransform().InverseTransformVector(PrevFixture->GetComponentsBoundingBox(true).GetSize());
					//FVector PrevFixtureStartPos = PrevFixturePosInPlane -1.0f * PrevFixtureLocalBounds.Y * WallDir;
					FVector PrevFixtureStartPos = FVector::PointPlaneProject(PrevBottomOfFixture, DimStartPoint, FloorNormal);
					//if the fixture's start pos is not coplanar with the wall plane, force it.
					PrevFixtureStartPos = FVector::PointPlaneProject(PrevFixtureStartPos, DimStartPoint, WallRight);
					

					/*
					DrawDebugLine(World, FixtureHeightOffset + PrevFixtureStartPos, FixtureHeightOffset + PrevFixtureStartPos + FixtureTextOffset.Y * 1.25f * WallRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// out from wall start
					DrawDebugLine(World, FixtureHeightOffset + FixtureStartPos, FixtureHeightOffset + FixtureStartPos + FixtureTextOffset.Y * 1.25f * WallRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// out from fixture start
					DrawDebugLine(World, FixtureHeightOffset + PrevFixtureStartPos + FixtureTextOffset.Y * WallRight, FixtureHeightOffset + FixtureStartPos + FixtureTextOffset.Y * WallRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// between hatch marks

					TextPosition = 0.5f * (PrevFixtureStartPos + FixtureStartPos) + FixtureTextOffset.Y * WallRight + FixtureTextOffset.Z * FloorNormal;
					*/
					FixtureDimensionStrings[i * 4]->SetDimensionString(FixtureHeightOffset + PrevFixtureStartPos, FixtureHeightOffset + FixtureStartPos,
						FixtureHeightOffset + PrevFixtureStartPos + FixtureTextOffset.Y * 1.25f * WallRight,
						FixtureHeightOffset + FixtureStartPos + FixtureTextOffset.Y * 1.25f * WallRight,
						-1 * ExtWallDirection*WallRight, -1 * ExtWallDirection*WallDir, FVector(0, 0, 1));
					
					//GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Blue, PrevFixtureStartPos.ToString());
					//GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Blue, SortedAttachedFixtures[i - 1] == SortedAttachedFixtures[i] ? TEXT("True") : TEXT("False"));
				}
				/*
				FQuat TextRot(FRotationMatrix::MakeFromXZ(FloorNormal, -WallRight));
				FixtureText->SetWorldLocationAndRotation(TextPosition, TextRot);
				FixtureText->SetText(FText::FromString(FString::Printf(TEXT("%.2f"), 0.01f * FixtureDist)));
				*/
			}
		}
	}
}

FVector AWall::GetWallStart() const
{
	return StartNode ? StartNode->GetActorLocation() : GetActorLocation();
}

FVector AWall::GetWallEnd() const
{
	return EndNode ? EndNode->GetActorLocation() : GetActorLocation();
}

FVector AWall::GetWallDir() const
{
	return GetActorForwardVector();
}

FVector AWall::GetWallDirFrom(ARoomNode* Node) const
{
	if (Node)
	{
		bool bComesFrom = (StartNode == Node);
		bool bGoesTo = (EndNode == Node);
		if (ensureAlwaysMsgf(bComesFrom != bGoesTo, TEXT("Wall %s isn't connected to node %node"), *GetName(), *Node->GetName()))
		{
			FVector WallDir = GetWallDir();
			if (bGoesTo)
			{
				WallDir *= -1.0f;
			}

			return WallDir;
		}
	}
	
	return FVector::ZeroVector;
}

void AWall::SetEndPoint(const FVector& NewEndPoint)
{
	EndPoint = NewEndPoint;
	EndPoint.Z = StartPoint.Z;	// for now, assume walls will be on flat floor

	if (EndNode)
	{
		// for now, assume that we have ownership over EndNode if we are moving the wall
		// if this isn't the case then we need to split the node and create a new one for this wall
		ensure(EndNode->SortedWalls.Num() == 1 && EndNode->SortedWalls[0] == this);
		EndNode->SetActorLocation(EndPoint);
	}

	FVector FloorNormal = FVector::UpVector;

	FVector WallDelta = EndPoint - StartPoint;
	float WallLength = WallDelta.Size();
	if (WallLength == 0.0f)
	{
		return;
	}

	FVector WallDir = WallDelta / WallLength;
	FVector WallRelScale = GetActorRelativeScale3D();

	FQuat WallRot(FRotationMatrix::MakeFromXZ(WallDir, FloorNormal));
	FVector WallMidPoint = 0.5f * (StartPoint + EndPoint);

	FVector WallScale(WallLength / OriginalBounds.GetSize().X, WallRelScale.Y, WallRelScale.Z);

	SetActorTransform(FTransform(WallRot, WallMidPoint, WallScale), false, nullptr, ETeleportType::TeleportPhysics);

	


	FVector WallRight = WallRot.GetRightVector();
	//DimensionTextComponent->SetVisibility(true);
	FVector TextPosition = WallMidPoint + DimensionTextOffset.Y * WallRight + DimensionTextOffset.Z * FloorNormal;
	FQuat TextRot(FRotationMatrix::MakeFromXZ(FloorNormal, -WallRight));


	//DimensionTextComponent->SetWorldTransform(FTransform(TextRot, TextPosition));
	//DimensionTextComponent->SetText(FText::FromString(FString::Printf(TEXT("%.2f"), 0.01f * WallLength)));
}

void AWall::AttachFixture(AStaticMeshActor* Fixture)
{
	
	if (PreviewFixture != nullptr)
	{
		int32 PrevPos = SortedAttachedFixtures.Find(PreviewFixture);
		if (PrevPos != INDEX_NONE)
		{
			SortedAttachedFixtures[PrevPos] = Fixture;
			SortedAttachedFixtures[PrevPos]->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
			PreviewFixture = nullptr;
		}
		
	}
	/*
	Fixture->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	float DistanceAlongWall = Fixture->GetRootComponent()->RelativeLocation.X * GetActorRelativeScale3D().X;

	UE_LOG(LogTemp, Warning, TEXT("Fixture %s dist along wall: %.2f"), *Fixture->GetName(), DistanceAlongWall);

	SortedAttachedFixtures.Add(Fixture);
	SortedAttachedFixtures.Sort([](AStaticMeshActor& A, AStaticMeshActor& B) {
		return A.GetRootComponent()->RelativeLocation.X < B.GetRootComponent()->RelativeLocation.X;
	});
	//Add a new dimension string to the world for the fixture.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FixtureDimensionStrings.Add(GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams));
	FixtureDimensionStrings.Add(GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams));
	*/
	// make text for the new fixture
	/*
	UTextRenderComponent* NewFixtureText = NewObject<UTextRenderComponent>(this);
	NewFixtureText->SetTextMaterial(DimensionTextComponent->TextMaterial);
	NewFixtureText->SetFont(DimensionTextComponent->Font);
	NewFixtureText->SetHorizontalAlignment(DimensionTextComponent->HorizontalAlignment);
	NewFixtureText->SetVerticalAlignment(DimensionTextComponent->VerticalAlignment);
	NewFixtureText->SetTextRenderColor(DimensionTextComponent->TextRenderColor);
	NewFixtureText->SetXScale(DimensionTextComponent->XScale);
	NewFixtureText->SetYScale(DimensionTextComponent->YScale);
	NewFixtureText->SetWorldSize(DimensionTextComponent->WorldSize);
	NewFixtureText->RegisterComponent();
	NewFixtureText->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	AttachedFixtureDimensionTexts.Add(NewFixtureText);
	*/
}

void AWall::UpdatePreviewFixture(AStaticMeshActor* Fixture)
{
	if (PreviewFixture == nullptr)
	{
		PreviewFixture = Fixture;
		PreviewFixture->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		float DistanceAlongWall = PreviewFixture->GetRootComponent()->RelativeLocation.X * GetActorRelativeScale3D().X;

		UE_LOG(LogTemp, Warning, TEXT("Fixture %s dist along wall: %.2f"), *PreviewFixture->GetName(), DistanceAlongWall);
		SortedAttachedFixtures.Add(PreviewFixture);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FixtureDimensionStrings.Add(GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams));
		FixtureDimensionStrings.Add(GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams));
		FixtureDimensionStrings.Add(GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams));
		FixtureDimensionStrings.Add(GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams));
	}
	
	//check if there's a ccw room.
	if (RightRoom->IsInterior() && !LeftRoom->IsInterior())
	{
		SortedAttachedFixtures.Sort([](AStaticMeshActor& A, AStaticMeshActor& B) {
			return -1 * A.GetRootComponent()->RelativeLocation.X < -1 * B.GetRootComponent()->RelativeLocation.X;
		});
	}
	else
	{
		SortedAttachedFixtures.Sort([](AStaticMeshActor& A, AStaticMeshActor& B) {
			return A.GetRootComponent()->RelativeLocation.X < B.GetRootComponent()->RelativeLocation.X;
		});
	}
	
	/*
	for (int32 i = 0; i < SortedAttachedFixtures.Num(); i++)
	{
		FString Output;
		Output += FString(" ") + FString::SanitizeFloat(SortedAttachedFixtures[i]->GetRootComponent()->RelativeLocation.X);
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Red, Output);
	}
	*/
	/*
	if (PreviewFixture)
		GEngine->AddOnScreenDebugMessage(0, 0.5f, FColor::Green, FString::SanitizeFloat(PreviewFixture->GetRootComponent()->RelativeLocation.X * GetActorRelativeScale3D().X));
		*/
	/*
	Fixture->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	float DistanceAlongWall = Fixture->GetRootComponent()->RelativeLocation.X * GetActorRelativeScale3D().X;

	UE_LOG(LogTemp, Warning, TEXT("Fixture %s dist along wall: %.2f"), *Fixture->GetName(), DistanceAlongWall);

	SortedAttachedFixtures.Add(Fixture);
	SortedAttachedFixtures.Sort([](AStaticMeshActor& A, AStaticMeshActor& B) {
	return A.GetRootComponent()->RelativeLocation.X < B.GetRootComponent()->RelativeLocation.X;
	});
	//Add a new dimension string to the world for the fixture.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FixtureDimensionStrings.Add(GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams));
	FixtureDimensionStrings.Add(GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams));
	*/
	
}

void AWall::RemovePrevDimStrings()
{
	if (PreviewFixture != nullptr)
	{
		int32 PrevPos = SortedAttachedFixtures.Find(PreviewFixture);

		if (PrevPos != INDEX_NONE)
		{
			SortedAttachedFixtures[PrevPos]->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			SortedAttachedFixtures.RemoveAt(PrevPos);
			
			FixtureDimensionStrings.Pop(true)->Destroy();
			FixtureDimensionStrings.Pop(true)->Destroy();
			FixtureDimensionStrings.Pop(true)->Destroy();
			FixtureDimensionStrings.Pop(true)->Destroy();
			PreviewFixture = nullptr;
		}
	}
	
}

bool AWall::SortConnectedWalls()
{
	bool bChanged = false;
	if (FindLeftAndRightWalls(true, SourceLeftWall, SourceRightWall))
	{
		bChanged = true;
	}
	if (FindLeftAndRightWalls(false, DestRightWall, DestLeftWall))
	{
		bChanged = true;
	}

	if (bChanged)
	{
		DebugDrawConnectedWalls();
	}

	return bChanged;
}

void AWall::DebugDrawConnectedWalls()
{
	if (UWorld* World = GetWorld())
	{
		UE_LOG(LogTemp, Log, TEXT("%s's src_l: %s, src_r: %s, dst_l: %s, dst_r: %s"), *GetName(),
			SourceLeftWall ? *SourceLeftWall->GetName() : TEXT("NULL"), SourceRightWall ? *SourceRightWall->GetName() : TEXT("NULL"),
			DestLeftWall ? *DestLeftWall->GetName() : TEXT("NULL"), DestRightWall ? *DestRightWall->GetName() : TEXT("NULL"));

		static FVector WallArrowOffset(0.0f, 0.0f, 205.0f);
		static float WallArrowThickness = 3.0f;

		DrawDebugDirectionalArrow(World, StartPoint + WallArrowOffset, EndPoint + WallArrowOffset, 10.0f, FColor::Cyan, true, 2.0f, 0, WallArrowThickness);

		static FVector ConnectedArrowOffset(0.0f, 0.0f, 210.0f);
		static float ConnectedArrowThickness = 5.0f;
		if (SourceLeftWall)
		{
			FVector SourceLeftEnd = SourceLeftWall->StartNode->IsConnectedToWall(this) ? SourceLeftWall->EndPoint : SourceLeftWall->StartPoint;
			DrawDebugDirectionalArrow(World, StartPoint + ConnectedArrowOffset, 0.75f * StartPoint + 0.25f * SourceLeftEnd + ConnectedArrowOffset, 10.0f, FColor::Green, true, 2.0f, 0, ConnectedArrowThickness);
		}
		if (SourceRightWall)
		{
			FVector SourceRightEnd = SourceRightWall->StartNode->IsConnectedToWall(this) ? SourceRightWall->EndPoint : SourceRightWall->StartPoint;
			DrawDebugDirectionalArrow(World, StartPoint + ConnectedArrowOffset, 0.75f * StartPoint + 0.25f * SourceRightEnd + ConnectedArrowOffset, 10.0f, FColor::Green, true, 2.0f, 0, ConnectedArrowThickness);
		}
		if (DestLeftWall)
		{
			FVector DestLeftEnd = DestLeftWall->StartNode->IsConnectedToWall(this) ? DestLeftWall->EndPoint : DestLeftWall->StartPoint;
			DrawDebugDirectionalArrow(World, EndPoint + ConnectedArrowOffset, 0.75f * EndPoint + 0.25f * DestLeftEnd + ConnectedArrowOffset, 10.0f, FColor::Red, true, 2.0f, 0, ConnectedArrowThickness);
		}
		if (DestRightWall)
		{
			FVector DestRightEnd = DestRightWall->StartNode->IsConnectedToWall(this) ? DestRightWall->EndPoint : DestRightWall->StartPoint;
			DrawDebugDirectionalArrow(World, EndPoint + ConnectedArrowOffset, 0.75f * EndPoint + 0.25f * DestRightEnd + ConnectedArrowOffset, 10.0f, FColor::Red, true, 2.0f, 0, ConnectedArrowThickness);
		}
	}
}

bool AWall::GetWallIntersection2D(AWall* OtherWall, FWallIntersection& WallIntersection, float Epsilon) const
{
	WallIntersection.Init();

	FVector ThisDelta = EndPoint - StartPoint;
	ThisDelta.Z = 0.0f;

	const FVector& OtherStart = OtherWall->StartPoint;
	const FVector& OtherEnd = OtherWall->EndPoint;
	FVector OtherDelta = OtherEnd - OtherStart;
	OtherDelta.Z = 0.0f;

	float ThisLength = ThisDelta.Size();
	float OtherLength = OtherDelta.Size();
	if (FMath::IsNearlyZero(ThisLength, Epsilon) || FMath::IsNearlyZero(OtherLength, Epsilon))
	{
		return false;
	}

	FVector ThisDir = ThisDelta / ThisLength;
	FVector OtherDir = OtherDelta / OtherLength;
	if (FVector::Parallel(ThisDir, OtherDir, 1.0f - Epsilon))
	{
		return false;
	}

	FMatrix2x2 DenominatorMatrix(-ThisDelta.X, -ThisDelta.Y, -OtherDelta.X, -OtherDelta.Y);
	float Denominator = DenominatorMatrix.Determinant();
	if (FMath::IsNearlyZero(Denominator, Epsilon))
	{
		return false;
	}

	FMatrix2x2 InnerNumeratorMatrixThis(StartPoint.X, StartPoint.Y, EndPoint.X, EndPoint.Y);
	FMatrix2x2 InnerNumeratorMatrixOther(OtherStart.X, OtherStart.Y, OtherEnd.X, OtherEnd.Y);
	float InnerNumeratorThis = InnerNumeratorMatrixThis.Determinant();
	float InnerNumeratorOther = InnerNumeratorMatrixOther.Determinant();

	FMatrix2x2 XNumeratorMatrix(InnerNumeratorThis, -ThisDelta.X, InnerNumeratorOther, -OtherDelta.X);
	FMatrix2x2 YNumeratorMatrix(InnerNumeratorThis, -ThisDelta.Y, InnerNumeratorOther, -OtherDelta.Y);
	float XNumerator = XNumeratorMatrix.Determinant();
	float YNumerator = YNumeratorMatrix.Determinant();

	WallIntersection.Location.Set(XNumerator / Denominator, YNumerator / Denominator, 0.0f);

	FVector ThisToIntersection = WallIntersection.Location - StartPoint;
	ThisToIntersection.Z = 0.0f;
	WallIntersection.DistAlongQueryWall = ThisToIntersection | ThisDir;
	FVector ThisIntersection = StartPoint + WallIntersection.DistAlongQueryWall * ThisDir;

	FVector OtherToIntersection = WallIntersection.Location - OtherStart;
	OtherToIntersection.Z = 0.0f;
	WallIntersection.DistAlongHitWall = OtherToIntersection | OtherDir;
	FVector OtherIntersection = OtherStart + WallIntersection.DistAlongHitWall * OtherDir;

	WallIntersection.Location.Z = 0.5f * (ThisIntersection.Z + OtherIntersection.Z);

	float DistPctAlongThis = WallIntersection.DistAlongQueryWall / ThisLength;
	float DistPctAlongOther = WallIntersection.DistAlongHitWall / OtherLength;

	bool bHit = (DistPctAlongThis >= Epsilon && DistPctAlongThis <= (1.0f - Epsilon) &&
				DistPctAlongOther >= Epsilon && DistPctAlongOther <= (1.0f - Epsilon));

	WallIntersection.HitWall = bHit ? OtherWall : nullptr;
	return bHit;
}

TSharedPtr<FJsonObject> AWall::SerializeToJson() const
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());

	TArray<TSharedPtr<FJsonValue>> StartPointJson;
	StartPointJson.Add(MakeShareable(new FJsonValueNumber(StartPoint.X)));
	StartPointJson.Add(MakeShareable(new FJsonValueNumber(StartPoint.Y)));
	StartPointJson.Add(MakeShareable(new FJsonValueNumber(StartPoint.Z)));
	ResultJson->SetArrayField(GET_MEMBER_NAME_STRING_CHECKED(AWall, StartPoint), StartPointJson);

	TArray<TSharedPtr<FJsonValue>> EndPointJson;
	EndPointJson.Add(MakeShareable(new FJsonValueNumber(EndPoint.X)));
	EndPointJson.Add(MakeShareable(new FJsonValueNumber(EndPoint.Y)));
	EndPointJson.Add(MakeShareable(new FJsonValueNumber(EndPoint.Z)));
	ResultJson->SetArrayField(GET_MEMBER_NAME_STRING_CHECKED(AWall, EndPoint), EndPointJson);

	return ResultJson;
}

bool AWall::DeserializeFromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	auto StartPointJson = JsonObject->GetArrayField(GET_MEMBER_NAME_STRING_CHECKED(AWall, StartPoint));
	StartPoint.Set(StartPointJson[0]->AsNumber(), StartPointJson[1]->AsNumber(), StartPointJson[2]->AsNumber());

	auto EndPointJson = JsonObject->GetArrayField(GET_MEMBER_NAME_STRING_CHECKED(AWall, EndPoint));
	EndPoint.Set(EndPointJson[0]->AsNumber(), EndPointJson[1]->AsNumber(), EndPointJson[2]->AsNumber());

	SetEndPoint(EndPoint);
	bPlaced = true;

	return true;
}

bool AWall::FindLeftAndRightWalls(bool bForward, AWall*& LeftWall, AWall*& RightWall)
{
	AWall* NewLeftWall = nullptr;
	AWall* NewRightWall = nullptr;
	ARoomNode* AdjacentNode = bForward ? StartNode : EndNode;

	if (ensureAlways(AdjacentNode) && AdjacentNode->FindAdjacentWalls(this, NewLeftWall, NewRightWall))
	{
		if ((LeftWall != NewLeftWall) || (RightWall != NewRightWall))
		{
			LeftWall = NewLeftWall;
			RightWall = NewRightWall;
			return true;
		}
	}

	return false;
}
