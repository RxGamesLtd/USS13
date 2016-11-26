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

#pragma once

#include <Runtime/Core/Public/HAL/Platform.h>
#include <Runtime/Core/Public/Core.h>

class AFogOfWarManager;

#define ECC_SightStatic ECC_GameTraceChannel2

class SS13REMAKE_API FogOfWarWorker : public FRunnable {
public:
    explicit FogOfWarWorker(AFogOfWarManager *manager);

    virtual ~FogOfWarWorker();

    //FRunnable interface
    bool Init() override;

    uint32 Run() override;

    void Stop() override;

    //Method to perform work
    void UpdateFowTexture(float time) const;

    void UpdateFowTextureFromCamera(float time) const;

    bool bShouldUpdate = false;

    void ShutDown();

private:
    //Thread to run the FRunnable on
    FRunnableThread *Thread;

    //Pointer to our manager
    AFogOfWarManager *Manager;

    //Thread safe counter
    FThreadSafeCounter StopTaskCounter;

    float TimeTillLastTick;
	
	void DrawUnveilShape(FVector2D observerTexLoc, TSet<FVector2D> sightShape) const;

	void FloodFill(int x, int y) const;
};
