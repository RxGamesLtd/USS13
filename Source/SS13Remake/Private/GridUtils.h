#pragma once

#include "SS13Remake.h"
#include "IntVector.h"
#include "ObjectBase.h"
#include "AtmoStruct.h"

#include "GridUtils.generated.h"

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

	UFUNCTION(Category = "Grid", BlueprintCallable)
	FAtmoStruct GetAtmo(FVector location) const;

    FGridCell(TSharedPtr<AWorldGrid> grid, FIntVector location)
    {
        Index = location;
		WorldGrid = grid;
	};

private:

	UPROPERTY()
	TSharedPtr<AWorldGrid> WorldGrid;

};
