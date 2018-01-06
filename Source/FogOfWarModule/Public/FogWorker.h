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

#pragma once

#include <Runnable.h>
#include <Stats2.h>
#include <ThreadSafeBool.h>
#include <UniquePtr.h>

#include "FogOfWarModule.h"

DECLARE_CYCLE_STAT_EXTERN(TEXT("FogUpdatesCount"), STAT_FogUpdatesCount, STATGROUP_FOWStats, FOGOFWARMODULE_API);

class AFogPlane;

class FOGOFWARMODULE_API FogWorker : public FRunnable
{
public:
    explicit FogWorker(AFogPlane& manager);

    // FRunnable interface
    bool Init() override;

    uint32 Run() override;

    void Stop() override;

protected:
    void ForgetOldLocations(float time);

    void UpdateTextureData();

    void UpdateRenderOrigin() const;

    // Method to perform work
    void Update(float time);

    bool bShouldUpdate = false;

    // Fill 1.0f in Unfogged data for given sightShape
    void DrawUnveilShape(FVector2D observerTexLoc, TArray<FVector2D> sightShape);

    void DrawLine(FVector2D start, FVector2D end);

    void DrawPoint(FVector2D point, const int32 brushSize = 1);

    // Fill unfogged data with 1.0f inside box
    void FloodFill(int32 x, int32 y);

private:
    // Thread to run the FRunnable on
    TUniquePtr<FRunnableThread> Thread;

    AFogPlane& Manager;

    // Thread safe counter
    FThreadSafeBool bIsTaskStopped;

    float TimeTillLastTick;

    int32 TextureSize;

    TArray<float> UnfoggedData;
};
