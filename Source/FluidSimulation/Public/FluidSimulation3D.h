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
#include "VelPkg3D.h"

enum class EFlowDirection : uint32
{
    None = 0x0,
    ZPlus = 0x1,
    ZMinus = 0x2,
    YPlus = 0x4,
    YMinus = 0x8,
    XPlus = 0x10,
    XMinus = 0x20,
    Self = 0x40,
    Max = 0xFFFFFFFF
};
ENUM_CLASS_FLAGS(EFlowDirection)

// Defines how fluid objects can interact with each other in order to create a fluid simulation
class FLUIDSIMULATION_API FluidSimulation3D
{
public:
    // Constructor - Set size of array and timestep
    FluidSimulation3D(int32 xSize, int32 ySize, int32 zSize, float dt);

    // Updates all fluid objects across a single timestep
    void update();

    // Applies diffusion across the simulation grids
    void updateDiffusion();

    // Applies forces across the simulation grids
    void updateForces();

    // Advects across the simulation grids, resulting in net ink and velocity states
    void updateAdvection();

    // Resets the fluid simulation to the default state
    void reset();

    // Fluid object accessors
    VelPkg3D& velocity() { return m_velocity; }
    AtmoPkg3D& pressure() { return m_pressure; }
    TArray3D<EFlowDirection>& solids() { return m_solids; }

    // Fluid property accessors
    int32 diffusionIterations() const { return m_diffusionIter; }

    void diffusionIterations(int32 value) { m_diffusionIter = value; }

    float vorticity() const { return m_vorticity; }

    void vorticity(float value) { m_vorticity = value; }

    float pressureAccel() const { return m_pressureAccel; }

    void pressureAccel(float value) { m_pressureAccel = value; }

    float dt() const { return m_dt; }

    void dt(float value) { m_dt = value; }

    int32 height() const { return m_sizeZ; }

    int32 width() const { return m_sizeY; }

    int32 depth() const { return m_sizeX; }

private:
    // Solids
    TArray3D<EFlowDirection> m_solids;
    // Fluid objects
    Fluid3D m_curl;
    VelPkg3D m_velocity;
    AtmoPkg3D m_pressure; // equivalent to density

    // Fluid properties
    int32 m_diffusionIter; // diffusion cycles per call to Update()
    float m_vorticity; // level of vorticity confinement to apply
    float m_pressureAccel; // Pressure accelleration.  Values >0.5 are more realistic, values too large lead to chaotic
                           // waves
    float m_dt; // time step
    const int32 m_sizeX; // width of simulation
    const int32 m_sizeY; // height of simulation
    const int32 m_sizeZ; // depth of the simulation

    // Forward advection moves the value at each grid point forward along the velocity field
    // and dissipates it between the four nearest ending points, values are scaled to be > 0
    // Drawback: Does not handle the dissipation of single cells of pressure (or lines of cells)
    void forwardAdvection(const Fluid3D& in, Fluid3D& out, float scale) const;

    // Reverse advection moves the value at each grid point backward along the velocity field
    // and dissipates it between the four nearest ending points, values are scaled to be > 0
    // Drawback: Does not handle self-advection of velocity without diffusion
    void reverseAdvection(const Fluid3D& in, Fluid3D& out, float scale) const;

    // Reverse Signed Advection is a simpler implementation of ReverseAdvection that does not scale
    // the values to be > 0.  Used for self-advecting velocity as velocity can be < 0.
    void reverseSignedAdvection(VelPkg3D& v, float scale) const;

    // Smooth out the velocity and pressure fields by applying a diffusion filter
    void diffusionStable(const Fluid3D& in, Fluid3D& out, float scale) const;

    // Checks for boundaries and walls when diffuse gas
    float transferPressure(const Fluid3D& in, int32 x, int32 y, int32 z, float force) const;

    // Checks is specific direction is blocked for transfer
    bool isBlocked(int32 x, int32 y, int32 z, EFlowDirection dir) const;

    // Checks if destination point during advection is out of bounds and pulls point in if needed
    bool collide(int32 thisX, int32 thisY, int32 thisZ, float& newX, float& newY, float& newZ) const;

    // Alters velocity to move areas of high pressure to low pressure to emulate incompressibility and mass
    // conservation. Allows mass to circulate and not compress into a single cell
    void pressureAcceleration(float scale);

    // Exponentially Decays value towards zero to emulate natural friction on the forces
    void exponentialDecay(Fluid3D& data, float decay) const;

    // Cosmetic patch that accellerates the velocity field in a direction tangential to the curve defined by the
    // surrounding points in ordert to produce vorticities in the fluid
    void vorticityConfinement(float scale);

    // Returns the vortex strength (curl) at the specified cell.
    float curl(int32 x, int32 y, int32 z) const;
};
