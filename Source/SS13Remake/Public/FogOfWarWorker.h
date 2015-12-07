// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

class AFogOfWarManager;

/**
 * 
 */
class SS13REMAKE_API FogOfWarWorker : public FRunnable
{
public:
    explicit FogOfWarWorker(AFogOfWarManager* manager);
    virtual ~FogOfWarWorker();

    //FRunnable interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;

    //Method to perform work
    void UpdateFowTexture();

    bool bShouldUpdate = false;

    void ShutDown();

private:
    //Thread to run the FRunnable on
    FRunnableThread* Thread;

    //Pointer to our manager
    AFogOfWarManager* Manager;

    //Thread safe counter 
    FThreadSafeCounter StopTaskCounter;
};
