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

#include "AtmoPkg3D.h"

#include "CoreMinimal.h"
#include "Fluid3D.h"

#include <utility>

AtmoPkg3D::AtmoPkg3D(int32 x, int32 y, int32 z) : m_X(x), m_Y(y), m_Z(z) {
	mp_prop = MakeUnique<FluidProperties>();
	mp_prop->diffusion = 0.0f;
	mp_prop->advection = 0.0f;
	mp_prop->force = 0.0f;
	mp_prop->decay = 0.0f;
	mp_sourceO2		= MakeUnique<Fluid3D>(m_X, m_Y, m_Z);
	mp_destO2		= MakeUnique<Fluid3D>(m_X, m_Y, m_Z);
	mp_sourceN2		= MakeUnique<Fluid3D>(m_X, m_Y, m_Z);
	mp_destN2		= MakeUnique<Fluid3D>(m_X, m_Y, m_Z);
	mp_sourceCO2	= MakeUnique<Fluid3D>(m_X, m_Y, m_Z);
	mp_destCO2		= MakeUnique<Fluid3D>(m_X, m_Y, m_Z);
	mp_sourceToxin	= MakeUnique<Fluid3D>(m_X, m_Y, m_Z);
	mp_destToxin	= MakeUnique<Fluid3D>(m_X, m_Y, m_Z);
}

AtmoPkg3D::AtmoPkg3D(const AtmoPkg3D& right)
	: m_X(right.m_X), m_Y(right.m_Y), m_Z(right.m_Z)
{
	mp_sourceO2		= MakeUnique<Fluid3D>(right.m_X, right.m_Y, right.m_Z);
	mp_destO2		= MakeUnique<Fluid3D>(right.m_X, right.m_Y, right.m_Z);
	mp_sourceN2		= MakeUnique<Fluid3D>(right.m_X, right.m_Y, right.m_Z);
	mp_destN2		= MakeUnique<Fluid3D>(right.m_X, right.m_Y, right.m_Z);
	mp_sourceCO2	= MakeUnique<Fluid3D>(right.m_X, right.m_Y, right.m_Z);
	mp_destCO2		= MakeUnique<Fluid3D>(right.m_X, right.m_Y, right.m_Z);
	mp_sourceToxin	= MakeUnique<Fluid3D>(right.m_X, right.m_Y, right.m_Z);
	mp_destToxin	= MakeUnique<Fluid3D>(right.m_X, right.m_Y, right.m_Z);
	mp_prop			= MakeUnique<FluidProperties>();
	mp_prop->diffusion = right.mp_prop->diffusion;
	mp_prop->advection = right.mp_prop->advection;
	mp_prop->force = right.mp_prop->force;
	mp_prop->decay = right.mp_prop->decay;
}

AtmoPkg3D& AtmoPkg3D::operator=(const AtmoPkg3D& right) {
	if (this != &right) {
		mp_sourceO2		= MakeUnique<Fluid3D>(right.m_X, right.m_Y, right.m_Z);
		mp_destO2		= MakeUnique<Fluid3D>(right.m_X, right.m_Y, right.m_Z);
		mp_sourceN2		= MakeUnique<Fluid3D>(right.m_X, right.m_Y, right.m_Z);
		mp_destN2		= MakeUnique<Fluid3D>(right.m_X, right.m_Y, right.m_Z);
		mp_sourceCO2	= MakeUnique<Fluid3D>(right.m_X, right.m_Y, right.m_Z);
		mp_destCO2		= MakeUnique<Fluid3D>(right.m_X, right.m_Y, right.m_Z);
		mp_sourceToxin	= MakeUnique<Fluid3D>(right.m_X, right.m_Y, right.m_Z);
		mp_destToxin	= MakeUnique<Fluid3D>(right.m_X, right.m_Y, right.m_Z);
		mp_prop			= MakeUnique<FluidProperties>();
		mp_prop->diffusion = right.mp_prop->diffusion;
		mp_prop->advection = right.mp_prop->advection;
		mp_prop->force = right.mp_prop->force;
		mp_prop->decay = right.mp_prop->decay;
	}
	return *this;
}

void AtmoPkg3D::SwapLocations() {
	std::swap(mp_sourceO2, mp_destO2);
	std::swap(mp_sourceN2, mp_destN2);
	std::swap(mp_sourceCO2, mp_destCO2);
	std::swap(mp_sourceToxin, mp_destToxin);
}

void AtmoPkg3D::Reset(float v) const {
	mp_sourceO2->Set(v);
	mp_destO2->Set(v);

	mp_sourceN2->Set(v);
	mp_destN2->Set(v);

	mp_sourceCO2->Set(v);
	mp_destCO2->Set(v);

	mp_sourceToxin->Set(v);
	mp_destToxin->Set(v);
}
