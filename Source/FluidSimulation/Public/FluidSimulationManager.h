#pragma once

#include "FluidSimulation3D.h"
#include "AtmoStruct.h"
#include "ThreadingBase.h"
#include "Object.h"
#include "Platform.h"
#include "SharedPointer.h"

class FLUIDSIMULATION_API FFluidSimulationManager : public TSharedFromThis<FFluidSimulationManager>, public FRunnable {
public:
    FFluidSimulationManager();

    ~FFluidSimulationManager();

    FIntVector Size;

    void SetSize(FVector size);

    void Start();

    bool IsStarted() const {
        return StopTaskCounter.GetValue() == 0;
    }

    bool Init() override;

    uint32 Run() override;

    void Stop() override;

    FAtmoStruct GetValue(int32 x, int32 y, int32 z) const;

    FVector GetVelocity(int32 x, int32 y, int32 z) const;

private:
    /** SimulationObject */
    TSharedPtr<FluidSimulation3D, ESPMode::ThreadSafe> sim;
    /** Thread to run the worker FRunnable on */
    TSharedPtr<FRunnableThread> Thread;
    /** Stop this thread? Uses Thread Safe Counter */
    FThreadSafeCounter StopTaskCounter;

protected:
    float InitializeAtmoCell(int32 x, int32 y, int32 z) const;
};
