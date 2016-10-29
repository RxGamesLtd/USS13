// Fill out your copyright notice in the Description page of Project Settings.

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
