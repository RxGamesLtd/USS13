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

#include "Fluid3D.h"
#include "FluidProperties.h"

// Class consists of a 'source' and 'destination' Fluid3D object and defines
// the properties they share
class FluidPkg3D
{
public:

	// Constructor - Initilizes source and destination FLuid3D objects
	FluidPkg3D(int32 xSize, int32 ySize, int32 zSize);

	// Assignment Operator
	FluidPkg3D& operator= (const FluidPkg3D& right);

	// Swap the source and destination objects 
	void SwapLocations();

	// Reset the source and destination objects to specified value
	void Reset(float v) const;

	// Accessors
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> Source() const;
    TSharedPtr<Fluid3D, ESPMode::ThreadSafe> Destination() const;
	TSharedPtr<FluidProperties, ESPMode::ThreadSafe> Properties() const;

private:
    TSharedPtr<Fluid3D, ESPMode::ThreadSafe> mp_source;       // source 
    TSharedPtr<Fluid3D, ESPMode::ThreadSafe> mp_dest;         // destination

	TSharedPtr<FluidProperties, ESPMode::ThreadSafe> mp_prop; // defines properties of fluid 

	int32 m_X;				  // x dimension
	int32 m_Y;                  // y dimension
	int32 m_Z;                  // z dimension
};
