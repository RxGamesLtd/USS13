#pragma once

#include "Object.h"
#include "AtmoStruct.generated.h"

USTRUCT(BlueprintType)
struct FLUIDSIMULATION_API FAtmoStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		float O2;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		float N2;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		float CO2;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
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