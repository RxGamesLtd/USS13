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

#include "FluidSimulationManager.h"

#include "AtmoStruct.h"
#include "FluidSimulation3D.h"
#include "FluidSimulationModule.h"

FFluidSimulationManager::FFluidSimulationManager() : m_isTaskStopped(true), m_size(1, 1, 1) {}

void FFluidSimulationManager::setSize(FVector size)
{
    m_size = {FMath::CeilToInt(size.X), FMath::CeilToInt(size.Y), FMath::CeilToInt(size.Z)};
    // add boundaries
    m_size += FIntVector(2);
}

void FFluidSimulationManager::start()
{
    m_thread.Reset(FRunnableThread::Create(this, TEXT("FFluidSimulationManager")));
}

bool FFluidSimulationManager::Init()
{
    m_sim = MakeUnique<FluidSimulation3D>(m_size.X, m_size.Y, m_size.Z, 0.1f);
    UE_LOG(LogFluidSimulation, Log, TEXT("Atmo thread init start"));

    {
        Fluid3D::SetterDelegate binder;
        binder.BindRaw(this, &FFluidSimulationManager::initializeAtmoCell, static_cast<uint32>(0));
        m_sim->pressure().oxigen().destination().set(binder);
        UE_LOG(LogFluidSimulation, Log, TEXT("Atmo O2 values loaded"));
    }

    {
        Fluid3D::SetterDelegate binder;
        binder.BindRaw(this, &FFluidSimulationManager::initializeAtmoCell, static_cast<uint32>(1));
        m_sim->pressure().nitrogen().destination().set(binder);
        UE_LOG(LogFluidSimulation, Log, TEXT("Atmo N2 values loaded"));
    }

    {
        Fluid3D::SetterDelegate binder;
        binder.BindRaw(this, &FFluidSimulationManager::initializeAtmoCell, static_cast<uint32>(2));
        m_sim->pressure().carbonDioxide().destination().set(binder);
        UE_LOG(LogFluidSimulation, Log, TEXT("Atmo CO2 values loaded"));
    }

    {
        Fluid3D::SetterDelegate binder;
        binder.BindRaw(this, &FFluidSimulationManager::initializeAtmoCell, static_cast<uint32>(3));
        m_sim->pressure().toxin().destination().set(binder);
        UE_LOG(LogFluidSimulation, Log, TEXT("Atmo Toxin values loaded"));
    }

    // apply to source
    m_sim->pressure().swap();

    // reset velocity map
    m_sim->velocity().reset(0.0f);

    // set solids
    {
        TBaseDelegate<EFlowDirection, int32, int32, int32> binder;
        binder.BindRaw(this, &FFluidSimulationManager::initializeSolid);
        m_sim->solids().set(binder);
    }

    m_sim->diffusionIterations(15);
    m_sim->pressureAccel(1.0f);
    m_sim->vorticity(0.03f);

    m_sim->pressure().properties().diffusion = 1.0f;
    m_sim->pressure().properties().advection = 1.0f;

    m_sim->velocity().properties().diffusion = 1.0f;
    m_sim->velocity().properties().advection = 1.0f;
    m_sim->velocity().properties().decay = 0.5f;

    m_isTaskStopped = false;
    UE_LOG(LogFluidSimulation, Log, TEXT("Atmo thread initialized"));
    return true;
}

uint32 FFluidSimulationManager::Run()
{
    auto timestamp = FPlatformTime::Seconds();
    // Initial wait before starting
    FPlatformProcess::Sleep(0.03);
    UE_LOG(LogFluidSimulation, Log, TEXT("Atmo thread started"));

    while(!m_isTaskStopped)
    {
        if(!m_sim.IsValid())
            break;
        const auto waitInterval = 1.0f / 30.0f;
        auto delta = FPlatformTime::Seconds() - timestamp;
        if(delta < waitInterval)
            FPlatformProcess::Sleep(waitInterval - delta);
        delta = FPlatformTime::Seconds() - timestamp;
        m_sim->dt(delta);
        m_sim->update();
        timestamp = FPlatformTime::Seconds();
    }
    UE_LOG(LogFluidSimulation, Log, TEXT("Atmo thread is exited"));
    m_isTaskStopped = false;
    m_sim->reset();

    return 0;
}

void FFluidSimulationManager::Stop()
{
    m_isTaskStopped = true;
    m_thread->WaitForCompletion();
}

FAtmoStruct FFluidSimulationManager::getPressure(int32 x, int32 y, int32 z) const
{
    if(x < 0 || x >= m_size.X)
    {
        return {};
    }
    if(y < 0 || y >= m_size.Y)
    {
        return {};
    }
    if(z < 0 || z >= m_size.Z)
    {
        return {};
    }

    FAtmoStruct atmo;
    atmo.O2 = m_sim->pressure().oxigen().source().element(x, y, z);
    atmo.N2 = m_sim->pressure().nitrogen().source().element(x, y, z);
    atmo.CO2 = m_sim->pressure().carbonDioxide().source().element(x, y, z);
    atmo.Toxin = m_sim->pressure().toxin().source().element(x, y, z);

    return atmo;
}

FVector FFluidSimulationManager::getVelocity(int32 x, int32 y, int32 z) const
{
    if(x < 0 || x >= m_size.X)
        return {};
    if(y < 0 || y >= m_size.Y)
        return {};
    if(z < 0 || z >= m_size.Z)
        return {};

    const auto sourceX = m_sim->velocity().sourceX().element(x, y, z);
    const auto sourceY = m_sim->velocity().sourceY().element(x, y, z);
    const auto sourceZ = m_sim->velocity().sourceZ().element(x, y, z);

    return {sourceX, sourceY, sourceZ};
}

EFlowDirection FFluidSimulationManager::initializeSolid(int32 x, int32 y, int32 z) const
{
    // Which direction is blocked
    auto mask = EFlowDirection::None;
    if(x <= 0)
        mask |= EFlowDirection::XMinus;
    if(x >= m_size.X - 1)
        mask |= EFlowDirection::XPlus;
    if(y <= 0)
        mask |= EFlowDirection::YMinus;
    if(y >= m_size.Y - 1)
        mask |= EFlowDirection::YPlus;
    if(z <= 0)
        mask |= EFlowDirection::ZMinus;
    if(z >= m_size.Z - 1)
        mask |= EFlowDirection::ZPlus;
    return mask;
}

float FFluidSimulationManager::initializeAtmoCell(int32 x, int32 y, int32 z, uint32 type) const
{
    // TODO: Load from file
    // For now just random
    return FMath::FRandRange(10.0f, 1200.0f);
}
