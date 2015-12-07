// Fill out your copyright notice in the Description page of Project Settings.

#include "SS13Remake.h"
#include "WorldGrid.h"

DEFINE_STAT(STAT_AtmosRequestsCount);

// Sets default values
AWorldGrid::AWorldGrid()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    AtmosManager = MakeShareable(new FFluidSimulationManager());
}

// Called when the game starts or when spawned
void AWorldGrid::BeginPlay()
{
	Super::BeginPlay();
    AtmosManager->SetSize(Size);
    AtmosManager->Start();
}

// Called every frame
void AWorldGrid::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

float AWorldGrid::GetAtmoStatusByIndex(int32 x, int32 y, int32 z) const
{
    if (!AtmosManager->IsStarted())
        return 0.0f;

    return AtmosManager->GetValue(x, y, z);
}

float AWorldGrid::GetAtmoStatusByLocation(FVector location) const
{
    SCOPE_CYCLE_COUNTER(STAT_AtmosRequestsCount);
    auto halfSize = Size * CellExtent / 2;
    auto worldCellSize = CellExtent;
    auto index = (location - this->GetActorLocation() + halfSize) / worldCellSize;
    
    auto x = FMath::FloorToInt(index.X); 
    auto y = FMath::FloorToInt(index.Y);
    auto z = FMath::FloorToInt(index.Z);

    return GetAtmoStatusByIndex(x, y, z);
}
