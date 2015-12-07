#pragma once

#include "FluidSimulation3D.h"
#include "ThreadingBase.h"
#include "Object.h"
#include "Platform.h"
#include "SharedPointer.h"

class FLUIDSIMULATION_API FFluidSimulationManager : public FRunnable
{
public:
    FFluidSimulationManager();
    ~FFluidSimulationManager();
    FIntVector Size;

    void SetSize(FVector size);

    void Start();

    bool IsStarted() const
    {
        return StopTaskCounter.GetValue() == 0;
    }

    virtual bool Init() override;

    virtual uint32 Run() override;

    virtual void Stop() override;

    float GetValue(int32 x, int32 y, int32 z) const;

private:
    /** SimulationObject */
    TSharedPtr<FluidSimulation3D, ESPMode::ThreadSafe> sim;
    /** Thread to run the worker FRunnable on */
    TSharedPtr<FRunnableThread> Thread;
    /** Stop this thread? Uses Thread Safe Counter */
    FThreadSafeCounter StopTaskCounter;
    /** Initial distribution of values*/
    void InitDistribution(float& arr, int32 i, int32 j, int32 k, int64 index) const;
};