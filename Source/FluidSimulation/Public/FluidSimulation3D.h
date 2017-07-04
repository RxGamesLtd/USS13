//-------------------------------------------------------------------------------------
//
// Copyright 2009 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//
//--------------------------------------------------------------------------------------
// Portions of the fluid simulation are based on the original work
// "Practical Fluid Mechanics" by Mick West used with permission.
//	http://www.gamasutra.com/view/feature/1549/practical_fluid_dynamics_part_1.php
//	http://www.gamasutra.com/view/feature/1615/practical_fluid_dynamics_part_2.php
//	http://cowboyprogramming.com/2008/04/01/practical-fluid-mechanics/
//-------------------------------------------------------------------------------------

#pragma once

#include "AtmoPkg3D.h"
#include "FluidSimulation.h"
#include "VelPkg3D.h"

DECLARE_CYCLE_STAT_EXTERN(TEXT("AtmosUpdatesCount"), STAT_AtmosUpdatesCount, STATGROUP_AtmosStats, FLUIDSIMULATION_API);

enum class EFlowDirection : uint32 {
    None = 0x0,
    Z_Plus = 0x1,
    Z_Minus = 0x2,
    Y_Plus = 0x4,
    Y_Minus = 0x8,
    X_Plus = 0x10,
    X_Minus = 0x20,
    Self = 0x40,
    MAX = 0xFFFFFFFF
};
ENUM_CLASS_FLAGS(EFlowDirection)

// Defines how fluid objects can interact with each other in order to create a fluid simulation
class FLUIDSIMULATION_API FluidSimulation3D {
public:
    // Constructor - Set size of array and timestep
    FluidSimulation3D(int32 xSize, int32 ySize, int32 zSize, float dt);

    ~FluidSimulation3D() {}

    // Updates all fluid objects across a single timestep
    void Update() const;

    // Applies diffusion across the simulation grids
    void UpdateDiffusion() const;

    // Applies forces across the simulation grids
    void UpdateForces() const;

    // Advects across the simulation grids, resulting in net ink and velocity states
    void UpdateAdvection() const;

    // Resets the fluid simulation to the default state
    void Reset() const;

    // Fluid object accessors
    VelPkg3D& Velocity() const
    {
        return *mp_velocity;
    }

    AtmoPkg3D& Pressure() const
    {
        return *mp_pressure;
    }

    // Fluid property accessors
    int32 DiffusionIterations() const
    {
        return m_diffusionIter;
    }

    void DiffusionIterations(int32 value)
    {
        m_diffusionIter = value;
    }

    float Vorticity() const
    {
        return m_vorticity;
    }

    void Vorticity(float value)
    {
        m_vorticity = value;
    }

    float PressureAccel() const
    {
        return m_pressureAccel;
    }

    void PressureAccel(float value)
    {
        m_pressureAccel = value;
    }

    float dt() const
    {
        return m_dt;
    }

    void dt(float value)
    {
        m_dt = value;
    }

    int32 Height() const
    {
        return m_size_z;
    }

    int32 Width() const
    {
        return m_size_y;
    }

    int32 Depth() const
    {
        return m_size_x;
    }

    bool m_bBoundaryCondition;

protected:
    // Solids
    TUniquePtr<TArray3D<EFlowDirection>> m_solids;
    // Fluid objects
    TUniquePtr<Fluid3D> m_curl;
    TUniquePtr<VelPkg3D> mp_velocity;
    TUniquePtr<AtmoPkg3D> mp_pressure; // equivalent to density

    // Fluid properties
    int32 m_diffusionIter; // diffusion cycles per call to Update()
    float m_vorticity; // level of vorticity confinement to apply
    float m_pressureAccel; // Pressure accelleration.  Values >0.5 are more realistic, values too large lead to chaotic waves
    float m_dt; // time step
    const int32 m_size_x; // width of simulation
    const int32 m_size_y; // height of simulation
    const int32 m_size_z; // depth of the simulation

private:
    // Forward advection moves the value at each grid point forward along the velocity field
    // and dissipates it between the four nearest ending points, values are scaled to be > 0
    // Drawback: Does not handle the dissipation of single cells of pressure (or lines of cells)
    void ForwardAdvection(const Fluid3D& p_in, Fluid3D& p_out, float scale) const;

    // Reverse advection moves the value at each grid point backward along the velocity field
    // and dissipates it between the four nearest ending points, values are scaled to be > 0
    // Drawback: Does not handle self-advection of velocity without diffusion
    void ReverseAdvection(const Fluid3D& p_in, Fluid3D& p_out, float scale) const;

    // Reverse Signed Advection is a simpler implementation of ReverseAdvection that does not scale
    // the values to be > 0.  Used for self-advecting velocity as velocity can be < 0.
    void ReverseSignedAdvection(VelPkg3D& v, const float scale) const;

    // Smooth out the velocity and pressure fields by applying a diffusion filter
    void DiffusionStable(const Fluid3D& p_in, Fluid3D& p_out, float scale) const;

    // Checks for boundaries and walls when diffuse gas
    float FluidSimulation3D::TransferPresure(const Fluid3D& p_in, int32 x, int32 y, int32 z, float force) const;

    // Checks is specific direction is blocked for transfer
    bool IsBlocked(int32 x, int32 y, int32 z, EFlowDirection dir) const;

    // Checks if destination point during advection is out of bounds and pulls point in if needed
    bool Collide(int32 this_x, int32 this_y, int32 this_z, float& new_x, float& new_y, float& new_z) const;

    // Alters velocity to move areas of high pressure to low pressure to emulate incompressibility and mass conservation.
    // Allows mass to circulate and not compress into a single cell
    void PressureAcceleration(const float force) const;

    // Exponentially Decays value towards zero to emulate natural friction on the forces
    void ExponentialDecay(Fluid3D& p_in_and_out, const float decay) const;

    // Cosmetic patch that accellerates the velocity field in a direction tangential to the curve defined by the surrounding points
    // in ordert to produce vorticities in the fluid
    void VorticityConfinement(const float scale) const;

    // Returns the vortex strength (curl) at the specified cell.
    float Curl(const int32 x, const int32 y, const int32 z) const;
};
