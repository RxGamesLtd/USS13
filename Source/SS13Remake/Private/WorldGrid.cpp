// The MIT License (MIT)
// Copyright (c) 2018 RxCompile
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

#include "WorldGrid.h"
#include "UniquePtr.h"

// Sets default values
AWorldGrid::AWorldGrid()
{
    PrimaryActorTick.bCanEverTick = true;
    m_atmosphericsManager = MakeUnique<FFluidSimulationManager>();

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    GroundCollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("GroundCollision"));
    GroundCollisionComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
    GroundCollisionComponent->SetCollisionProfileName(FName(TEXT("Floor")));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> s_boxMesh(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
    GroundMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundMesh"));
    GroundMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
    GroundMeshComponent->SetStaticMesh(s_boxMesh.Object);
}

void AWorldGrid::OnConstruction(const FTransform& transform)
{
    Super::OnConstruction(transform);
    auto gridFloorSize = Size * CellExtent;
    gridFloorSize.Z = 1.0f;
    GroundCollisionComponent->SetBoxExtent(gridFloorSize);
    GroundMeshComponent->SetRelativeScale3D(gridFloorSize / 50.0f);
}

// Called when the game starts or when spawned
void AWorldGrid::BeginPlay()
{
    Super::BeginPlay();
    m_atmosphericsManager->setSize(Size);
    m_atmosphericsManager->start();
}

// Called every frame
void AWorldGrid::Tick(float deltaTime)
{
    Super::Tick(deltaTime);
}

FAtmoStruct AWorldGrid::GetAtmosphericsReport(const FVector& location) const
{
    const auto index = getCellIndexFromWorldLocation(location);
    if(index == FIntVector::NoneValue)
        return {};
    if(!m_atmosphericsManager->isStarted())
        return {};

    return m_atmosphericsManager->getPressure(index.X, index.Y, index.Z);
}

bool AWorldGrid::GetFloorBlockConstructionLocation(const FVector& hitLocation,
                                                   FVector& floorCenter,
                                                   FVector& floorExtent) const
{
    const auto index = getCellIndexFromWorldLocation(hitLocation);
    if(index == FIntVector::NoneValue)
        return false;

    floorCenter = CellExtent * FVector(index.X, index.Y, index.Z) + GetActorLocation();
    floorCenter.Z -= CellExtent.Z;
    floorExtent = FVector{CellExtent.X, CellExtent.Y, FloorDepth};
    return true;
}

bool AWorldGrid::GetWallBlockConstructionLocation(const FVector& hitLocation,
                                                  FVector& wallCenter,
                                                  FVector& wallExtent,
                                                  EWallDirection& wallDirection) const
{
    const auto index = getCellIndexFromWorldLocation(hitLocation);
    if(index == FIntVector::NoneValue)
        return false;

    const auto worldCellSize = CellExtent * 2.0f;
    const auto halfSize = Size * CellExtent / 2.0f + CellExtent;
    auto floorCenter = worldCellSize * FVector(index.X, index.Y, index.Z) + GetActorLocation() - halfSize;
    floorCenter.Z -= CellExtent.Z;
    const auto direction = (hitLocation - floorCenter).GetSafeNormal2D();
    if(direction.X > 0.5f)
    {
        wallDirection = EWallDirection::East;
        wallCenter = floorCenter + FVector{CellExtent.X, 0.0f, CellExtent.Z};
        wallExtent = FVector{WallThickness, CellExtent.Y, CellExtent.Z};
        return true;
    }
    if(direction.X < -0.5f)
    {
        wallDirection = EWallDirection::West;
        wallCenter = floorCenter + FVector{-CellExtent.X, 0.0f, CellExtent.Z};
        wallExtent = FVector{WallThickness, CellExtent.Y, CellExtent.Z};
        return true;
    }
    if(direction.Y > 0.5f)
    {
        wallDirection = EWallDirection::North;
        wallCenter = floorCenter + FVector{0.0f, CellExtent.Y, CellExtent.Z};
        wallExtent = FVector{CellExtent.X, WallThickness, CellExtent.Z};
        return true;
    }
    if(direction.Y < -0.5f)
    {
        wallDirection = EWallDirection::South;
        wallCenter = floorCenter + FVector{0.0f, -CellExtent.Y, CellExtent.Z};
        wallExtent = FVector{CellExtent.X, WallThickness, CellExtent.Z};
        return true;
    }
    wallDirection = EWallDirection::Invalid;
    return false;
}

FIntVector AWorldGrid::getCellIndexFromWorldLocation(const FVector& location) const
{
    const auto halfSize = Size * CellExtent / 2.0f + CellExtent;
    const auto worldCellSize = CellExtent * 2.0f;
    const auto index = (location - this->GetActorLocation() + halfSize) / worldCellSize;
    if(index.X < 0 || index.X >= Size.X)
        return FIntVector::NoneValue;
    if(index.Y < 0 || index.Y >= Size.Y)
        return FIntVector::NoneValue;
    if(index.Z < 0 || index.Z >= Size.Z)
        return FIntVector::NoneValue;

    return {FMath::RoundToInt(index.X), FMath::RoundToInt(index.Y), FMath::RoundToInt(index.Z)};
}
