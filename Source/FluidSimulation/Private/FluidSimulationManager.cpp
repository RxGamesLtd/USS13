#include "Public/FluidSimulation.h"
#include "Public/FluidSimulation3D.h"
#include "Public/FluidSimulationManager.h"

FFluidSimulationManager::FFluidSimulationManager()
{
    StopTaskCounter.Increment();
    Size = FIntVector(100, 100, 3);
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
    sim->Pressure()->Source()->Set<FFluidSimulationManager>(this, &FFluidSimulationManager::InitDistribution);
    
    sim->DiffusionIterations(20);
    sim->PressureAccel(2.0f);
    sim->Vorticity(0.03f);
    sim->m_bBoundaryCondition = true;
     
    sim->Pressure()->Properties()->diffusion = 6.0f;
    sim->Pressure()->Properties()->advection = 150.0f;

    sim->Velocity()->Properties()->diffusion = 1.0f;
    sim->Velocity()->Properties()->decay = 0.0f;
    sim->Velocity()->Properties()->advection = 150.0f;
    
    StopTaskCounter.Reset();
    return true;
}


void FFluidSimulationManager::InitDistribution(float& arr, int32 i, int32 j, int32 k, int64 index) const
{
    //TODO: load from save file
    if(index == sim->Height() * sim->Width() * sim->Depth() / 2)
        arr = 30000;
    arr = (i * j * k)*3;
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
        FPlatformProcess::Sleep(1);
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

float FFluidSimulationManager::GetValue(int32 x, int32 y, int32 z) const
{
    auto cells = Size - FIntVector(1);
    cells /= 2;

    if (x < 0 || x >= cells.X)
        return 0.0;
    if (y < 0 || y >= cells.Y)
        return 0.0;
    if (z < 0 || z >= cells.Z)
        return 0.0;
    auto source = sim->Pressure()->Source();
    auto val = source->element(x, y, z);
    
    return val;

    //auto neighbor = 0.0f;
    //if (x > 1) {val += source->element(x - 1, y, z); neighbor++;}
    //if (x < Size.X - 1) { val += source->element(x + 1, y, z); neighbor++; }

    //if (y>1) {val += source->element(x, y - 1, z); neighbor++;}
    //if (y<Size.Y - 1) {val += source->element(x, y + 1, z); neighbor++;}

    //if (z>1) {val += source->element(x, y, z - 1); neighbor++;}
    //if (z<Size.Z - 1) {val += source->element(x, y, z + 1); neighbor++;}
    
    //return neighbor > 0 ? val / neighbor : val;
}
