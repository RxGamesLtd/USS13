// Fill out your copyright notice in the Description page of Project Settings.

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
};
