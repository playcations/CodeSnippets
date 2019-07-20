// Fill out your copyright notice in the Description page of Project Settings.

#include "DimensionStringBase.h"


// Sets default values
ADimensionStringBase::ADimensionStringBase()
{
	// Set this actor to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

// Called when the game starts or when spawned
void ADimensionStringBase::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void ADimensionStringBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Base class function to form the five lines from four points and the text position.
void ADimensionStringBase::SetDimensionString_Implementation(FVector WPointB, FVector WPointA, FVector TPointB, FVector TPointA, FVector TextUp, FVector TextRight, FVector TextForward)
{

}

// Base class function to set the string invisible.
void ADimensionStringBase::SetInvisible_Implementation()
{

}

// Base class function to set the string invisible.
void ADimensionStringBase::SetVisible_Implementation()
{

}

