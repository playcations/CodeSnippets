// Fill out your copyright notice in the Description page of Project Settings.

#include "ModumateUniversalFunctions.h"

void UModumateUniversalFunctions::CentimetersToImperialInches(float Centimeters,  TArray<int>& ReturnImperial)
{
	ReturnImperial.Empty();
	float Inches = Centimeters / 2.54f;
	int NonDecimal = (int)Inches;
	float Decimal = FMath::Abs(Inches - NonDecimal);
	ReturnImperial.Add(NonDecimal / 12);
	ReturnImperial.Add(NonDecimal % 12);
	if (Decimal >= 0.875)
	{
		ReturnImperial[0] = ReturnImperial[0] + (ReturnImperial[1] + 1) / 12;
		ReturnImperial[1] = (ReturnImperial[1] + 1) % 12;
		return;
	}
	else if (Decimal <= 0.125)
	{
		return;
	}
	else
	{
		for (int num = 1; num < 8; num++)
		{
			if ((float)num / 8 <= Decimal && Decimal <= (float)(num + 1) / 8)
			{
				//GEngine->AddOnScreenDebugMessage(0, 0.5f, FColor::Red, *FString::SanitizeFloat((float)num / denom));
				//GEngine->AddOnScreenDebugMessage(0, 0.5f, FColor::Red, *FString::SanitizeFloat((float)(num + 1) / denom));
				float ManhatDistFromLowToDec = FMath::Abs((float)num / 8 - Decimal);
				float ManhatDistFromDecToHigh = FMath::Abs((float)(num + 1) / 8 - Decimal);

				if (ManhatDistFromLowToDec < ManhatDistFromDecToHigh)
				{
					int Gcd = FMath::GreatestCommonDivisor(num, 8);
					ReturnImperial.Add(num / Gcd);
					ReturnImperial.Add(8 / Gcd);
					return;
				}
				else
				{
					int Gcd = FMath::GreatestCommonDivisor(num + 1, 8);
					ReturnImperial.Add((num + 1) / Gcd);
					ReturnImperial.Add(8 / Gcd);
					return;
				}
			}
		}
		return;
	}
	
	
}

FText UModumateUniversalFunctions::ImperialInchesToDimensionStringText(TArray<int>& Imperial)
{
	FString ImperialString("");
	if (Imperial.Num() == 2)
	{
		ImperialString += FString::FromInt(Imperial[0]) + FString("' ") + FString::FromInt(Imperial[1]) + FString("'' ");
	}
	if (Imperial.Num() == 4)
	{
		ImperialString += FString::FromInt(Imperial[0]) + FString("' ") + FString::FromInt(Imperial[1]) + FString(" ");
		ImperialString += FString::FromInt(Imperial[2]) + FString("/") + FString::FromInt(Imperial[3]) + FString(" ''");
	}
	return FText::FromString(ImperialString);
}

void UModumateUniversalFunctions::SetChildComponentOrientation(USceneComponent* ChildComponent, FVector Up, FVector Right, FVector Forward)
{
	FMatrix RotMatrix(Forward, Right, Up, FVector::ZeroVector);
	ChildComponent->SetWorldRotation(RotMatrix.Rotator());
}

