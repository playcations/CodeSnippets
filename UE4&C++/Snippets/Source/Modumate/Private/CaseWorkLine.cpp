// Fill out your copyright notice in the Description page of Project Settings.

#include "CaseWorkLine.h"

#include "Serialization/JsonTypes.h"
#include "KismetMathLibrary.generated.h"
#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h"
#include "Components/TextRenderComponent.h"



ACaseWorkLine::ACaseWorkLine(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DimensionTextOffset(0.0f, 80.0f, 1.0f)
	, FixtureTextOffset(0.0f, 40.0f, 1.0f)
	, DimensionLineColor(FColor::Transparent)
	, DimensionLineWeight(2.0f)
	, StartPoint(FVector::ZeroVector)
	, EndPoint(FVector::ZeroVector)
	, bPlaced(false)
	, OriginalBounds(EForceInit::ForceInitToZero)
	, DimensionTextComponent(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	DimensionTextComponent = CreateDefaultSubobject<UTextRenderComponent>(FName(TEXT("DimensionText")));
}


void ACaseWorkLine::BeginPlay()
{
	Super::BeginPlay();

	if (DimensionLineColor.A == 0)
	{
		DimensionLineColor = DimensionTextComponent->TextRenderColor;
	}


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
}



void ACaseWorkLine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	FVector CaseWorkLineDir = GetActorForwardVector();
	FVector CaseWorkLineRight = GetActorRightVector();
	FVector CaseWorkLineNormal = FVector::UpVector;
	FVector CaseWorkLineDelta = EndPoint - StartPoint;
	float CaseWorkLineLength = CaseWorkLineDelta.Size();
	FVector CaseWorkLineMidPoint = 0.5f * (StartPoint + EndPoint);
	float CaseWorkLineLengthScale = GetActorRelativeScale3D().X;

	// layout the general dimensions
	FVector DimensionHeightOffset = DimensionTextOffset.Z * CaseWorkLineNormal;

	DrawDebugLine(World, DimensionHeightOffset + StartPoint, DimensionHeightOffset + StartPoint + DimensionTextOffset.Y * 1.25f * CaseWorkLineRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);
	DrawDebugLine(World, DimensionHeightOffset + EndPoint, DimensionHeightOffset + EndPoint + DimensionTextOffset.Y * 1.25f * CaseWorkLineRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);
	DrawDebugLine(World, DimensionHeightOffset + StartPoint + DimensionTextOffset.Y * CaseWorkLineRight, DimensionHeightOffset + EndPoint + DimensionTextOffset.Y * CaseWorkLineRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);

	// layout the fixture dimensions
	if (SortedAttachedFixtures.Num() > 0)
	{
		int32 NumFixtures = SortedAttachedFixtures.Num();
		if (ensureAlways(NumFixtures == AttachedFixtureDimensionTexts.Num()))
		{
			FVector FixtureHeightOffset = FixtureTextOffset.Z * CaseWorkLineNormal;

			for (int32 i = 0; i < NumFixtures; ++i)
			{
				AStaticMeshActor* Fixture = SortedAttachedFixtures[i];
				UTextRenderComponent* FixtureText = AttachedFixtureDimensionTexts[i];

				FVector FixturePosInPlane = FVector::PointPlaneProject(Fixture->GetActorLocation(), StartPoint, CaseWorkLineNormal);
				FVector FixtureLocalBounds = Fixture->GetActorTransform().InverseTransformVector(Fixture->GetComponentsBoundingBox(true).GetSize());
				FVector FixtureStartPos = FixturePosInPlane - 0.5f * FixtureLocalBounds.Y * CaseWorkLineDir;
				float FixtureDist = (Fixture->GetRootComponent()->RelativeLocation.X * CaseWorkLineLengthScale) + 0.5f * CaseWorkLineLength;
				FVector TextPosition;

				if (i == 0)
				{
					// draw line from start point to this fixture's start
					DrawDebugLine(World, FixtureHeightOffset + StartPoint, FixtureHeightOffset + StartPoint + FixtureTextOffset.Y * 1.25f * CaseWorkLineRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// out from CaseWorkLine start
					DrawDebugLine(World, FixtureHeightOffset + FixtureStartPos, FixtureHeightOffset + FixtureStartPos + FixtureTextOffset.Y * 1.25f * CaseWorkLineRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// out from fixture start
					DrawDebugLine(World, FixtureHeightOffset + StartPoint + FixtureTextOffset.Y * CaseWorkLineRight, FixtureHeightOffset + FixtureStartPos + FixtureTextOffset.Y * CaseWorkLineRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// between hatch marks

					TextPosition = 0.5f * (StartPoint + FixtureStartPos) + FixtureTextOffset.Y * CaseWorkLineRight + FixtureTextOffset.Z * CaseWorkLineNormal;
				}
				else
				{
					// draw line from previous fixture's start to this fixture's start
					AStaticMeshActor* PrevFixture = SortedAttachedFixtures[i - 1];
					FVector PrevFixturePosInPlane = FVector::PointPlaneProject(PrevFixture->GetActorLocation(), StartPoint, CaseWorkLineNormal);
					FVector PrevFixtureLocalBounds = PrevFixture->GetActorTransform().InverseTransformVector(PrevFixture->GetComponentsBoundingBox(true).GetSize());
					FVector PrevFixtureStartPos = PrevFixturePosInPlane - 0.5f * PrevFixtureLocalBounds.Y * CaseWorkLineDir;

					DrawDebugLine(World, FixtureHeightOffset + PrevFixtureStartPos, FixtureHeightOffset + PrevFixtureStartPos + FixtureTextOffset.Y * 1.25f * CaseWorkLineRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// out from CaseWorkLine start
					DrawDebugLine(World, FixtureHeightOffset + FixtureStartPos, FixtureHeightOffset + FixtureStartPos + FixtureTextOffset.Y * 1.25f * CaseWorkLineRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// out from fixture start
					DrawDebugLine(World, FixtureHeightOffset + PrevFixtureStartPos + FixtureTextOffset.Y * CaseWorkLineRight, FixtureHeightOffset + FixtureStartPos + FixtureTextOffset.Y * CaseWorkLineRight, DimensionLineColor, false, -1.0f, 0, DimensionLineWeight);	// between hatch marks

					TextPosition = 0.5f * (PrevFixtureStartPos + FixtureStartPos) + FixtureTextOffset.Y * CaseWorkLineRight + FixtureTextOffset.Z * CaseWorkLineNormal;
				}

				FQuat TextRot(FRotationMatrix::MakeFromXZ(CaseWorkLineNormal, -CaseWorkLineRight));
				FixtureText->SetWorldLocationAndRotation(TextPosition, TextRot);
				FixtureText->SetText(FString::Printf(TEXT("%.2f"), 0.01f * FixtureDist));
			}
		}
	}
}




void ACaseWorkLine::SetEndPoint(const FVector& NewEndPoint)
{
	EndPoint = NewEndPoint;
	EndPoint.Z = StartPoint.Z;	
	FVector CaseWorkLineNormal = FVector::UpVector;

	FVector CaseWorkLineDelta = EndPoint - StartPoint;
	float CaseWorkLineLength = CaseWorkLineDelta.Size();
	if (CaseWorkLineLength == 0.0f)
	{
		return;
	}

	FVector CaseWorkLineDir = CaseWorkLineDelta / CaseWorkLineLength;
	FVector CaseWorkLineRelScale = GetActorRelativeScale3D();

	FQuat CaseWorkLineRot(FRotationMatrix::MakeFromXZ(CaseWorkLineDir, CaseWorkLineNormal));
	FVector CaseWorkLineMidPoint = 0.5f * (StartPoint + EndPoint);

	FVector CaseWorkLineScale(CaseWorkLineLength / OriginalBounds.GetSize().X, CaseWorkLineRelScale.Y, CaseWorkLineRelScale.Z);

	SetActorTransform(FTransform(CaseWorkLineRot, CaseWorkLineMidPoint, CaseWorkLineScale), false, nullptr, ETeleportType::TeleportPhysics);

	FVector CaseWorkLineRight = CaseWorkLineRot.GetRightVector();
	DimensionTextComponent->SetVisibility(true);
	FVector TextPosition = CaseWorkLineMidPoint + DimensionTextOffset.Y * CaseWorkLineRight + DimensionTextOffset.Z * CaseWorkLineNormal;
	FQuat TextRot(FRotationMatrix::MakeFromXZ(CaseWorkLineNormal, -CaseWorkLineRight));
	DimensionTextComponent->SetWorldTransform(FTransform(TextRot, TextPosition));
	DimensionTextComponent->SetText(FText::FromString(FString::Printf(TEXT("%.2f"), 0.01f * CaseWorkLineLength)));
}

void ACaseWorkLine::AttachFixture(AStaticMeshActor* Fixture)
{
	Fixture->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	float DistanceAlongCaseWorkLine = Fixture->GetRootComponent()->RelativeLocation.X * GetActorRelativeScale3D().X;

	UE_LOG(LogTemp, Warning, TEXT("Fixture %s dist along CaseWorkLine: %.2f"), *Fixture->GetName(), DistanceAlongCaseWorkLine);

	SortedAttachedFixtures.Add(Fixture);
	SortedAttachedFixtures.Sort([](AStaticMeshActor& A, AStaticMeshActor& B) {
		return A.GetRootComponent()->RelativeLocation.X < B.GetRootComponent()->RelativeLocation.X;
	});

	// make text for the new fixture
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
}

TSharedPtr<FJsonObject> ACaseWorkLine::SerializeToJson() const
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());

	TArray<TSharedPtr<FJsonValue>> StartPointJson;
	StartPointJson.Add(MakeShareable(new FJsonValueNumber(StartPoint.X)));
	StartPointJson.Add(MakeShareable(new FJsonValueNumber(StartPoint.Y)));
	StartPointJson.Add(MakeShareable(new FJsonValueNumber(StartPoint.Z)));
	ResultJson->SetArrayField(GET_MEMBER_NAME_STRING_CHECKED(ACaseWorkLine, StartPoint), StartPointJson);

	TArray<TSharedPtr<FJsonValue>> EndPointJson;
	EndPointJson.Add(MakeShareable(new FJsonValueNumber(EndPoint.X)));
	EndPointJson.Add(MakeShareable(new FJsonValueNumber(EndPoint.Y)));
	EndPointJson.Add(MakeShareable(new FJsonValueNumber(EndPoint.Z)));
	ResultJson->SetArrayField(GET_MEMBER_NAME_STRING_CHECKED(ACaseWorkLine, EndPoint), EndPointJson);

	return ResultJson;
}

bool ACaseWorkLine::DeserializeFromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	auto StartPointJson = JsonObject->GetArrayField(GET_MEMBER_NAME_STRING_CHECKED(ACaseWorkLine, StartPoint));
	StartPoint.Set(StartPointJson[0]->AsNumber(), StartPointJson[1]->AsNumber(), StartPointJson[2]->AsNumber());

	auto EndPointJson = JsonObject->GetArrayField(GET_MEMBER_NAME_STRING_CHECKED(ACaseWorkLine, EndPoint));
	EndPoint.Set(EndPointJson[0]->AsNumber(), EndPointJson[1]->AsNumber(), EndPointJson[2]->AsNumber());

	SetEndPoint(EndPoint);
	bPlaced = true;

	return true;
}
