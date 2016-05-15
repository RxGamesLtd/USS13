#include "Public/FluidSimulation.h"
#include "Public/FluidSimulation3D.h"
#include "Public/FluidSimulationManager.h"

FFluidSimulationManager::FFluidSimulationManager()
{
    StopTaskCounter.Increment();
    Size = FIntVector(1, 1, 1);
}

FFluidSimulationManager::~FFluidSimulationManager()
{
}

void FFluidSimulationManager::SetSize(FVector size)
{
    // Preserve boundaries
    Size = FIntVector(
        FMath::CeilToInt(size.X),
        FMath::CeilToInt(size.Y),
        FMath::CeilToInt(size.Z)) * 2 +
        FIntVector(1);
}

void FFluidSimulationManager::Start()
{
    Thread = MakeShareable(FRunnableThread::Create(this, TEXT("FFluidSimulationManager"), 0, TPri_Normal)); //windows default = 8mb for thread, could specify more
}

bool FFluidSimulationManager::Init()
{
    sim = MakeShareable(new FluidSimulation3D(Size.X, Size.Y, Size.Z, 0.1f));
    sim->Pressure()->SourceO2()->Set<FFluidSimulationManager>(this, &FFluidSimulationManager::InitDistribution);
	//sim->Pressure()->DestinationO2()->Set<FFluidSimulationManager>(this, &FFluidSimulationManager::InitDistribution);
    
    sim->DiffusionIterations(10);
    sim->PressureAccel(2.0f);
    sim->Vorticity(0.03f);
    sim->m_bBoundaryCondition = false;
     
    sim->Pressure()->Properties()->diffusion = 1.0f;
    sim->Pressure()->Properties()->advection = 1.0f;

    sim->Velocity()->Properties()->diffusion = 1.0f;
    sim->Velocity()->Properties()->decay = 0.5f;
    sim->Velocity()->Properties()->advection = 1.0f;
    
    StopTaskCounter.Reset();
    return true;
}

void FFluidSimulationManager::InitDistribution(float& arr, int32 i, int32 j, int32 k, int64 index) const
{
    //TODO: load from save file
    arr = FMath::RandRange(100, 150);
}

uint32 FFluidSimulationManager::Run()
{
    //Initial wait before starting
    FPlatformProcess::Sleep(0.03);

    //While not told to stop this thread 
    //		and not yet finished finding Prime Numbers
    while (StopTaskCounter.GetValue() == 0)
    {
        if (!sim.IsValid()) break;

        sim->Update();

        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        //prevent thread from using too many resources
        FPlatformProcess::Sleep(0.1f);
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    }
    UE_LOG(LogFluidSimulation, Log, TEXT("Atmo thread is exited"));
    StopTaskCounter.Reset();
    sim = nullptr;
    
    return 0;
}

void FFluidSimulationManager::Stop()
{
    StopTaskCounter.Increment();
    Thread->WaitForCompletion();
}

FAtmoStruct FFluidSimulationManager::GetValue(int32 x, int32 y, int32 z) const
{
	FAtmoStruct atmo;

    auto cells = (Size - FIntVector(1)) / 2;
    
    if (x < 0 || x >= cells.X)
        return atmo;
    if (y < 0 || y >= cells.Y)
        return atmo;
    if (z < 0 || z >= cells.Z)
        return atmo;
    
	//0 - wall, 1 - cell, 2- wall, ...
	auto x1 = x * 2 + 1;
	auto y1 = y * 2 + 1;
	auto z1 = z * 2 + 1;

 	atmo.O2 = sim->Pressure()->SourceO2()->element(x1, y1, z1);
	atmo.N2 = sim->Pressure()->SourceN2()->element(x1, y1, z1);
	atmo.CO2 = sim->Pressure()->SourceCO2()->element(x1, y1, z1);
	atmo.Toxin = sim->Pressure()->SourceToxin()->element(x1, y1, z1);

    return atmo;
}

FVector FFluidSimulationManager::GetVelocity(int32 x, int32 y, int32 z) const
{
    auto cells = Size - FIntVector(1);
    cells /= 2;

    if (x < 0 || x >= cells.X)
        return FVector();
    if (y < 0 || y >= cells.Y)
        return FVector();
    if (z < 0 || z >= cells.Z)
        return FVector();

    auto sourceX = sim->Velocity()->SourceX()->element(x, y, z);
    auto sourceY = sim->Velocity()->SourceY()->element(x, y, z);
    auto sourceZ = sim->Velocity()->SourceZ()->element(x, y, z);
    auto val = FVector(sourceX, sourceY, sourceZ);

    return val;
}
