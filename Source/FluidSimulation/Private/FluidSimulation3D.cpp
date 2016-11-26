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

#include "FluidSimulation.h"
#include "FluidSimulation3D.h"

DEFINE_STAT(STAT_AtmosUpdatesCount);

FluidSimulation3D::FluidSimulation3D(int32 xSize, int32 ySize, int32 zSize, float dt)
	: m_diffusionIter(1), m_vorticity(0.0), m_pressureAccel(0.0), m_dt(dt), m_size_x(xSize), m_size_y(ySize),
	  m_size_z(zSize) {
	m_curl = MakeShareable(new Fluid3D(xSize, ySize, zSize));
	mp_velocity = MakeShareable(new VelPkg3D(xSize, ySize, zSize));
	mp_pressure = MakeShareable(new AtmoPkg3D(xSize, ySize, zSize));
	mp_ink = MakeShareable(new FluidPkg3D(xSize, ySize, zSize));
	mp_heat = MakeShareable(new FluidPkg3D(xSize, ySize, zSize));
	m_solids = MakeShareable(new TArray3D<bool>(xSize, ySize, zSize));

	Reset();
}

// Update is called every frame or as specified in the max desired updates per second GUI slider
void FluidSimulation3D::Update() const {
	SCOPE_CYCLE_COUNTER(STAT_AtmosUpdatesCount);
	UpdateDiffusion();
	UpdateForces();
	UpdateAdvection();
}

// Apply diffusion across the grids
void FluidSimulation3D::UpdateDiffusion() const {
	// Skip diffusion if disabled
	// Diffusion of Velocity
	if (mp_velocity->Properties()->diffusion != 0.0f) {
		const auto scaledVelocity = mp_velocity->Properties()->diffusion / static_cast<float>(m_diffusionIter);

		for (int32 i = 0; i < m_diffusionIter; i++) {
			Diffusion(mp_velocity->SourceX(), mp_velocity->DestinationX(), scaledVelocity);
			mp_velocity->SwapLocationsX();
			Diffusion(mp_velocity->SourceY(), mp_velocity->DestinationY(), scaledVelocity);
			mp_velocity->SwapLocationsY();
			Diffusion(mp_velocity->SourceZ(), mp_velocity->DestinationZ(), scaledVelocity);
			mp_velocity->SwapLocationsZ();
		}
	}

	// Diffusion of Pressure
	if (!FMath::IsNearlyZero(mp_pressure->Properties()->diffusion)) {
		const auto scaledDiffusion = mp_pressure->Properties()->diffusion / static_cast<float>(m_diffusionIter);
		for (int32 i = 0; i < m_diffusionIter; i++) {
			DiffusionStable(mp_pressure->SourceO2(), mp_pressure->DestinationO2(), scaledDiffusion);
			DiffusionStable(mp_pressure->SourceCO2(), mp_pressure->DestinationCO2(), scaledDiffusion);
			DiffusionStable(mp_pressure->SourceN2(), mp_pressure->DestinationN2(), scaledDiffusion);
			DiffusionStable(mp_pressure->SourceToxin(), mp_pressure->DestinationToxin(), scaledDiffusion);
			mp_pressure->SwapLocations();
		}
	}

	// Diffusion of Heat
	if (!FMath::IsNearlyZero(mp_heat->Properties()->diffusion)) {
		const float scaledHeat = mp_heat->Properties()->diffusion / static_cast<float>(m_diffusionIter);
		for (int32 i = 0; i < m_diffusionIter; i++) {
			Diffusion(mp_heat->Source(), mp_heat->Destination(), scaledHeat);
			mp_heat->SwapLocations();
		}
	}

	// Diffusion of Ink
	if (!FMath::IsNearlyZero(mp_ink->Properties()->diffusion)) {
		const float scaledInk = mp_ink->Properties()->diffusion / static_cast<float>(m_diffusionIter);

		for (int32 i = 0; i < m_diffusionIter; i++) {
			Diffusion(mp_ink->Source(), mp_ink->Destination(), scaledInk);
			mp_ink->SwapLocations();
		}
	}
}

// Apply the effects of heat to the velocity grid
void FluidSimulation3D::Heat(float scale) const {
	const auto force = m_dt * scale;

	*mp_velocity->DestinationX() = *mp_velocity->SourceX();
	*mp_velocity->DestinationY() = *mp_velocity->SourceY();
	*mp_velocity->DestinationZ() = *mp_velocity->SourceZ();

	for (int32 x = 0; x < m_size_x - 1; x++) {
		for (int32 y = 0; y < m_size_y - 1; y++) {
			for (int32 z = 0; z < m_size_z - 1; z++) {
				// Pressure differential between points to get an accelleration force.
				auto force_x = mp_heat->Source()->element(x, y, z) - mp_heat->Source()->element(x + 1, y, z);
				auto force_y = mp_heat->Source()->element(x, y, z) - mp_heat->Source()->element(x, y + 1, z);
				auto force_z = mp_heat->Source()->element(x, y, z) - mp_heat->Source()->element(x, y, z + 1);

				// Use the acceleration force to move the velocity field in the appropriate direction.
				// Ex. If an area of high pressure exists the acceleration force will turn the velocity field
				// away from this area
				mp_velocity->DestinationX()->element(x, y, z) += force * force_x;
				mp_velocity->DestinationX()->element(x + 1, y, z) += force * force_x;

				mp_velocity->DestinationY()->element(x, y, z) += force * force_y;
				mp_velocity->DestinationY()->element(x, y + 1, z) += force * force_y;

				mp_velocity->DestinationZ()->element(x, y, z) += force * force_z;
				mp_velocity->DestinationZ()->element(x, y, z + 1) += force * force_z;
			}
		}
	}

	mp_velocity->SwapLocationsX();
	mp_velocity->SwapLocationsY();
	mp_velocity->SwapLocationsZ();
}

// Apply forces across the grids
void FluidSimulation3D::UpdateForces() const {
	// Apply upwards force on velocity from ink rising under its own steam
	if (!FMath::IsNearlyZero(mp_ink->Properties()->force)) {
		*mp_velocity->SourceY() += *mp_ink->Source() * mp_ink->Properties()->force;
	}

	// Apply upwards force on velocity field from heat
	if (!FMath::IsNearlyZero(mp_heat->Properties()->force)) {
		Heat(mp_heat->Properties()->force);

		if (!FMath::IsNearlyZero(mp_heat->Properties()->decay)) {
			ExponentialDecay(mp_heat->Source(), mp_heat->Properties()->decay);
		}
	}

	// Apply dampening force on velocity due to viscosity
	if (!FMath::IsNearlyZero(mp_velocity->Properties()->decay)) {
		ExponentialDecay(mp_velocity->SourceX(), mp_velocity->Properties()->decay);
		ExponentialDecay(mp_velocity->SourceY(), mp_velocity->Properties()->decay);
		ExponentialDecay(mp_velocity->SourceZ(), mp_velocity->Properties()->decay);
	}

	// Apply equilibrium force on pressure for mass conservation
	if (!FMath::IsNearlyZero(m_pressureAccel)) {
		PressureAcceleration(m_pressureAccel);
	}

	// Apply curl force on vorticies to prevent artificial dampening
	if (!FMath::IsNearlyZero(m_vorticity)) {
		VorticityConfinement(m_vorticity);
	}
}

// Apply advection across the grids
void FluidSimulation3D::UpdateAdvection() const {
	auto avg_dimension = (m_size_x + m_size_y + m_size_z) / 3.0f;
	const auto std_dimension = 100.0f;

	// Change advection scale depending on grid size. Smaller grids means larger cells, so scale should be smaller.
	// Average dimension size of std_dimension value (100) equals an advection_scale of 1
	auto advection_scale = avg_dimension / std_dimension;

	mp_ink->Destination()->Set(1.0f);

	// Advect the ink
	if (mp_ink->Properties()->advection > 0) {
		ForwardAdvection(mp_ink->Source(), mp_ink->Destination(), mp_ink->Properties()->advection * advection_scale);
		mp_ink->SwapLocations();
		ReverseAdvection(mp_ink->Source(), mp_ink->Destination(), mp_ink->Properties()->advection * advection_scale);
		mp_ink->SwapLocations();
	}

	// Only advect the heat if it is applying a force
	float in = mp_heat->Properties()->force;
	if (!FMath::IsNearlyZero(in)) {
		ForwardAdvection(mp_heat->Source(), mp_heat->Destination(), mp_heat->Properties()->advection * advection_scale);
		mp_heat->SwapLocations();
		ReverseAdvection(mp_heat->Source(), mp_heat->Destination(), mp_heat->Properties()->advection * advection_scale);
		mp_heat->SwapLocations();
	}

	// Advection order makes significant differences
	// Advecting pressure first leads to self-maintaining waves and ripple artifacts
	// Advecting velocity first naturally dissipates the waves

	// Advect Velocity
	ForwardAdvection(mp_velocity->SourceX(), mp_velocity->DestinationX(),
	                 mp_velocity->Properties()->advection * advection_scale);
	ForwardAdvection(mp_velocity->SourceY(), mp_velocity->DestinationY(),
	                 mp_velocity->Properties()->advection * advection_scale);
	ForwardAdvection(mp_velocity->SourceZ(), mp_velocity->DestinationZ(),
	                 mp_velocity->Properties()->advection * advection_scale);

	ReverseSignedAdvection(mp_velocity, advection_scale);

	// Take care of boundries
	InvertVelocityEdges();

	// Advect Pressure. Represents compressible fluid
	ForwardAdvection(mp_pressure->SourceO2(), mp_pressure->DestinationO2(),
	                 mp_pressure->Properties()->advection * advection_scale);
	ForwardAdvection(mp_pressure->SourceN2(), mp_pressure->DestinationN2(),
	                 mp_pressure->Properties()->advection * advection_scale);
	ForwardAdvection(mp_pressure->SourceCO2(), mp_pressure->DestinationCO2(),
	                 mp_pressure->Properties()->advection * advection_scale);
	ForwardAdvection(mp_pressure->SourceToxin(), mp_pressure->DestinationToxin(),
	                 mp_pressure->Properties()->advection * advection_scale);
	mp_pressure->SwapLocations();
	ReverseAdvection(mp_pressure->SourceO2(), mp_pressure->DestinationO2(),
	                 mp_pressure->Properties()->advection * advection_scale);
	ReverseAdvection(mp_pressure->SourceN2(), mp_pressure->DestinationN2(),
	                 mp_pressure->Properties()->advection * advection_scale);
	ReverseAdvection(mp_pressure->SourceCO2(), mp_pressure->DestinationCO2(),
	                 mp_pressure->Properties()->advection * advection_scale);
	ReverseAdvection(mp_pressure->SourceToxin(), mp_pressure->DestinationToxin(),
	                 mp_pressure->Properties()->advection * advection_scale);
	mp_pressure->SwapLocations();
}

void FluidSimulation3D::ForwardAdvection(TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_in,
                                         TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_out, float scale) const {
	auto force = m_dt * scale; // distance to advect
	float vx, vy, vz; // velocity values of the current x,y,z locations
	float x1, y1, z1; // x, y, z locations after advection
	int32 x1A, y1A, z1A; // x, y, z locations of top-left-back grid point (A) after advection
	float fx1, fy1, fz1; // fractional remainders of x1, y1, z1
	float source_value; // original source value
	float A, // top-left-back grid point value after advection
	      B, // top-right-back grid point value after advection
	      C, // bottom-left-back grid point value after advection
	      D, // bottom-right-back grid point value after advection
	      E, // top-left-front grid point value after advection
	      F, // top-right-front grid point value after advection
	      G, // bottom-left-front grid point value after advection
	      H; // bottom-right-front grid point value after advection

	//
	//    A_________B
	//    |\        |\
	//    | \E______|_\F
	//    |  |      |  |
	//    |  |      |  |
	//    C--|------D  |
	//     \ |       \ |
	//      \|G_______\H
	//

	// Copy source to destination as forward advection results in adding/subtracing not moving
	*p_out = *p_in;

	if (FMath::IsNearlyZero(force)) {
		return;
	}

	// This can easily be threaded as the input array is independent from the output array
	for (int32 x = 1; x < m_size_x - 1; x++) {
		for (int32 y = 1; y < m_size_y - 1; y++) {
			for (int32 z = 1; z < m_size_z - 1; z++) {
				vx = mp_velocity->SourceX()->element(x, y, z);
				vy = mp_velocity->SourceY()->element(x, y, z);
				vz = mp_velocity->SourceZ()->element(x, y, z);
				if (!FMath::IsNearlyZero(vx) || !FMath::IsNearlyZero(vy) || !FMath::IsNearlyZero(vz)) {
					// Find the floating point location of the forward advection
					x1 = x + vx * force;
					y1 = y + vy * force;
					z1 = z + vz * force;

					// Check for and correct boundary collisions
					Collide(x, y, z, x1, y1, z1);

					// Find the nearest top-left integer grid point of the advection
					x1A = FMath::FloorToInt(x1);
					y1A = FMath::FloorToInt(y1);
					z1A = FMath::FloorToInt(z1);

					// Store the fractional parts
					fx1 = x1 - x1A;
					fy1 = y1 - y1A;
					fz1 = z1 - z1A;

					// The floating point location after forward advection (x1,y1,z1) will land within an 8 point cube (A,B,C,D,E,F,G,H).
					// Distribute the value of the source point among the destination grid points using bilinear interoplation.
					// Subtract the total value given to the destination grid points from the source point.


					// Pull source value from the unmodified p_in
					source_value = p_in->element(x, y, z);

					// Bilinear interpolation
					A = (1.0f - fz1) * (1.0f - fy1) * (1.0f - fx1) * source_value;
					B = (1.0f - fz1) * (1.0f - fy1) * (fx1) * source_value;
					C = (1.0f - fz1) * (fy1) * (1.0f - fx1) * source_value;
					D = (1.0f - fz1) * (fy1) * (fx1) * source_value;
					E = (fz1) * (1.0f - fy1) * (1.0f - fx1) * source_value;
					F = (fz1) * (1.0f - fy1) * (fx1) * source_value;
					G = (fz1) * (fy1) * (1.0f - fx1) * source_value;
					H = (fz1) * (fy1) * (fx1) * source_value;

					// Add A,B,C,D,E,F,G,H to the eight destination cells
					p_out->element(x1A, y1A, z1A) += A;
					p_out->element(x1A + 1, y1A, z1A) += B;
					p_out->element(x1A, y1A + 1, z1A) += C;
					p_out->element(x1A + 1, y1A + 1, z1A) += D;
					p_out->element(x1A, y1A, z1A + 1) += E;
					p_out->element(x1A + 1, y1A, z1A + 1) += F;
					p_out->element(x1A, y1A + 1, z1A + 1) += G;
					p_out->element(x1A + 1, y1A + 1, z1A + 1) += H;

					// Subtract A-H from source for mass conservation
					p_out->element(x, y, z) -= (A + B + C + D + E + F + G + H);
				}
			}
		}
	}
}

void FluidSimulation3D::ReverseAdvection(TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_in,
                                         TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_out, float scale) const {
	auto force = m_dt * scale; // distance to advect
	float vx, vy, vz; // velocity values of the current x,y,z locations
	float x1, y1, z1; // x, y, z locations after advection
	int32 x1A, y1A, z1A; // x, y, z locations of top-left-back grid point (A) after advection
	float fx1, fy1, fz1; // fractional remainders of x1, y1, z1
	float A, // top-left-back grid point value after advection
	      B, // top-right-back grid point value after advection
	      C, // bottom-left-back grid point value after advection
	      D, // bottom-right-back grid point value after advection
	      E, // top-left-front grid point value after advection
	      F, // top-right-front grid point value after advection
	      G, // bottom-left-front grid point value after advection
	      H; // bottom-right-front grid point value after advection
	float A_Total, B_Total, C_Total, D_Total,
	      E_Total, F_Total, G_Total, H_Total; // Total fraction being requested by the 8 grid points after entire grid has been advected

	// Copy source to destination as reverse advection results in adding/subtracing not moving
	*p_out = *p_in;

	if (FMath::IsNearlyZero(force)) {
		return;
	}

	// we need to zero out the fractions
	TArray3D<int32> FromSource_xA(m_size_x, m_size_y, m_size_z,
	                              -1); // The new X coordinate after advection stored in x,y,z where x,y,z,z is the original source point
	TArray3D<int32> FromSource_yA(m_size_x, m_size_y, m_size_z,
	                              -1); // The new Y coordinate after advection stored in x,y,z where x,y,z is the original source point
	TArray3D<int32> FromSource_zA(m_size_x, m_size_y, m_size_z,
	                              -1); // The new Z coordinate after advection stored in x,y,z where x,y,z is the original source point
	TArray3D<float> FromSource_A(m_size_x, m_size_y,
	                             m_size_z); // The value of A after advection stored in x,y,z where x,y,z is the original source point
	TArray3D<float> FromSource_B(m_size_x, m_size_y,
	                             m_size_z); // The value of B after advection stored in x,y,z where x,y,z is the original source point
	TArray3D<float> FromSource_C(m_size_x, m_size_y,
	                             m_size_z); // The value of C after advection stored in x,y,z where x,y,z is the original source point
	TArray3D<float> FromSource_D(m_size_x, m_size_y,
	                             m_size_z); // The value of D after advection stored in x,y,z where x,y,z is the original source point
	TArray3D<float> FromSource_E(m_size_x, m_size_y,
	                             m_size_z); // The value of E after advection stored in x,y,z where x,y,z is the original source point
	TArray3D<float> FromSource_F(m_size_x, m_size_y,
	                             m_size_z); // The value of F after advection stored in x,y,z where x,y,z is the original source point
	TArray3D<float> FromSource_G(m_size_x, m_size_y,
	                             m_size_z); // The value of G after advection stored in x,y,z where x,y,z is the original source point
	TArray3D<float> FromSource_H(m_size_x, m_size_y,
	                             m_size_z); // The value of H after advection stored in x,y,z where x,y,z is the original source point
	TArray3D<float> TotalDestValue(m_size_x, m_size_y,
	                               m_size_z); // The total accumulated value after advection stored in x,y,z where x,y,z is the destination point

	// This can easily be threaded as the input array is independent from the output array
	for (int32 x = 1; x < m_size_x - 1; x++) {
		for (int32 y = 1; y < m_size_y - 1; y++) {
			for (int32 z = 1; z < m_size_z - 1; z++) {
				vx = mp_velocity->SourceX()->element(x, y, z);
				vy = mp_velocity->SourceY()->element(x, y, z);
				vz = mp_velocity->SourceZ()->element(x, y, z);
				if (!FMath::IsNearlyZero(vx) || !FMath::IsNearlyZero(vy) || !FMath::IsNearlyZero(vz)) {
					// Find the floating point location of the advection
					x1 = x + vx * force;
					y1 = y + vy * force;
					z1 = z + vz * force;

					// Check for and correct boundary collisions
					Collide(x, y, z, x1, y1, z1);

					// Find the nearest top-left integer grid point of the advection
					x1A = FMath::FloorToInt(x1);
					y1A = FMath::FloorToInt(y1);
					z1A = FMath::FloorToInt(z1);

					// Store the fractional parts
					fx1 = x1 - x1A;
					fy1 = y1 - y1A;
					fz1 = z1 - z1A;

					/*
					A_________B
					|\        |\
					| \E______|_\F
					|  |      |  |
					|  |      |  |
					C--|------D  |
					\ |       \ |
					\|G_______\H


					From Mick West:
					By adding the source value into the destination, we handle the problem of multiple destinations
					but by subtracting it from the source we gloss over the problem of multiple sources.
					Suppose multiple destinations have the same (partial) source cells, then what happens is the first dest that
					is processed will get all of that source cell (or all of the fraction it needs).  Subsequent dest
					cells will get a reduced fraction.  In extreme cases this will lead to holes forming based on
					the update order.

					Solution:  Maintain an array for dest cells, and source cells.
					For dest cells, store the eight source cells and the eight fractions
					For source cells, store the number of dest cells that source from here, and the total fraction
					E.G.  Dest cells A, B, C all source from cell D (and explicit others XYZ, which we don't need to store)
					So, dest cells store A->D(0.1)XYZ..., B->D(0.5)XYZ.... C->D(0.7)XYZ...
					Source Cell D is updated with A, B then C
					Update A:   Dests = 1, Tot = 0.1
					Update B:   Dests = 2, Tot = 0.6
					Update C:   Dests = 3, Tot = 1.3

					How much should go to each of A, B and C? They are asking for a total of 1.3, so should they get it all, or
					should they just get 0.4333 in total?
					Ad Hoc answer:
					if total <=1 then they get what they ask for
					if total >1 then is is divided between them proportionally.
					If there were two at 1.0, they would get 0.5 each
					If there were two at 0.5, they would get 0.5 each
					If there were two at 0.1, they would get 0.1 each
					If there were one at 0.6 and one at 0.8, they would get 0.6/1.4 and 0.8/1.4  (0.429 and 0.571) each

					So in our example, total is 1.3,
					A gets 0.1/1.3, B gets 0.6/1.3 C gets 0.7/1.3, all totalling 1.0

					*/
					// Bilinear interpolation
					A = (1.0f - fz1) * (1.0f - fy1) * (1.0f - fx1);
					B = (1.0f - fz1) * (1.0f - fy1) * (fx1);
					C = (1.0f - fz1) * (fy1) * (1.0f - fx1);
					D = (1.0f - fz1) * (fy1) * (fx1);
					E = (fz1) * (1.0f - fy1) * (1.0f - fx1);
					F = (fz1) * (1.0f - fy1) * (fx1);
					G = (fz1) * (fy1) * (1.0f - fx1);
					H = (fz1) * (fy1) * (fx1);


					// Store the coordinates of destination point A for this source point (x,y,z)
					FromSource_xA.element(x, y, z) = x1A;
					FromSource_yA.element(x, y, z) = y1A;
					FromSource_zA.element(x, y, z) = z1A;

					// Store the values of A,B,C,D,E,F,G,H for this source point
					FromSource_A.element(x, y, z) = A;
					FromSource_B.element(x, y, z) = B;
					FromSource_C.element(x, y, z) = C;
					FromSource_D.element(x, y, z) = D;
					FromSource_E.element(x, y, z) = E;
					FromSource_F.element(x, y, z) = F;
					FromSource_G.element(x, y, z) = G;
					FromSource_H.element(x, y, z) = H;

					// Accumullting the total value for the four destinations
					TotalDestValue.element(x1A, y1A, z1A) += A;
					TotalDestValue.element(x1A + 1, y1A, z1A) += B;
					TotalDestValue.element(x1A, y1A + 1, z1A) += C;
					TotalDestValue.element(x1A + 1, y1A + 1, z1A) += D;
					TotalDestValue.element(x1A, y1A, z1A + 1) += E;
					TotalDestValue.element(x1A + 1, y1A, z1A + 1) += F;
					TotalDestValue.element(x1A, y1A + 1, z1A + 1) += G;
					TotalDestValue.element(x1A + 1, y1A + 1, z1A + 1) += H;
				}
			}
		}
	}

	for (int32 y = 1; y < m_size_y - 1; y++) {
		for (int32 x = 1; x < m_size_x - 1; x++) {
			for (int32 z = 1; z < m_size_z - 1; z++) {
				if (FromSource_xA.element(x, y, z) != -1) {
					// Get the coordinates of A
					x1A = FromSource_xA.element(x, y, z);
					y1A = FromSource_yA.element(x, y, z);
					z1A = FromSource_zA.element(x, y, z);

					// Get the four fractional amounts we earlier interpolated
					A = FromSource_A.element(x, y, z);
					B = FromSource_B.element(x, y, z);
					C = FromSource_C.element(x, y, z);
					D = FromSource_D.element(x, y, z);
					E = FromSource_E.element(x, y, z);
					F = FromSource_F.element(x, y, z);
					G = FromSource_G.element(x, y, z);
					H = FromSource_H.element(x, y, z);

					// Get the TOTAL fraction requested from each source cell
					A_Total = TotalDestValue.element(x1A, y1A, z1A);
					B_Total = TotalDestValue.element(x1A + 1, y1A, z1A);
					C_Total = TotalDestValue.element(x1A, y1A + 1, z1A);
					D_Total = TotalDestValue.element(x1A + 1, y1A + 1, z1A);
					E_Total = TotalDestValue.element(x1A, y1A, z1A + 1);
					F_Total = TotalDestValue.element(x1A + 1, y1A, z1A + 1);
					G_Total = TotalDestValue.element(x1A, y1A + 1, z1A + 1);
					H_Total = TotalDestValue.element(x1A + 1, y1A + 1, z1A + 1);

					// If less then 1.0 in total then no scaling is neccessary
					if (A_Total < 1.0f) A_Total = 1.0f;
					if (B_Total < 1.0f) B_Total = 1.0f;
					if (C_Total < 1.0f) C_Total = 1.0f;
					if (D_Total < 1.0f) D_Total = 1.0f;
					if (E_Total < 1.0f) E_Total = 1.0f;
					if (F_Total < 1.0f) F_Total = 1.0f;
					if (G_Total < 1.0f) G_Total = 1.0f;
					if (H_Total < 1.0f) H_Total = 1.0f;

					// Scale the amount we are transferring
					A /= A_Total;
					B /= B_Total;
					C /= C_Total;
					D /= D_Total;
					E /= E_Total;
					F /= F_Total;
					G /= G_Total;
					H /= H_Total;

					// Give the fraction of the original source, do not alter the original
					// So we are taking fractions from p_in, but not altering those values as they are used again by later cells
					// if the field were mass conserving, then we could simply move the value but if we try that we lose mass
					p_out->element(x, y, z) += A * p_in->element(x1A, y1A, z1A) +
							B * p_in->element(x1A + 1, y1A, z1A) +
							C * p_in->element(x1A, y1A + 1, z1A) +
							D * p_in->element(x1A + 1, y1A + 1, z1A) +
							E * p_in->element(x1A, y1A, z1A + 1) +
							F * p_in->element(x1A + 1, y1A, z1A + 1) +
							G * p_in->element(x1A, y1A + 1, z1A + 1) +
							H * p_in->element(x1A + 1, y1A + 1, z1A + 1);

					// Subtract the values added to the destination from the source for mass conservation
					p_out->element(x1A, y1A, z1A) -= A * p_in->element(x1A, y1A, z1A);
					p_out->element(x1A + 1, y1A, z1A) -= B * p_in->element(x1A + 1, y1A, z1A);
					p_out->element(x1A, y1A + 1, z1A) -= C * p_in->element(x1A, y1A + 1, z1A);
					p_out->element(x1A + 1, y1A + 1, z1A) -= D * p_in->element(x1A + 1, y1A + 1, z1A);
					p_out->element(x1A, y1A, z1A + 1) -= E * p_in->element(x1A, y1A, z1A + 1);
					p_out->element(x1A + 1, y1A, z1A + 1) -= F * p_in->element(x1A + 1, y1A, z1A + 1);
					p_out->element(x1A, y1A + 1, z1A + 1) -= G * p_in->element(x1A, y1A + 1, z1A + 1);
					p_out->element(x1A + 1, y1A + 1, z1A + 1) -= H * p_in->element(x1A + 1, y1A + 1, z1A + 1);
				}
			}
		}
	}
}

void FluidSimulation3D::Diffusion(TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_in,
                                  TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_out, const float scale) const {
	float force = m_dt * scale;

	if (FMath::IsNegativeFloat(force) || FMath::IsNearlyZero(force))
		return;

	// Iterate through cells along the top and bottom surfaces (not including corners).  5 neighbors total.
	for (int32 x = 1; x < m_size_x - 1; x++) {
		for (int32 z = 1; z < m_size_z - 1; z++) {
			p_out->element(x, 0, z) = p_in->element(x, 0, z) + force *
			(p_in->element(x - 1, 0, z) +
				p_in->element(x + 1, 0, z) +
				p_in->element(x, 0, z - 1) +
				p_in->element(x, 0, z + 1) +
				p_in->element(x, 1, z) - 5.0f * p_in->element(x, 0, z));
			p_out->element(x, m_size_y - 1, z) = p_in->element(x, m_size_y - 1, z) + force *
			(p_in->element(x - 1, m_size_y - 1,
			               z) +
				p_in->element(x + 1, m_size_y - 1,
				              z) +
				p_in->element(x, m_size_y - 1,
				              z - 1) +
				p_in->element(x, m_size_y - 1,
				              z + 1) +
				p_in->element(x, m_size_y - 2,
				              z) - 5.0f *
				p_in->element(
					x,
					m_size_y -
					1, z));
		}
	}
	// Iterate through cells along the left and right surfaces (not including corners).  5 neighbors total.
	for (int32 y = 1; y < m_size_y - 1; y++) {
		for (int32 z = 1; z < m_size_z - 1; z++) {
			p_out->element(0, y, z) = p_in->element(0, y, z) + force *
			(p_in->element(0, y - 1, z) +
				p_in->element(0, y + 1, z) +
				p_in->element(0, y, z - 1) +
				p_in->element(0, y, z + 1) +
				p_in->element(1, y, z) - 5.0f * p_in->element(0, y, z));
			p_out->element(m_size_x - 1, y, z) = p_in->element(m_size_x - 1, y, z) + force *
			(p_in->element(m_size_x - 1, y - 1,
			               z) +
				p_in->element(m_size_x - 1, y + 1,
				              z) +
				p_in->element(m_size_x - 1, y,
				              z - 1) +
				p_in->element(m_size_x - 1, y,
				              z + 1) +
				p_in->element(m_size_x - 2, y,
				              z) - 5.0f *
				p_in->element(
					m_size_x -
					1, y,
					z));
		}
	}

	// Iterate through cells along the front and back surfaces (not including corners).  5 neighbors total.
	for (int32 x = 1; x < m_size_x - 1; x++) {
		for (int32 y = 1; y < m_size_y - 1; y++) {
			p_out->element(x, y, 0) = p_in->element(x, y, 0) + force *
			(p_in->element(x, y - 1, 0) +
				p_in->element(x, y + 1, 0) +
				p_in->element(x - 1, y, 0) +
				p_in->element(x + 1, y, 0) +
				p_in->element(x, y, 1) - 5.0f * p_in->element(x, y, 0));
			p_out->element(x, y, m_size_z - 1) = p_in->element(x, y, m_size_z - 1) + force *
			(p_in->element(x, y - 1,
			               m_size_z - 1) +
				p_in->element(x, y + 1,
				              m_size_z - 1) +
				p_in->element(x - 1, y,
				              m_size_z - 1) +
				p_in->element(x + 1, y,
				              m_size_z - 1) +
				p_in->element(x, y,
				              m_size_z - 2) -
				5.0f * p_in->element(x, y,
				                     m_size_z -
				                     1));
		}
	}

	// Iterate through cells along the x axis.  4 neighbors total.
	for (int32 x = 1; x < m_size_x - 1; x++) {
		p_out->element(x, 0, 0) = p_in->element(x, 0, 0) + force *
		(p_in->element(x - 1, 0, 0) +
			p_in->element(x + 1, 0, 0) +
			p_in->element(x, 0, 1) +
			p_in->element(x, 1, 0) - 4.0f * p_in->element(x, 0, 0));
		p_out->element(x, m_size_y - 1, 0) = p_in->element(x, m_size_y - 1, 0) + force *
		(p_in->element(x - 1, m_size_y - 1,
		               0) +
			p_in->element(x + 1, m_size_y - 1,
			              0) +
			p_in->element(x, m_size_y - 2, 0) +
			p_in->element(x, m_size_y - 1, 1) -
			4.0f *
			p_in->element(x, m_size_y - 1, 0));
		p_out->element(x, 0, m_size_z - 1) = p_in->element(x, 0, m_size_z - 1) + force *
		(p_in->element(x - 1, 0,
		               m_size_z - 1) +
			p_in->element(x + 1, 0,
			              m_size_z - 1) +
			p_in->element(x, 0, m_size_z - 2) +
			p_in->element(x, 1, m_size_z - 1) -
			4.0f *
			p_in->element(x, 0, m_size_z - 1));
		p_out->element(x, m_size_y - 1, m_size_z - 1) = p_in->element(x, m_size_y - 1, m_size_z - 1) + force *
		(p_in->element(
				x - 1,
				m_size_y -
				1,
				m_size_z -
				1) +
			p_in->element(
				x + 1,
				m_size_y -
				1,
				m_size_z -
				1) +
			p_in->element(x,
			              m_size_y -
			              2,
			              m_size_z -
			              1) +
			p_in->element(x,
			              m_size_y -
			              1,
			              m_size_z -
			              2) -
			4.0f *
			p_in->element(x,
			              m_size_y -
			              1,
			              m_size_z -
			              1));
	}

	// Iterate through cells along the y axis.  4 neighbors total.
	for (int32 y = 1; y < m_size_y - 1; y++) {
		p_out->element(0, y, 0) = p_in->element(0, y, 0) + force *
		(p_in->element(0, y - 1, 0) +
			p_in->element(0, y + 1, 0) +
			p_in->element(0, y, 1) +
			p_in->element(1, y, 0) - 4.0f * p_in->element(0, y, 0));
		p_out->element(m_size_x - 1, y, 0) = p_in->element(m_size_x - 1, y, 0) + force *
		(p_in->element(m_size_x - 1, y - 1,
		               0) +
			p_in->element(m_size_x - 1, y + 1,
			              0) +
			p_in->element(m_size_x - 1, y, 1) +
			p_in->element(m_size_x - 2, y, 0) -
			4.0f *
			p_in->element(m_size_x - 1, y, 0));
		p_out->element(0, y, m_size_z - 1) = p_in->element(0, y, m_size_z - 1) + force *
		(p_in->element(0, y - 1,
		               m_size_z - 1) +
			p_in->element(0, y + 1,
			              m_size_z - 1) +
			p_in->element(0, y, m_size_z - 2) +
			p_in->element(1, y, m_size_z - 1) -
			4.0f *
			p_in->element(0, y, m_size_z - 1));
		p_out->element(m_size_x - 1, y, m_size_z - 1) = p_in->element(m_size_x - 1, y, m_size_z - 1) + force *
		(p_in->element(
				m_size_x -
				1, y - 1,
				m_size_z -
				1) +
			p_in->element(
				m_size_x -
				1,
				y + 1,
				m_size_z -
				1) +
			p_in->element(
				m_size_x -
				1, y,
				m_size_z -
				2) +
			p_in->element(
				m_size_x -
				2, y,
				m_size_z -
				1) -
			4.0f *
			p_in->element(
				m_size_x -
				1, y,
				m_size_z -
				1));
	}

	// Iterate through cells along the z axis.  4 neighbors total.
	for (int32 z = 1; z < m_size_z - 1; z++) {
		p_out->element(0, 0, z) = p_in->element(0, 0, z) + force *
		(p_in->element(0, 0, z - 1) +
			p_in->element(0, 0, z + 1) +
			p_in->element(1, 0, z) +
			p_in->element(0, 1, z) - 4.0f * p_in->element(0, 0, z));
		p_out->element(m_size_x - 1, 0, z) = p_in->element(m_size_x - 1, 0, z) + force *
		(p_in->element(m_size_x - 1, 0,
		               z - 1) +
			p_in->element(m_size_x - 1, 0,
			              z + 1) +
			p_in->element(m_size_x - 2, 0, z) +
			p_in->element(m_size_x - 1, 1, z) -
			4.0f *
			p_in->element(m_size_x - 1, 0, z));
		p_out->element(0, m_size_y - 1, z) = p_in->element(0, m_size_y - 1, z) + force *
		(p_in->element(0, m_size_y - 1,
		               z - 1) +
			p_in->element(0, m_size_y - 1,
			              z + 1) +
			p_in->element(1, m_size_y - 1, z) +
			p_in->element(0, m_size_y - 2, z) -
			4.0f *
			p_in->element(0, m_size_y - 1, z));
		p_out->element(m_size_x - 1, m_size_y - 1, z) = p_in->element(m_size_x - 1, m_size_y - 1, z) + force *
		(p_in->element(
				m_size_x -
				1,
				m_size_y -
				1,
				z - 1) +
			p_in->element(
				m_size_x -
				1,
				m_size_y -
				1,
				z + 1) +
			p_in->element(
				m_size_x -
				2,
				m_size_y -
				1, z) +
			p_in->element(
				m_size_x -
				1,
				m_size_y -
				2, z) -
			4.0f *
			p_in->element(
				m_size_x -
				1,
				m_size_y -
				1, z));
	}

	// Diffuse the last 8 corner cells.  3 neighbors total.
	p_out->element(0, 0, 0) = p_in->element(0, 0, 0) + force *
	(p_in->element(1, 0, 0) +
		p_in->element(0, 1, 0) +
		p_in->element(0, 0, 1) - 3.0f * p_in->element(0, 0, 0));
	p_out->element(m_size_x - 1, 0, 0) = p_in->element(m_size_x - 1, 0, 0) + force *
	(p_in->element(m_size_x - 2, 0, 0) +
		p_in->element(m_size_x - 1, 1, 0) +
		p_in->element(m_size_x - 1, 0, 1) -
		3.0f * p_in->element(m_size_x - 1, 0, 0));
	p_out->element(0, m_size_y - 1, 0) = p_in->element(0, m_size_y - 1, 0) + force *
	(p_in->element(1, m_size_y - 1, 0) +
		p_in->element(0, m_size_y - 2, 0) +
		p_in->element(0, m_size_y - 1, 1) -
		3.0f * p_in->element(0, m_size_y - 1, 0));
	p_out->element(m_size_x - 1, m_size_y - 1, 0) = p_in->element(m_size_x - 1, m_size_y - 1, 0) + force *
	(p_in->element(
			m_size_x - 2,
			m_size_y - 1,
			0) +
		p_in->element(
			m_size_x -
			1,
			m_size_y -
			2, 0) +
		p_in->element(
			m_size_x -
			1,
			m_size_y -
			1, 1) -
		3.0f *
		p_in->element(
			m_size_x -
			1,
			m_size_y -
			1, 0));
	p_out->element(0, 0, m_size_z - 1) = p_in->element(0, 0, m_size_z - 1) + force *
	(p_in->element(1, 0, m_size_z - 1) +
		p_in->element(0, 1, m_size_z - 1) +
		p_in->element(0, 0, m_size_z - 2) -
		3.0f * p_in->element(0, 0, m_size_z - 1));
	p_out->element(m_size_x - 1, 0, m_size_z - 1) = p_in->element(m_size_x - 1, 0, m_size_z - 1) + force *
	(p_in->element(
			m_size_x - 2,
			0, m_size_z -
			1) +
		p_in->element(
			m_size_x -
			1, 1,
			m_size_z -
			1) +
		p_in->element(
			m_size_x -
			1, 0,
			m_size_z -
			2) - 3.0f *
		p_in->element(
			m_size_x -
			1,
			0,
			m_size_z -
			1));
	p_out->element(0, m_size_y - 1, m_size_z - 1) = p_in->element(0, m_size_y - 1, m_size_z - 1) + force *
	(p_in->element(1,
	               m_size_y -
	               1,
	               m_size_z -
	               1) +
		p_in->element(0,
		              m_size_y -
		              2,
		              m_size_z -
		              1) +
		p_in->element(0,
		              m_size_y -
		              1,
		              m_size_z -
		              2) -
		3.0f *
		p_in->element(0,
		              m_size_y -
		              1,
		              m_size_z -
		              1));
	p_out->element(m_size_x - 1, m_size_y - 1, m_size_z - 1) =
			p_in->element(m_size_x - 1, m_size_y - 1, m_size_z - 1) + force *
			(p_in->element(m_size_x - 2, m_size_y - 1,
			               m_size_z - 1) +
				p_in->element(m_size_x - 1, m_size_y - 2,
				              m_size_z - 1) +
				p_in->element(m_size_x - 1, m_size_y - 1,
				              m_size_z - 2) - 3.0f *
				p_in->element(
					m_size_x -
					1,
					m_size_y -
					1,
					m_size_z -
					1));

	// Iterate through all the remaining cells that are not touching a boundary.  6 neighbors total.
	for (int32 x = 1; x < m_size_x - 1; x++) {
		for (int32 y = 1; y < m_size_y - 1; y++) {
			for (int32 z = 1; z < m_size_z - 1; z++) {
				p_out->element(x, y, z) = p_in->element(x, y, z) + force *
				(p_in->element(x, y, z + 1) +
					p_in->element(x, y, z - 1) +
					p_in->element(x, y + 1, z) +
					p_in->element(x, y - 1, z) +
					p_in->element(x + 1, y, z) +
					p_in->element(x - 1, y, z) -
					6.0f * p_in->element(x, y, z));
			}
		}
	}
}

void FluidSimulation3D::DiffusionStable(TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_in,
                                        TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_out, const float scale) const {
	float force = m_dt * scale;

	if (FMath::IsNegativeFloat(force) || FMath::IsNearlyZero(force))
		return;

	for (int32 x = 1; x < m_size_x - 1; x++) {
		for (int32 y = 1; y < m_size_y - 1; y++) {
			for (int32 z = 1; z < m_size_z - 1; z++) {
				p_out->element(x, y, z) = p_in->element(x, y, z) + force *
				(p_in->element(x, y, z + 1) +
					p_in->element(x, y, z - 1) +
					p_in->element(x, y + 1, z) +
					p_in->element(x, y - 1, z) +
					p_in->element(x + 1, y, z) +
					p_in->element(x - 1, y, z) -
					6.0f * p_in->element(x, y, z));
			}
		}
	}

	//set boundary
	// Iterate through cells along the left and right surfaces
	for (int32 x = 0; x < m_size_x; x++) {
		for (int32 z = 0; z < m_size_z; z++) {
			p_out->element(x, 0, z) = 0.0f;
			p_out->element(x, m_size_y - 1, z) = 0.0f;
		}
	}
	// Iterate through cells along the front and back surfaces
	for (int32 y = 0; y < m_size_y; y++) {
		for (int32 z = 0; z < m_size_z; z++) {
			p_out->element(0, y, z) = 0.0f;
			p_out->element(m_size_x - 1, y, z) = 0.0f;
		}
	}

	// Iterate through cells along the top and bottom surfaces
	for (int32 x = 0; x < m_size_x; x++) {
		for (int32 y = 0; y < m_size_y; y++) {
			p_out->element(x, y, 0) = 0.0f;
			p_out->element(x, y, m_size_z - 1) = 0.0f;
		}
	}
}

// Signed advection is mass conserving, but allows signed quantities 
// so could be used for velocity, since it's faster.
void
FluidSimulation3D::ReverseSignedAdvection(TSharedPtr<VelPkg3D, ESPMode::ThreadSafe> v, const float advectScale) const {
	auto scale = v->Properties()->advection * advectScale;
	// negate advection scale, since it's reverse advection
	auto force = -m_dt * scale;
	float vx, vy, vz; // velocity values of the current x,y,z locations
	int32 x1A, y1A, z1A; // x, y, z locations of top-left-back grid point (A) after advection
	float fx1, fy1, fz1; // fractional remainders of x1, y1, z1
	float A_X, A_Y, A_Z, // top-left-back grid point value after advection
	      B_X, B_Y, B_Z, // top-right-back grid point value after advection
	      C_X, C_Y, C_Z, // bottom-left-back grid point value after advection
	      D_X, D_Y, D_Z, // bottom-right-back grid point value after advection
	      E_X, E_Y, E_Z, // top-left-front grid point value after advection
	      F_X, F_Y, F_Z, // top-right-front grid point value after advection
	      G_X, G_Y, G_Z, // bottom-left-front grid point value after advection
	      H_X, H_Y, H_Z; // bottom-right-front grid point value after advection

	if (FMath::IsNearlyZero(scale)) {
		return;
	}
	// First copy the scalar values over, since we are adding/subtracting in values, not moving things
	Fluid3D velOutX = *v->DestinationX();
	Fluid3D velOutY = *v->DestinationY();
	Fluid3D velOutZ = *v->DestinationZ();

	for (int32 x = 1; x < m_size_x - 1; x++) {
		for (int32 y = 1; y < m_size_y - 1; y++) {
			for (int32 z = 1; z < m_size_z - 1; z++) {
				vx = mp_velocity->SourceX()->element(x, y, z);
				vy = mp_velocity->SourceY()->element(x, y, z);
				vz = mp_velocity->SourceZ()->element(x, y, z);
				if (!FMath::IsNearlyZero(vx) || !FMath::IsNearlyZero(vy) || !FMath::IsNearlyZero(vz)) {
					// Find the floating point location of the advection
					// x, y, z locations after advection
					float x1 = x + vx * force;
					float y1 = y + vy * force;
					float z1 = z + vz * force;

					bool bCollide = Collide(x, y, z, x1, y1, z1);

					// Find the nearest top-left integer grid point of the advection
					x1A = FMath::FloorToInt(x1);
					y1A = FMath::FloorToInt(y1);
					z1A = FMath::FloorToInt(z1);

					// Store the fractional parts
					fx1 = x1 - x1A;
					fy1 = y1 - y1A;
					fz1 = z1 - z1A;

					// Get amounts from (in) source cells for X velocity
					A_X = (1.0f - fx1) * (1.0f - fy1) * (1.0f - fz1) * v->DestinationX()->element(x1A, y1A, z1A);
					B_X = (fx1) * (1.0f - fy1) * (1.0f - fz1) * v->DestinationX()->element(x1A + 1, y1A, z1A);
					C_X = (1.0f - fx1) * (fy1) * (1.0f - fz1) * v->DestinationX()->element(x1A, y1A + 1, z1A);
					D_X = (fx1) * (fy1) * (1.0f - fz1) * v->DestinationX()->element(x1A + 1, y1A + 1, z1A);
					E_X = (1.0f - fx1) * (1.0f - fy1) * (fz1) * v->DestinationX()->element(x1A, y1A, z1A + 1);
					F_X = (fx1) * (1.0f - fy1) * (fz1) * v->DestinationX()->element(x1A + 1, y1A, z1A + 1);
					G_X = (1.0f - fx1) * (fy1) * (fz1) * v->DestinationX()->element(x1A, y1A + 1, z1A + 1);
					H_X = (fx1) * (fy1) * (fz1) * v->DestinationX()->element(x1A + 1, y1A + 1, z1A + 1);

					// Get amounts from (in) source cells for Y velocity
					A_Y = (1.0f - fx1) * (1.0f - fy1) * (1.0f - fz1) * v->DestinationY()->element(x1A, y1A, z1A);
					B_Y = (fx1) * (1.0f - fy1) * (1.0f - fz1) * v->DestinationY()->element(x1A + 1, y1A, z1A);
					C_Y = (1.0f - fx1) * (fy1) * (1.0f - fz1) * v->DestinationY()->element(x1A, y1A + 1, z1A);
					D_Y = (fx1) * (fy1) * (1.0f - fz1) * v->DestinationY()->element(x1A + 1, y1A + 1, z1A);
					E_Y = (1.0f - fx1) * (1.0f - fy1) * (fz1) * v->DestinationY()->element(x1A, y1A, z1A + 1);
					F_Y = (fx1) * (1.0f - fy1) * (fz1) * v->DestinationY()->element(x1A + 1, y1A, z1A + 1);
					G_Y = (1.0f - fx1) * (fy1) * (fz1) * v->DestinationY()->element(x1A, y1A + 1, z1A + 1);
					H_Y = (fx1) * (fy1) * (fz1) * v->DestinationY()->element(x1A + 1, y1A + 1, z1A + 1);

					// Get amounts from (in) source cells for Z velocity
					A_Z = (1.0f - fx1) * (1.0f - fy1) * (1.0f - fz1) * v->DestinationZ()->element(x1A, y1A, z1A);
					B_Z = (fx1) * (1.0f - fy1) * (1.0f - fz1) * v->DestinationZ()->element(x1A + 1, y1A, z1A);
					C_Z = (1.0f - fx1) * (fy1) * (1.0f - fz1) * v->DestinationZ()->element(x1A, y1A + 1, z1A);
					D_Z = (fx1) * (fy1) * (1.0f - fz1) * v->DestinationZ()->element(x1A + 1, y1A + 1, z1A);
					E_Z = (1.0f - fx1) * (1.0f - fy1) * (fz1) * v->DestinationZ()->element(x1A, y1A, z1A + 1);
					F_Z = (fx1) * (1.0f - fy1) * (fz1) * v->DestinationZ()->element(x1A + 1, y1A, z1A + 1);
					G_Z = (1.0f - fx1) * (fy1) * (fz1) * v->DestinationZ()->element(x1A, y1A + 1, z1A + 1);
					H_Z = (fx1) * (fy1) * (fz1) * v->DestinationZ()->element(x1A + 1, y1A + 1, z1A + 1);

					// X Velocity
					// add to (out) source cell
					if (!bCollide) {
						velOutX.element(x, y, z) += A_X + B_X + C_X + D_X + E_X + F_X + G_X + H_X;
					}
					// and subtract from (out) dest cells
					velOutX.element(x1A, y1A, z1A) -= A_X;
					velOutX.element(x1A + 1, y1A, z1A) -= B_X;
					velOutX.element(x1A, y1A + 1, z1A) -= C_X;
					velOutX.element(x1A + 1, y1A + 1, z1A) -= D_X;
					velOutX.element(x1A, y1A, z1A + 1) -= E_X;
					velOutX.element(x1A + 1, y1A, z1A + 1) -= F_X;
					velOutX.element(x1A, y1A + 1, z1A + 1) -= G_X;
					velOutX.element(x1A + 1, y1A + 1, z1A + 1) -= H_X;

					// Y Velocity
					// add to (out) source cell
					if (!bCollide) {
						velOutY.element(x, y, z) += A_Y + B_Y + C_Y + D_Y + E_Y + F_Y + G_Y + H_Y;
					}
					// and subtract from (out) dest cells
					velOutY.element(x1A, y1A, z1A) -= A_Y;
					velOutY.element(x1A + 1, y1A, z1A) -= B_Y;
					velOutY.element(x1A, y1A + 1, z1A) -= C_Y;
					velOutY.element(x1A + 1, y1A + 1, z1A) -= D_Y;
					velOutY.element(x1A, y1A, z1A + 1) -= E_Y;
					velOutY.element(x1A + 1, y1A, z1A + 1) -= F_Y;
					velOutY.element(x1A, y1A + 1, z1A + 1) -= G_Y;
					velOutY.element(x1A + 1, y1A + 1, z1A + 1) -= H_Y;

					// Z Velocity
					// add to (out) source cell
					if (!bCollide) {
						velOutZ.element(x, y, z) += A_Z + B_Z + C_Z + D_Z + E_Z + F_Z + G_Z + H_Z;
					}
					// and subtract from (out) dest cells
					velOutZ.element(x1A, y1A, z1A) -= A_Z;
					velOutZ.element(x1A + 1, y1A, z1A) -= B_Z;
					velOutZ.element(x1A, y1A + 1, z1A) -= C_Z;
					velOutZ.element(x1A + 1, y1A + 1, z1A) -= D_Z;
					velOutZ.element(x1A, y1A, z1A + 1) -= E_Z;
					velOutZ.element(x1A + 1, y1A, z1A + 1) -= F_Z;
					velOutZ.element(x1A, y1A + 1, z1A + 1) -= G_Z;
					velOutZ.element(x1A + 1, y1A + 1, z1A + 1) -= H_Z;
				}
			}
		}
	}

	*v->SourceX() = velOutX;
	*v->SourceY() = velOutY;
	*v->SourceZ() = velOutZ;
}

// Checks if destination point during advection is out of bounds and pulls point in if needed
bool
FluidSimulation3D::Collide(int32 this_x, int32 this_y, int32 this_z, float& new_x, float& new_y, float& new_z) const {
	const float max_advect = 1 - KINDA_SMALL_NUMBER;
	auto bCollide = false;

	// Clamp step to 2 cells (one for wall)
	new_x = this_x + FMath::Clamp<float>(new_x - this_x, -max_advect, max_advect);
	new_y = this_y + FMath::Clamp<float>(new_y - this_y, -max_advect, max_advect);
	new_z = this_z + FMath::Clamp<float>(new_z - this_z, -max_advect, max_advect);

	if (IsSolid(this_x, this_y, this_z)) {
		new_x = this_x;
		new_y = this_y;
		new_z = this_z;
	}
	if (FMath::Abs(new_x) > 1.0f) // should check wall condition
	{
		if (IsSolid(this_x + 1, this_y, this_z) || IsSolid(this_x - 1, this_y, this_z))
			new_x = this_x; //bounce to the wall
	}
	if (FMath::Abs(new_y) > 1.0f) // should check wall condition
	{
		if (IsSolid(this_x, this_y + 1, this_z) || IsSolid(this_x, this_y - 1, this_z))
			new_y = this_x; //bounce to the wall
	}
	if (FMath::Abs(new_z) > 1.0f) // should check wall condition
	{
		if (IsSolid(this_x, this_y, this_z + 1) || IsSolid(this_x, this_y, this_z - 1))
			new_x = this_x; //bounce to the wall
	}

	if (m_bBoundaryCondition) {
		bCollide = false;
	}
	return bCollide;
}

bool FluidSimulation3D::IsSolid(int32 this_x, int32 this_y, int32 this_z) const {
	// bounds allways is solid (it nullifies in diffusion step)
	if (this_x == 0 || this_x == m_size_x - 1)
		return true;
	if (this_y == 0 || this_y == m_size_y - 1)
		return true;
	if (this_z == 0 || this_z == m_size_z - 1)
		return true;
	if (this_x / 2 == 0 || this_y / 2 == 0 || this_z / 2 == 0) {
		//its a wall
	}
	if (this_x / 2 == 1 && this_y / 2 == 1 && this_z / 2 == 1) {
		//its a cell
	}
	return m_solids->element(this_x, this_y, this_z);
}

// Apply acceleration due to pressure
void FluidSimulation3D::PressureAcceleration(const float scale) const {
	float force = m_dt * scale;

	*mp_velocity->DestinationX() = *mp_velocity->SourceX();
	*mp_velocity->DestinationY() = *mp_velocity->SourceY();
	*mp_velocity->DestinationZ() = *mp_velocity->SourceZ();

	for (int32 x = 0; x < m_size_x - 1; x++) {
		for (int32 y = 0; y < m_size_y - 1; y++) {
			for (int32 z = 0; z < m_size_z - 1; z++) {
				// Pressure differential between points to get an accelleration force.
				float src_press = mp_pressure->SourceO2()->element(x, y, z) +
						mp_pressure->SourceN2()->element(x, y, z) +
						mp_pressure->SourceCO2()->element(x, y, z) +
						mp_pressure->SourceToxin()->element(x, y, z);
				//if (!EqualToZero(src_press))
				//    src_press = mp_pressure_O2->Source()->element(x, y, z) / src_press +
				//                mp_pressure_N2->Source()->element(x, y, z) / src_press +
				//                mp_pressure_C02->Source()->element(x, y, z) / src_press +
				//                mp_pressure_Toxin->Source()->element(x, y, z) / src_press;

				float dest_x = mp_pressure->SourceO2()->element(x + 1, y, z) +
						mp_pressure->SourceN2()->element(x + 1, y, z) +
						mp_pressure->SourceCO2()->element(x + 1, y, z) +
						mp_pressure->SourceToxin()->element(x + 1, y, z);
				//if (!EqualToZero(dest_x))
				//    dest_x = mp_pressure_O2->Source()->element(x + 1, y, z) / dest_x +
				//            mp_pressure_N2->Source()->element(x + 1, y, z) / dest_x +
				//            mp_pressure_C02->Source()->element(x + 1, y, z) / dest_x +
				//            mp_pressure_Toxin->Source()->element(x + 1, y, z) / dest_x;

				float dest_y = mp_pressure->SourceO2()->element(x, y + 1, z) +
						mp_pressure->SourceN2()->element(x, y + 1, z) +
						mp_pressure->SourceCO2()->element(x, y + 1, z) +
						mp_pressure->SourceToxin()->element(x, y + 1, z);
				//if (!EqualToZero(dest_y))
				//    dest_y = mp_pressure_O2->Source()->element(x, y + 1, z) / dest_y +
				//             mp_pressure_N2->Source()->element(x, y + 1, z) / dest_y +
				//             mp_pressure_C02->Source()->element(x, y + 1, z) / dest_y +
				//             mp_pressure_Toxin->Source()->element(x, y + 1, z) / dest_y;

				float dest_z = mp_pressure->SourceO2()->element(x, y, z + 1) +
						mp_pressure->SourceN2()->element(x, y, z + 1) +
						mp_pressure->SourceCO2()->element(x, y, z + 1) +
						mp_pressure->SourceToxin()->element(x, y, z + 1);
				//if (!EqualToZero(dest_z))
				//    dest_z = mp_pressure_O2->Source()->element(x, y, z + 1) / dest_z +
				//             mp_pressure_N2->Source()->element(x, y, z + 1) / dest_z +
				//             mp_pressure_C02->Source()->element(x, y, z + 1) / dest_z +
				//             mp_pressure_Toxin->Source()->element(x, y, z + 1) / dest_z;

				float force_x = src_press - dest_x;
				float force_y = src_press - dest_y;
				float force_z = src_press - dest_z;

				// Use the acceleration force to move the velocity field in the appropriate direction.
				// Ex. If an area of high pressure exists the acceleration force will turn the velocity field
				// away from this area
				mp_velocity->DestinationX()->element(x, y, z) += force * force_x;
				mp_velocity->DestinationX()->element(x + 1, y, z) += force * force_x;

				mp_velocity->DestinationY()->element(x, y, z) += force * force_y;
				mp_velocity->DestinationY()->element(x, y + 1, z) += force * force_y;

				mp_velocity->DestinationZ()->element(x, y, z) += force * force_z;
				mp_velocity->DestinationZ()->element(x, y, z + 1) += force * force_z;
			}
		}
	}

	mp_velocity->SwapLocationsX();
	mp_velocity->SwapLocationsY();
	mp_velocity->SwapLocationsZ();
}

// Apply a natural deceleration to forces applied to the grids
void
FluidSimulation3D::ExponentialDecay(TSharedPtr<Fluid3D, ESPMode::ThreadSafe> p_in_and_out, const float decay) const {
	for (int32 x = 0; x < m_size_x; x++) {
		for (int32 y = 0; y < m_size_y; y++) {
			for (int32 z = 0; z < m_size_z; z++) {
				p_in_and_out->element(x, y, z) = p_in_and_out->element(x, y, z) * FMath::Pow(1 - decay, m_dt);
			}
		}
	}
}

// Apply vorticities to the simulation
void FluidSimulation3D::VorticityConfinement(const float scale) const {
	float lr_curl; // curl in the left-right direction
	float ud_curl; // curl in the up-down direction
	float bf_curl; // curl in the back-front direction
	float length;
	float magnitude;

	mp_velocity->DestinationX()->Set(0.0f);
	mp_velocity->DestinationY()->Set(0.0f);
	mp_velocity->DestinationZ()->Set(0.0f);

	for (int32 i = 1; i < m_size_x - 1; i++) {
		for (int32 j = 1; j < m_size_y - 1; j++) {
			for (int32 k = 1; k < m_size_z - 1; k++) {
				m_curl->element(i, j, k) = FMath::Abs(Curl(i, j, k));
			}
		}
	}

	for (int32 x = 2; x < m_size_x - 1; x++) {
		for (int32 y = 2; y < m_size_y - 1; y++) {
			for (int32 z = 2; z < m_size_z - 1; z++) {
				// Get curl gradient across cells
				lr_curl = (m_curl->element(x + 1, y, z) - m_curl->element(x - 1, y, z)) * 0.5f;
				ud_curl = (m_curl->element(x, y + 1, z) - m_curl->element(x, y - 1, z)) * 0.5f;
				bf_curl = (m_curl->element(x, y, z + 1) - m_curl->element(x, y, z - 1)) * 0.5f;

				// Normalize the derivitive curl vector
				length = FMath::Sqrt(lr_curl * lr_curl + ud_curl * ud_curl + bf_curl * bf_curl) + 0.000001f;
				lr_curl /= length;
				ud_curl /= length;
				bf_curl /= length;

				magnitude = Curl(x, y, z);

				mp_velocity->DestinationX()->element(x, y, z) = -ud_curl * magnitude;
				mp_velocity->DestinationY()->element(x, y, z) = lr_curl * magnitude;
				mp_velocity->DestinationZ()->element(x, y, z) = bf_curl * magnitude;
			}
		}
	}

	*mp_velocity->SourceX() += *mp_velocity->DestinationX() * scale;
	*mp_velocity->SourceY() += *mp_velocity->DestinationY() * scale;
	*mp_velocity->SourceZ() += *mp_velocity->DestinationZ() * scale;
}

// Calculate the curl at position (x,y,z) in the fluid grid. Physically this represents the vortex strength at the
// cell. Computed as follows: w = (del x U) where U is the velocity vector at (i, j).
float FluidSimulation3D::Curl(const int32 x, const int32 y, const int32 z) const {
	// difference in XV of cells above and below
	// positive number is a counter-clockwise rotation
	float x_curl = (mp_velocity->SourceX()->element(x, y + 1, z) - mp_velocity->SourceX()->element(x, y - 1, z)) * 0.5f;

	// difference in YV of cells left and right
	float y_curl = (mp_velocity->SourceY()->element(x + 1, y, z) - mp_velocity->SourceY()->element(x - 1, y, z)) * 0.5f;

	// difference in ZV of cells front and back
	float z_curl = (mp_velocity->SourceZ()->element(x, y, z + 1) - mp_velocity->SourceY()->element(x, y, z - 1)) * 0.5f;

	return x_curl - y_curl - z_curl;
}

// Invert velocities that are facing outwards at boundaries
void FluidSimulation3D::InvertVelocityEdges() const {
	for (int32 y = 0; y < m_size_y; y++) {
		for (int32 z = 0; z < m_size_z; z++) {
			if (mp_velocity->SourceX()->element(0, y, z) < 0.0f) {
				mp_velocity->SourceX()->element(0, y, z) = -mp_velocity->SourceX()->element(0, y, z);
			}
			if (mp_velocity->SourceX()->element(m_size_x - 1, y, z) > 0.0f) {
				mp_velocity->SourceX()->element(m_size_x - 1, y, z) = -mp_velocity->SourceX()->element(m_size_x - 1, y,
				                                                                                       z);
			}
		}
	}

	for (int32 x = 0; x < m_size_x; x++) {
		for (int32 z = 0; z < m_size_z; z++) {
			if (mp_velocity->SourceY()->element(x, 0, z) < 0.0f) {
				mp_velocity->SourceY()->element(x, 0, z) = -mp_velocity->SourceY()->element(x, 0, z);
			}
			if (mp_velocity->SourceY()->element(x, m_size_y - 1, z) > 0.0f) {
				mp_velocity->SourceY()->element(x, m_size_y - 1, z) = -mp_velocity->SourceY()->element(x, m_size_y - 1,
				                                                                                       z);
			}
		}
	}

	for (int32 x = 0; x < m_size_x; x++) {
		for (int32 y = 0; y < m_size_y; y++) {
			if (mp_velocity->SourceZ()->element(x, y, 0) < 0.0f) {
				mp_velocity->SourceZ()->element(x, y, 0) = -mp_velocity->SourceZ()->element(x, y, 0);
			}
			if (mp_velocity->SourceZ()->element(x, y, m_size_z - 1) > 0.0f) {
				mp_velocity->SourceZ()->element(x, y, m_size_z - 1) = -mp_velocity->SourceZ()->element(x, y,
				                                                                                       m_size_z - 1);
			}
		}
	}
}

// Reset the simulation's grids to defaults, does not affect individual parameters
void FluidSimulation3D::Reset() const {
	mp_pressure->Reset(1.0f);
	mp_velocity->Reset(0.0f);
	mp_ink->Reset(0.0f);
	mp_heat->Reset(0.0f);
}

// Accessor methods
int32 FluidSimulation3D::DiffusionIterations() const {
	return m_diffusionIter;
}

void FluidSimulation3D::DiffusionIterations(int32 value) {
	m_diffusionIter = value;
}

float FluidSimulation3D::Vorticity() const {
	return m_vorticity;
}

void FluidSimulation3D::Vorticity(float value) {
	m_vorticity = value;
}

float FluidSimulation3D::PressureAccel() const {
	return m_pressureAccel;
}

void FluidSimulation3D::PressureAccel(float value) {
	m_pressureAccel = value;
}

float FluidSimulation3D::dt() const {
	return m_dt;
}

void FluidSimulation3D::dt(float value) {
	m_dt = value;
}

TSharedPtr<AtmoPkg3D, ESPMode::ThreadSafe> FluidSimulation3D::Pressure() const {
	return mp_pressure;
}

TSharedPtr<VelPkg3D, ESPMode::ThreadSafe> FluidSimulation3D::Velocity() const {
	return mp_velocity;
};

TSharedPtr<FluidPkg3D, ESPMode::ThreadSafe> FluidSimulation3D::Ink() const {
	return mp_ink;
};

TSharedPtr<FluidPkg3D, ESPMode::ThreadSafe> FluidSimulation3D::Heat() const {
	return mp_heat;
};

int32 FluidSimulation3D::Height() const {
	return m_size_z;
}

int32 FluidSimulation3D::Width() const {
	return m_size_x;
}

int32 FluidSimulation3D::Depth() const {
	return m_size_y;
}
