// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Private/GridUtils.h"
#include "FluidSimulationManager.h"
#include "WorldGrid.generated.h"

DECLARE_STATS_GROUP(TEXT("Atmospherics"), STATGROUP_SS13Stats, STATCAT_Advanced);
DECLARE_CYCLE_STAT_EXTERN(TEXT("AtmosRequestsCount"), STAT_AtmosRequestsCount, STATGROUP_SS13Stats, SS13REMAKE_API);

UCLASS()
class SS13REMAKE_API AWorldGrid : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWorldGrid();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	UPROPERTY()
	TArray<FGridCell> Grid;

    UPROPERTY(Category = "Grid", BlueprintReadWrite, EditAnywhere)
    FVector Size;
	
    UPROPERTY(Category = "Grid", BlueprintReadWrite, EditAnywhere)
    FVector CellExtent;

    float GetAtmoStatusByIndex(int32 x, int32 y, int32 z) const;

    UFUNCTION(Category = "Grid", BlueprintCallable)
    float GetAtmoStatusByLocation(FVector location) const;

protected:

    TSharedPtr<FFluidSimulationManager> AtmosManager;
};

