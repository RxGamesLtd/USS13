// The MIT License (MIT)
// Copyright (c) 2016 RxCompile
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions
// of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
// OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "Private/GridUtils.h"
#include "FluidSimulationManager.h"
#include "AtmoStruct.h"
#include "WorldGrid.generated.h"

UCLASS()

class SS13REMAKE_API AWorldGrid : public AActor {
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AWorldGrid();

    // Called when the game starts or when spawned
    void BeginPlay() override;

    // Called every frame
    void Tick(float DeltaSeconds) override;

    UPROPERTY()
    TArray<FGridCell> Grid;

    UPROPERTY(Category = "Grid", BlueprintReadWrite, EditAnywhere)
    FVector Size;

    UPROPERTY(Category = "Grid", BlueprintReadWrite, EditAnywhere)
    FVector CellExtent;

    UFUNCTION(Category = "Grid", BlueprintCallable)

    FAtmoStruct GetAtmoStatusByLocation(FVector location) const;

protected:

    FAtmoStruct GetAtmoStatusByIndex(int32 x, int32 y, int32 z) const;

    TSharedPtr<FFluidSimulationManager> AtmosManager;
};
