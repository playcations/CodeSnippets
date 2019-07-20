// Fill out your copyright notice in the Description page of Project Settings.

#include "PrimitiveTag.h"


// Sets default values for this component's properties
APrimitiveTag::APrimitiveTag()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	//// off to improve performance if you don't need them.
	PrimaryActorTick.bCanEverTick = true;
	//tagInfo = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TagInfo"));
	//tag = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TagMesh"));
	//tagInfo->SetHorizontalAlignment(EHTA_Center);
	//tagInfo->SetTextRenderColor(FColor(0,0,0,1.0f));
	//RootComponent = tag;
	/*tag->AttachTo(this);
	tagInfo->AttachTo(this);*/
	//FTransform transform = tagInfo->GetComponentTransform();
	//transform.SetRotation(FRotator(0, 90, 90));
	//tagInfo->SetWorldRotation(FRotator(90, 0, 0));
	
	

	// ...
}

// Called when the game starts
void APrimitiveTag::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

//
//void APrimitiveTag::AttachAndPlaceTag(FText textInfo, UMaterialInterface* tagMaterial)
//{
//	tagInfo->Text = textInfo;
//	tagInfo->SetMaterial(0, tagMaterial);
//	/*SetPosition();
//	OrientNorth();*/
//}
//
//
//void APrimitiveTag::OrientNorth()
//{
//	/*FQuat* rotation = new FQuat(0, 0, 0, 1);
//	
//	FTransform transform = GetTransform();
//	transform.SetRotation(*rotation);
//	SetWorldRotation(*rotation);*/
//	
//}
//
//
//void APrimitiveTag::SetPosition()
//{
//
//}


// Called every frame
void APrimitiveTag::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ...
}

