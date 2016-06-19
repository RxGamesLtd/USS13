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

#include "Public/FluidSimulation.h"
#include "Fluid3D.h"
#include "UnrealMathUtility.h"

Fluid3D::Fluid3D(int32 x, int32 y, int32 z) : TArray3D(x, y, z)
{
}

// Distribute a value to the 8 grid points surrounding the floating point coordinates
// x,y,z must be 1 less than their associated max values (dimensions of array)
void Fluid3D::DistributeFloatingPoint(float x, float y, float z, float value)
{
	// Coordinate (ix, iy, iz) is the top-left-back point on cube
	auto ix = FMath::FloorToInt(x);
	auto iy = FMath::FloorToInt(y);
	auto iz = FMath::FloorToInt(z);

	// Get fractional parts of coordinates 
	float fx = x - ix;
	float fy = y - iy;
	float fz = z - iz;

	// Add appropriate fraction of value into each of the 8 cube points
	this->element(ix, iy, iz) += (1.0f - fx) * (1.0f - fy) * (1.0f - fz) * value;
	this->element(ix + 1, iy, iz) += (fx) * (1.0f - fy) * (1.0f - fz) * value;
	this->element(ix, iy + 1, iz) += (1.0f - fx) * (fy) * (1.0f - fz) * value;
	this->element(ix, iy, iz + 1) += (1.0f - fx) * (1.0f - fy) * (fz) * value;
	this->element(ix + 1, iy + 1, iz) += (fx) * (fy) * (1.0f - fz) * value;
	this->element(ix + 1, iy, iz + 1) += (fx) * (1.0f - fy) * (fz) * value;
	this->element(ix, iy + 1, iz + 1) += (1.0f - fx) * (fy) * (fz) * value;
	this->element(ix + 1, iy + 1, iz + 1) += (fx) * (fy) * (fz) * value;
}
