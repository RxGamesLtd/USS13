#pragma once

#include "Object.h"
#include "AtmoStruct.generated.h"

USTRUCT(BlueprintType)
struct FLUIDSIMULATION_API FAtmoStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float O2;

	UPROPERTY(BlueprintReadOnly)
	float N2;

	UPROPERTY(BlueprintReadOnly)
	float CO2;

	UPROPERTY(BlueprintReadOnly)
	float Toxin;

	//For GC
	void Destroy()
	{
	}

	//Constructor
	FAtmoStruct()
	{
		O2 = 0.0f;
		CO2 = 0.0f;
		N2 = 0.0f;
		Toxin = 0.0f;
	}
};