#pragma once

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
	
	FGridCell()
	{
		Index = FIntVector();
	};

    FGridCell(TSharedPtr<AWorldGrid> grid, FIntVector index)
    {
        Index = index;
		WorldGrid = grid;
	};

private:

	TSharedPtr<AWorldGrid> WorldGrid;

};
