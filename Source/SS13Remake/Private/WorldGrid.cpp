// Fill out your copyright notice in the Description page of Project Settings.

#include "SS13Remake.h"
#include "WorldGrid.h"

// Sets default values
AWorldGrid::AWorldGrid() {
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
    AtmosManager = MakeShareable(new FFluidSimulationManager());
}

// Called when the game starts or when spawned
void AWorldGrid::BeginPlay() {
    Super::BeginPlay();
    AtmosManager->SetSize(Size);
    AtmosManager->Start();
}

// Called every frame
void AWorldGrid::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);
}

FAtmoStruct AWorldGrid::GetAtmoStatusByIndex(int32 x, int32 y, int32 z) const {
    if (!AtmosManager->IsStarted())
        return FAtmoStruct();

    return AtmosManager->GetValue(x, y, z);
}

FAtmoStruct AWorldGrid::GetAtmoStatusByLocation(FVector location) const {
    auto halfSize = Size * CellExtent / 2.0f + CellExtent;
    halfSize.Z -= CellExtent.Z / 2.0f;
    auto worldCellSize = CellExtent * 2.0f;
    auto index = (location - this->GetActorLocation() + halfSize) / worldCellSize;

    auto x = FMath::FloorToInt(index.X);
    auto y = FMath::FloorToInt(index.Y);
    auto z = FMath::FloorToInt(index.Z);

    auto ret = GetAtmoStatusByIndex(x, y, z);

    return ret;
}
