#pragma once

#include <Runtime/Core/Public/Math/IntVector.h>
#include <Runtime/Core/Public/Templates/SharedPointer.h>
#include "GridUtils.generated.h"

class AWorldGrid;

USTRUCT(BlueprintType)
struct FGridCell
{
	GENERATED_BODY();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FIntVector Index;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	uint8 WallN;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	uint8 WallS;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	uint8 WallE;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	uint8 WallW;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	uint8 Floor;

	FGridCell(): WallN(0), WallS(0), WallE(0), WallW(0), Floor(0)
	{
		Index = FIntVector();
	}

	FGridCell(TSharedPtr<AWorldGrid> grid, FIntVector index)
		: FGridCell()
	{
		Index = index;
		WorldGrid = grid;
	}

private:

	TSharedPtr<AWorldGrid> WorldGrid;
};
