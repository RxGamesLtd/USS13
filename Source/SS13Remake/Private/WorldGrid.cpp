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
