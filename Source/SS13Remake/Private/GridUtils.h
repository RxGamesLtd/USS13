#pragma once

#include "SS13Remake.h"
#include "IntVector.h"
#include "ObjectBase.h"

#include "GridUtils.generated.h"

USTRUCT()
struct FAtmosCell
{
    GENERATED_USTRUCT_BODY();

    float N2O;
    float O2;
    float H2;
    float CO2;

    FAtmosCell()
    {
        N2O = O2 = H2 = CO2 = 0.0f;
    }
};

USTRUCT()
struct FGridCell
{
    GENERATED_USTRUCT_BODY();

    UPROPERTY()
    FIntVector Index;

    UPROPERTY()
    uint8 WallN;

    UPROPERTY()
    uint8 WallS;

    UPROPERTY()
    uint8 WallE;

    UPROPERTY()
    uint8 WallW;

    UPROPERTY()
    uint8 Floor;

    UPROPERTY()
    FAtmosCell Atmos;

    FGridCell()
    {
        Index = FIntVector(1);
    }
};
