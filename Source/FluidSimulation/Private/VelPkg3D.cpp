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
#include "VelPkg3D.h"


VelPkg3D::VelPkg3D(int32 x, int32 y, int32 z) : m_X(x), m_Y(y), m_Z(z)
{
	mp_prop = MakeShareable(new FluidProperties());
	mp_prop->diffusion = 0.0f;
	mp_prop->advection = 0.0f;
	mp_prop->force = 0.0f;
	mp_prop->decay = 0.0f;
	mp_sourceX = MakeShareable(new Fluid3D(x, y, z));
	mp_destX = MakeShareable(new Fluid3D(x, y, z));
	mp_sourceY = MakeShareable(new Fluid3D(x, y, z));
	mp_destY = MakeShareable(new Fluid3D(x, y, z));
	mp_sourceZ = MakeShareable(new Fluid3D(x, y, z));
	mp_destZ = MakeShareable(new Fluid3D(x, y, z));
}


VelPkg3D& VelPkg3D::operator=(const VelPkg3D& right)
{
	if (this != &right)
	{
		mp_sourceX = MakeShareable(new Fluid3D(right.m_X, right.m_Y, right.m_Z));
		mp_destX = MakeShareable(new Fluid3D(right.m_X, right.m_Y, right.m_Z));
		mp_sourceY = MakeShareable(new Fluid3D(right.m_X, right.m_Y, right.m_Z));
		mp_destY = MakeShareable(new Fluid3D(right.m_X, right.m_Y, right.m_Z));
		mp_sourceZ = MakeShareable(new Fluid3D(right.m_X, right.m_Y, right.m_Z));
		mp_destZ = MakeShareable(new Fluid3D(right.m_X, right.m_Y, right.m_Z));
		mp_prop = MakeShareable(new FluidProperties());
		mp_prop->diffusion = right.mp_prop->diffusion;
		mp_prop->advection = right.mp_prop->advection;
		mp_prop->force = right.mp_prop->force;
		mp_prop->decay = right.mp_prop->decay;
	}
	return *this;
}


void VelPkg3D::SwapLocationsX()
{
	auto temp = mp_sourceX;
	mp_sourceX = mp_destX;
	mp_destX = temp;
}


void VelPkg3D::SwapLocationsY()
{
	auto temp = mp_sourceY;
	mp_sourceY = mp_destY;
	mp_destY = temp;
}


void VelPkg3D::SwapLocationsZ()
{
	auto temp = mp_sourceZ;
	mp_sourceZ = mp_destZ;
	mp_destZ = temp;
}


void VelPkg3D::Reset(float v) const
{
	mp_sourceX->Set(v);
	mp_destX->Set(v);
	mp_sourceY->Set(v);
	mp_destY->Set(v);
	mp_sourceZ->Set(v);
	mp_destZ->Set(v);
}

// Accessor methods
TSharedPtr<Fluid3D, ESPMode::ThreadSafe> VelPkg3D::SourceX() const
{
	return mp_sourceX;
}

TSharedPtr<Fluid3D, ESPMode::ThreadSafe> VelPkg3D::DestinationX() const
{
	return mp_destX;
}

TSharedPtr<Fluid3D, ESPMode::ThreadSafe> VelPkg3D::SourceY() const
{
	return mp_sourceY;
}

TSharedPtr<Fluid3D, ESPMode::ThreadSafe> VelPkg3D::DestinationY() const
{
	return mp_destY;
}

TSharedPtr<Fluid3D, ESPMode::ThreadSafe> VelPkg3D::SourceZ() const
{
	return mp_sourceZ;
}

TSharedPtr<Fluid3D, ESPMode::ThreadSafe> VelPkg3D::DestinationZ() const
{
	return mp_destZ;
}

TSharedPtr<FluidProperties, ESPMode::ThreadSafe> VelPkg3D::Properties() const
{
	return mp_prop;
}
