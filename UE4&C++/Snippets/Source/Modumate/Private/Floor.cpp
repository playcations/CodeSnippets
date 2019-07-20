// Fill out your copyright notice in the Description page of Project Settings.

#include "Floor.h"

#include "Serialization/JsonTypes.h"
#include "KismetMathLibrary.generated.h"
#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h"
#include "Components/TextRenderComponent.h"
#include "DimensionStringBase.h"


AFloor::AFloor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DimensionTextOffset(0.0f, 80.0f, 1.0f)
	, FixtureTextOffset(0.0f, 40.0f, 1.0f)
	//, DimensionLineColor(FColor::Transparent)
	, DimensionLineWeight(2.0f)
	, DimensionStringClass(ADimensionStringBase::StaticClass())
	, PreviewFixture(nullptr)
	, WidthDimensionString(nullptr)
	, HeightDimensionString(nullptr)
	, StartPoint(FVector::ZeroVector)
	, EndPoint(FVector::ZeroVector)
	, bPlaced(false)
	, OriginalBounds(EForceInit::ForceInitToZero)
	//, DimensionTextComponent(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	//DimensionTextComponent = CreateDefaultSubobject<UTextRenderComponent>(FName(TEXT("DimensionText")));
}


void AFloor::BeginPlay()
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



void AFloor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	FVector FloorDir = GetActorForwardVector();
	FVector FloorRight = GetActorRightVector();
	FVector FloorNormal = FVector::UpVector;
	FVector FloorDelta = EndPoint - StartPoint;
	float FloorLength = FloorDelta.Size();
	FVector FloorMidPoint = 0.5f * (StartPoint + EndPoint);
	float FloorLengthScale = GetActorRelativeScale3D().X;

	// layout the general dimensions
	FVector DimensionHeightOffset = DimensionTextOffset.Z * FloorNormal;
	/*
	DrawDebugLine(World, DimensionHeightOffset + StartPoint, DimensionHeightOffset + StartPoint + DimensionTextOffset.Y * 1.25f * FloorRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);
	DrawDebugLine(World, DimensionHeightOffset + EndPoint, DimensionHeightOffset + EndPoint + DimensionTextOffset.Y * 1.25f * FloorRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);
	DrawDebugLine(World, DimensionHeightOffset + StartPoint + DimensionTextOffset.Y * FloorRight, DimensionHeightOffset + EndPoint + DimensionTextOffset.Y * FloorRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);
	*/
	WidthDimensionString->SetDimensionString(DimensionHeightOffset + StartPoint, DimensionHeightOffset + EndPoint,
		DimensionHeightOffset + StartPoint + DimensionTextOffset.Y * 1.25f * FloorRight, DimensionHeightOffset + EndPoint + DimensionTextOffset.Y * 1.25f * FloorRight,
		FVector(0, 0, 0), FVector(0, 0, 0), FVector(0, 0, 0));

	HeightDimensionString->SetDimensionString(DimensionHeightOffset + StartPoint, DimensionHeightOffset + StartPoint + -25.f*FloorNormal,
		DimensionHeightOffset + StartPoint + -100.0f*FloorDir, DimensionHeightOffset + StartPoint + -100.0f*FloorDir + -25.f*FloorNormal,
		FVector(0, 0, 0), FVector(0, 0, 0), FVector(0, 0, 0));
	// layout the fixture dimensions
	if (SortedAttachedFixtures.Num() > 0)
	{
		int32 NumFixtures = SortedAttachedFixtures.Num();
		if (ensureAlways(2*NumFixtures == FixtureDimensionStrings.Num()))
		{
			FVector FixtureHeightOffset = FixtureTextOffset.Z * FloorNormal;

			for (int32 i = 0; i < NumFixtures; ++i)
			{
				AStaticMeshActor* Fixture = SortedAttachedFixtures[i];
				//UTextRenderComponent* FixtureText = AttachedFixtureDimensionTexts[i];

				FVector FixturePosInPlane = FVector::PointPlaneProject(Fixture->GetActorLocation(), StartPoint, FloorNormal);
				FVector FixtureLocalBounds = Fixture->GetActorTransform().InverseTransformVector(Fixture->GetComponentsBoundingBox(true).GetSize());
				FVector FixtureStartPos = FixturePosInPlane - 0.5f * FixtureLocalBounds.Y * FloorDir;
				float FixtureDist = (Fixture->GetRootComponent()->RelativeLocation.X * FloorLengthScale) + 0.5f * FloorLength;
				FVector TextPosition;

				if (i == 0)
				{
					// draw line from start point to this fixture's start
					DrawDebugLine(World, FixtureHeightOffset + StartPoint, FixtureHeightOffset + StartPoint + FixtureTextOffset.Y * 1.25f * FloorRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// out from Floor start
					DrawDebugLine(World, FixtureHeightOffset + FixtureStartPos, FixtureHeightOffset + FixtureStartPos + FixtureTextOffset.Y * 1.25f * FloorRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// out from fixture start
					DrawDebugLine(World, FixtureHeightOffset + StartPoint + FixtureTextOffset.Y * FloorRight, FixtureHeightOffset + FixtureStartPos + FixtureTextOffset.Y * FloorRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// between hatch marks

					TextPosition = 0.5f * (StartPoint + FixtureStartPos) + FixtureTextOffset.Y * FloorRight + FixtureTextOffset.Z * FloorNormal;
				}
				else
				{
					// draw line from previous fixture's start to this fixture's start
					AStaticMeshActor* PrevFixture = SortedAttachedFixtures[i - 1];
					FVector PrevFixturePosInPlane = FVector::PointPlaneProject(PrevFixture->GetActorLocation(), StartPoint, FloorNormal);
					FVector PrevFixtureLocalBounds = PrevFixture->GetActorTransform().InverseTransformVector(PrevFixture->GetComponentsBoundingBox(true).GetSize());
					FVector PrevFixtureStartPos = PrevFixturePosInPlane - 0.5f * PrevFixtureLocalBounds.Y * FloorDir;

					DrawDebugLine(World, FixtureHeightOffset + PrevFixtureStartPos, FixtureHeightOffset + PrevFixtureStartPos + FixtureTextOffset.Y * 1.25f * FloorRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// out from Floor start
					DrawDebugLine(World, FixtureHeightOffset + FixtureStartPos, FixtureHeightOffset + FixtureStartPos + FixtureTextOffset.Y * 1.25f * FloorRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// out from fixture start
					DrawDebugLine(World, FixtureHeightOffset + PrevFixtureStartPos + FixtureTextOffset.Y * FloorRight, FixtureHeightOffset + FixtureStartPos + FixtureTextOffset.Y * FloorRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// between hatch marks

					TextPosition = 0.5f * (PrevFixtureStartPos + FixtureStartPos) + FixtureTextOffset.Y * FloorRight + FixtureTextOffset.Z * FloorNormal;
				}

				//FQuat TextRot(FRotationMatrix::MakeFromXZ(FloorNormal, -FloorRight));
				//FixtureText->SetWorldLocationAndRotation(TextPosition, TextRot);
				//FixtureText->SetText(FString::Printf(TEXT("%.2f"), 0.01f * FixtureDist));
			}
		}
	}
}




void AFloor::SetEndPoint(const FVector& NewEndPoint)
{
	EndPoint = NewEndPoint;
	EndPoint.Z = StartPoint.Z;	
	FVector FloorNormal = FVector::UpVector;

	FVector FloorDelta = EndPoint - StartPoint;
	float FloorLength = FloorDelta.Size();
	if (FloorLength == 0.0f)
	{
		return;
	}

	FVector FloorDir = FloorDelta / FloorLength;
	FVector FloorRelScale = GetActorRelativeScale3D();

	FQuat FloorRot(FRotationMatrix::MakeFromXZ(FloorDir, FloorNormal));
	FVector FloorMidPoint = 0.5f * (StartPoint + EndPoint);

	FVector FloorScale(FloorLength / OriginalBounds.GetSize().X, FloorRelScale.Y, FloorRelScale.Z);

	SetActorTransform(FTransform(FloorRot, FloorMidPoint, FloorScale), false, nullptr, ETeleportType::TeleportPhysics);

	FVector FloorRight = FloorRot.GetRightVector();
	/*
	DimensionTextComponent->SetVisibility(true);
	FVector TextPosition = FloorMidPoint + DimensionTextOffset.Y * FloorRight + DimensionTextOffset.Z * FloorNormal;
	FQuat TextRot(FRotationMatrix::MakeFromXZ(FloorNormal, -FloorRight));
	DimensionTextComponent->SetWorldTransform(FTransform(TextRot, TextPosition));
	DimensionTextComponent->SetText(FText::FromString(FString::Printf(TEXT("%.2f"), 0.01f * FloorLength)));
	*/
}

void AFloor::AttachFixture(AStaticMeshActor* Fixture)
{
	Fixture->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	float DistanceAlongFloor = Fixture->GetRootComponent()->RelativeLocation.X * GetActorRelativeScale3D().X;

	UE_LOG(LogTemp, Warning, TEXT("Fixture %s dist along Floor: %.2f"), *Fixture->GetName(), DistanceAlongFloor);

	SortedAttachedFixtures.Add(Fixture);
	SortedAttachedFixtures.Sort([](AStaticMeshActor& A, AStaticMeshActor& B) {
		return A.GetRootComponent()->RelativeLocation.X < B.GetRootComponent()->RelativeLocation.X;
	});
	//Add a new dimension string to the world for the fixture.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FixtureDimensionStrings.Add(GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams));
	FixtureDimensionStrings.Add(GetWorld()->SpawnActor<ADimensionStringBase>(DimensionStringClass, FTransform::Identity, SpawnParams));
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

TSharedPtr<FJsonObject> AFloor::SerializeToJson() const
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());

	TArray<TSharedPtr<FJsonValue>> StartPointJson;
	StartPointJson.Add(MakeShareable(new FJsonValueNumber(StartPoint.X)));
	StartPointJson.Add(MakeShareable(new FJsonValueNumber(StartPoint.Y)));
	StartPointJson.Add(MakeShareable(new FJsonValueNumber(StartPoint.Z)));
	ResultJson->SetArrayField(GET_MEMBER_NAME_STRING_CHECKED(AFloor, StartPoint), StartPointJson);

	TArray<TSharedPtr<FJsonValue>> EndPointJson;
	EndPointJson.Add(MakeShareable(new FJsonValueNumber(EndPoint.X)));
	EndPointJson.Add(MakeShareable(new FJsonValueNumber(EndPoint.Y)));
	EndPointJson.Add(MakeShareable(new FJsonValueNumber(EndPoint.Z)));
	ResultJson->SetArrayField(GET_MEMBER_NAME_STRING_CHECKED(AFloor, EndPoint), EndPointJson);

	return ResultJson;
}

bool AFloor::DeserializeFromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	auto StartPointJson = JsonObject->GetArrayField(GET_MEMBER_NAME_STRING_CHECKED(AFloor, StartPoint));
	StartPoint.Set(StartPointJson[0]->AsNumber(), StartPointJson[1]->AsNumber(), StartPointJson[2]->AsNumber());

	auto EndPointJson = JsonObject->GetArrayField(GET_MEMBER_NAME_STRING_CHECKED(AFloor, EndPoint));
	EndPoint.Set(EndPointJson[0]->AsNumber(), EndPointJson[1]->AsNumber(), EndPointJson[2]->AsNumber());

	SetEndPoint(EndPoint);
	bPlaced = true;

	return true;
}
