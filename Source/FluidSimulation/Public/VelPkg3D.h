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

#include "FluidSimulation.h"
#include "Fluid3D.h"
#include "FluidProperties.h"

// VelocityPackage3D is a container class for the Source and Destination Fluid3D objects of velocity in the 
// x, y, and z directions.
class FLUIDSIMULATION_API VelPkg3D {
public:

	// Constructor - Initilizes source and destination FLuid3D objects for velocity in X, Y, Z directions
	VelPkg3D(int32 xSize, int32 ySize, int32 zSize);

	// Assignment Operator
	VelPkg3D& operator=(const VelPkg3D& right);

	// Swap the source and destination objects for velocity in X direction
	void SwapLocationsX();

	// Swap the source and destination objects for velocity in Y direction
	void SwapLocationsY();

	// Swap the source and destination objects for velocity in Z direction
	void SwapLocationsZ();

	// Reset the source and destination objects to specified value
	void Reset(float v) const;

	// Accessors
	Fluid3D& SourceX() { return *mp_sourceX; }
	Fluid3D& SourceY() { return *mp_sourceY; }
	Fluid3D& SourceZ() { return *mp_sourceZ; }

	Fluid3D& DestinationX() { return *mp_destX; }
	Fluid3D& DestinationY() { return *mp_destY; }
	Fluid3D& DestinationZ() { return *mp_destZ; }

	FluidProperties& Properties() { return *mp_prop; }

private:
	TUniquePtr<Fluid3D> mp_sourceX; // source  for velocity in X direction
	TUniquePtr<Fluid3D> mp_destX; // destination for velocity in X direction
	TUniquePtr<Fluid3D> mp_sourceY; // source for velocity in Y direction
	TUniquePtr<Fluid3D> mp_destY; // destination for velocity in Y direction
	TUniquePtr<Fluid3D> mp_sourceZ; // source for velocity in Z direction
	TUniquePtr<Fluid3D> mp_destZ; // destination for velocity in Z direction
	TUniquePtr<FluidProperties> mp_prop;
	int32 m_X;
	int32 m_Y;
	int32 m_Z;
};
