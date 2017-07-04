// The MIT License (MIT)
// Copyright (c) 2017 RxCompile
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

#pragma once

#include "FluidSimulation.h"
#include "FluidSimulation3D.h"

class FLUIDSIMULATION_API FFluidSimulationManager
    : public FRunnable {
public:
    FFluidSimulationManager();

    FIntVector Size;

    void SetSize(FVector size);

    void Start();

    bool IsStarted() const
    {
        return !bIsTaskStopped;
    }

    bool Init() override;

    uint32 Run() override;

    void Stop() override;

    struct FAtmoStruct GetValue(int32 x, int32 y, int32 z) const;

    FVector GetVelocity(int32 x, int32 y, int32 z) const;

private:
    /** SimulationObject */
    TUniquePtr<FluidSimulation3D> sim;
    /** Thread to run the worker FRunnable on */
    TUniquePtr<FRunnableThread> Thread;
    /** Stop this thread? Uses Thread Safe Counter */
    class FThreadSafeBool bIsTaskStopped;

protected:
    float InitializeAtmoCell(int32 x, int32 y, int32 z, uint32 type) const;
};
