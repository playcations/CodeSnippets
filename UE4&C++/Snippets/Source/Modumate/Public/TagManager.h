// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"/*
#include "PrimitiveTag.h"*/
#include "TagManager.generated.h"

/**
 * 
 */
UENUM(BlueprintType)		
enum class ETagType : uint8
{

	Door		UMETA(DisplayName = "Door"),
	Fixture		UMETA(DisplayName = "Fixture"),
	Floor		UMETA(DisplayName = "Floor"),
	Furniture	UMETA(DisplayName = "Furniture"),
	Room		UMETA(DisplayName = "Room"),
	Wall		UMETA(DisplayName = "Wall"),
	Window		UMETA(DisplayName = "Window")


};

USTRUCT(Blueprintable)
struct FPrimitiveTagArray
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<class APrimitiveTag*> Tags;
};

UCLASS(Blueprintable)
class MODUMATE_API ATagManager : public AActor
{
	GENERATED_BODY()
	static ATagManager* SingletonInstance;
public:
	ATagManager();
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintNativeEvent)
	void CreateATag(ETagType tagType, AActor * parent, bool bIsDoorLeft);

	UFUNCTION(BlueprintCallable)
		ATagManager* GetInstance();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<class APrimitiveTag*> DoorTags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<class APrimitiveTag*> FixtureTags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<class APrimitiveTag*> FloorTags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<class APrimitiveTag*> FurnitureTags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<class APrimitiveTag*> RoomTags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<class APrimitiveTag*> WallTags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<class APrimitiveTag*> WindowTags;

	/*UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TMap<ETagType, > TagMap;*/
	
};
