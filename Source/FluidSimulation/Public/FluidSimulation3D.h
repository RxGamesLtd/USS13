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

#include "FluidPkg3D.h"
#include "VelPkg3D.h"
#include <Runtime/Core/Public/Templates/SharedPointerInternals.h>
#include "FluidSimulation3D.h"
#include <vector>

DECLARE_STATS_GROUP(TEXT("Atmospherics"), STATGROUP_AtmosStats, STATCAT_Advanced);
DECLARE_CYCLE_STAT_EXTERN(TEXT("AtmosUpdatesCount"), STAT_AtmosUpdatesCount, STATGROUP_AtmosStats, FLUIDSIMULATION_API);

// Defines how fluid objects can interact with each other in order to create a fluid simulation
class FLUIDSIMULATION_API FluidSimulation3D
{
public:

	// Constructor - Set size of array and timestep
	FluidSimulation3D(int32 xSize, int32 ySize, int32 zSize, float dt);

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
    TSharedPtr<VelPkg3D, ESPMode::ThreadSafe> Velocity() const;
    TSharedPtr<FluidPkg3D, ESPMode::ThreadSafe> Pressure() const;
    TSharedPtr<FluidPkg3D, ESPMode::ThreadSafe> Ink() const;
    TSharedPtr<FluidPkg3D, ESPMode::ThreadSafe> Heat() const;

	// Fluid property accessors
	int32 DiffusionIterations() const;
	void DiffusionIterations(int32 value);
	float Vorticity() const;
	void Vorticity(float value);
	float PressureAccel() const;
	void PressureAccel(float value);
	float dt() const;
	void dt(float value);
	int32 Height() const;
	int32 Width() const;
	int32 Depth() const;
	bool m_bBoundaryCondition;

protected:
    // Solids
    TSharedPtr<std::vector<bool>, ESPMode::ThreadSafe> m_solids;
	// Fluid objects
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> m_curl;
    TSharedPtr<VelPkg3D, ESPMode::ThreadSafe> mp_velocity;
    TSharedPtr<FluidPkg3D, ESPMode::ThreadSafe> mp_pressure_O2;  // equivalent to density O2
    TSharedPtr<FluidPkg3D, ESPMode::ThreadSafe> mp_pressure_C02;  // equivalent to density
    TSharedPtr<FluidPkg3D, ESPMode::ThreadSafe> mp_pressure_Toxin;  // equivalent to density
    TSharedPtr<FluidPkg3D, ESPMode::ThreadSafe> mp_pressure_N2;  // equivalent to density
    TSharedPtr<FluidPkg3D, ESPMode::ThreadSafe> mp_ink;       // ink is one fluid suspended in another, like smoke in air
    TSharedPtr<FluidPkg3D, ESPMode::ThreadSafe> mp_heat;
	
	// Fluid properties
	int32 m_diffusionIter;    // diffusion cycles per call to Update()
	float m_vorticity;      // level of vorticity confinement to apply
	float m_pressureAccel;  // Pressure accelleration.  Values >0.5 are more realistic, values too large lead to chaotic waves
	float m_dt;             // time step
	int32 m_size_x;                // width of simulation
	int32 m_size_y;				// height of simulation
	int32 m_size_z;				// depth of the simulation

private:
	// Apply heat as a diffusion step in 3D
	void Heat(float scale) const;

	// Forward advection moves the value at each grid point forward along the velocity field 
	// and dissipates it between the four nearest ending points, values are scaled to be > 0
	// Drawback: Does not handle the dissipation of single cells of pressure (or lines of cells)
	void ForwardAdvection(TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_in, TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_out, float scale) const;

	// Reverse advection moves the value at each grid point backward along the velocity field 
	// and dissipates it between the four nearest ending points, values are scaled to be > 0
	// Drawback: Does not handle self-advection of velocity without diffusion
	void ReverseAdvection(TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_in, TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_out, float scale) const;

	// Reverse Signed Advection is a simpler implementation of ReverseAdvection that does not scale 
	// the values to be > 0.  Used for self-advecting velocity as velocity can be < 0.
	void ReverseSignedAdvection(TSharedPtr<VelPkg3D, ESPMode::ThreadSafe> v, const float scale) const;

	// Smooth out the velocity and pressure fields by applying a diffusion filter
	void Diffusion(TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_in, TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_out, const float scale) const;

    void DiffusionStable(TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_in, TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_out, const float scale) const;

    bool IsSolid(int32 this_x, int32 this_y, int32 this_z) const;

    // Checks if destination point during advection is out of bounds and pulls point in if needed
	bool Collide(int32 this_x, int32 this_y, int32 this_z, float &new_x, float &new_y, float &new_z) const;

	// Alters velocity to move areas of high pressure to low pressure to emulate incompressibility and mass conservation.
	// Allows mass to circulate and not compress into a single cell
	void PressureAcceleration(const float force) const;

	// Exponentially Decays value towards zero to emulate natural friction on the forces
	void ExponentialDecay(TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_in_and_out, const float decay) const;

	// Cosmetic patch that accellerates the velocity field in a direction tangential to the curve defined by the surrounding points
	// in ordert to produce vorticities in the fluid 
	void VorticityConfinement(const float scale) const; 

	// Returns the vortex strength (curl) at the specified cell. 
	float Curl(const int32 x, const int32 y, const int32 z) const;

	// Invert velocities that are facing outwards at boundaries
	void InvertVelocityEdges() const;

	// Checks if floats are aprox equal to zero. 
	bool EqualToZero(float in) const;
};