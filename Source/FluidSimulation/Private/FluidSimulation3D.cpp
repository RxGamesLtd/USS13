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

#include "FluidSimulation3D.h"

DEFINE_STAT(STAT_AtmosUpdatesCount)

FluidSimulation3D::FluidSimulation3D(int32 xSize, int32 ySize, int32 zSize, float dt)
    : m_diffusionIter(1)
    , m_vorticity(0.0)
    , m_pressureAccel(0.0)
    , m_dt(dt)
    , m_size_x(xSize)
    , m_size_y(ySize)
    , m_size_z(zSize)
{
    m_curl = MakeUnique<Fluid3D>(xSize, ySize, zSize);
    mp_velocity = MakeUnique<VelPkg3D>(xSize, ySize, zSize);
    mp_pressure = MakeUnique<AtmoPkg3D>(xSize, ySize, zSize);
    m_solids = MakeUnique<TArray3D<EFlowDirection>>(xSize - 1, ySize - 1, zSize - 1);

    Reset();
}

// Update is called every frame or as specified in the max desired updates per second GUI slider
void FluidSimulation3D::Update() const
{
    SCOPE_CYCLE_COUNTER(STAT_AtmosUpdatesCount);
    UpdateDiffusion();
    UpdateForces();
    UpdateAdvection();
}

// Apply diffusion across the grids
void FluidSimulation3D::UpdateDiffusion() const
{
    // Skip diffusion if disabled
    // Diffusion of Velocity
    if (!FMath::IsNearlyZero(mp_velocity->Properties().diffusion)) {
        const auto scaledVelocity = mp_velocity->Properties().diffusion / static_cast<float>(m_diffusionIter);

        for (auto i = 0; i < m_diffusionIter; ++i) {
            DiffusionStable(mp_velocity->SourceX(), mp_velocity->DestinationX(), scaledVelocity);
            mp_velocity->SwapLocationsX();
            DiffusionStable(mp_velocity->SourceY(), mp_velocity->DestinationY(), scaledVelocity);
            mp_velocity->SwapLocationsY();
            DiffusionStable(mp_velocity->SourceZ(), mp_velocity->DestinationZ(), scaledVelocity);
            mp_velocity->SwapLocationsZ();
        }
    }

    // Diffusion of Pressure
    if (!FMath::IsNearlyZero(mp_pressure->Properties().diffusion)) {
        const auto scaledDiffusion = mp_pressure->Properties().diffusion / static_cast<float>(m_diffusionIter);
        for (auto i = 0; i < m_diffusionIter; ++i) {
            DiffusionStable(mp_pressure->SourceO2(), mp_pressure->DestinationO2(), scaledDiffusion);
            DiffusionStable(mp_pressure->SourceCO2(), mp_pressure->DestinationCO2(), scaledDiffusion);
            DiffusionStable(mp_pressure->SourceN2(), mp_pressure->DestinationN2(), scaledDiffusion);
            DiffusionStable(mp_pressure->SourceToxin(), mp_pressure->DestinationToxin(), scaledDiffusion);
            mp_pressure->SwapLocations();
        }
    }
}

// Apply forces across the grids
void FluidSimulation3D::UpdateForces() const
{
    // Apply dampening force on velocity due to viscosity
    if (!FMath::IsNearlyZero(mp_velocity->Properties().decay)) {
        ExponentialDecay(mp_velocity->SourceX(), mp_velocity->Properties().decay);
        ExponentialDecay(mp_velocity->SourceY(), mp_velocity->Properties().decay);
        ExponentialDecay(mp_velocity->SourceZ(), mp_velocity->Properties().decay);
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
void FluidSimulation3D::UpdateAdvection() const
{
    auto avg_dimension = (m_size_x + m_size_y + m_size_z) / 3.0f;
    const auto std_dimension = 100.0f;

    // Change advection scale depending on grid size. Smaller grids means larger cells, so scale should be smaller.
    // Average dimension size of std_dimension value (100) equals an advection_scale of 1
    auto advection_scale = avg_dimension / std_dimension;

    // Advection order makes significant differences
    // Advecting pressure first leads to self-maintaining waves and ripple artifacts
    // Advecting velocity first naturally dissipates the waves

    // Advect Velocity
    ForwardAdvection(mp_velocity->SourceX(), mp_velocity->DestinationX(),
        mp_velocity->Properties().advection * advection_scale);
    ForwardAdvection(mp_velocity->SourceY(), mp_velocity->DestinationY(),
        mp_velocity->Properties().advection * advection_scale);
    ForwardAdvection(mp_velocity->SourceZ(), mp_velocity->DestinationZ(),
        mp_velocity->Properties().advection * advection_scale);

    ReverseSignedAdvection(*mp_velocity, mp_velocity->Properties().advection * advection_scale);

    SetBoundary(mp_velocity->SourceX());
    SetBoundary(mp_velocity->SourceY());
    SetBoundary(mp_velocity->SourceZ());

    // Advect Pressure. Represents compressible fluid
    ForwardAdvection(mp_pressure->SourceO2(), mp_pressure->DestinationO2(),
        mp_pressure->Properties().advection * advection_scale);
    ForwardAdvection(mp_pressure->SourceN2(), mp_pressure->DestinationN2(),
        mp_pressure->Properties().advection * advection_scale);
    ForwardAdvection(mp_pressure->SourceCO2(), mp_pressure->DestinationCO2(),
        mp_pressure->Properties().advection * advection_scale);
    ForwardAdvection(mp_pressure->SourceToxin(), mp_pressure->DestinationToxin(),
        mp_pressure->Properties().advection * advection_scale);
    mp_pressure->SwapLocations();
    ReverseAdvection(mp_pressure->SourceO2(), mp_pressure->DestinationO2(),
        mp_pressure->Properties().advection * advection_scale);
    ReverseAdvection(mp_pressure->SourceN2(), mp_pressure->DestinationN2(),
        mp_pressure->Properties().advection * advection_scale);
    ReverseAdvection(mp_pressure->SourceCO2(), mp_pressure->DestinationCO2(),
        mp_pressure->Properties().advection * advection_scale);
    ReverseAdvection(mp_pressure->SourceToxin(), mp_pressure->DestinationToxin(),
        mp_pressure->Properties().advection * advection_scale);
    mp_pressure->SwapLocations();
}

void FluidSimulation3D::ForwardAdvection(const Fluid3D& p_in, Fluid3D& p_out, float scale) const
{
    auto force = m_dt * scale; // distance to advect

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
    p_out = p_in;

    if (FMath::IsNearlyZero(force)) {
        return;
    }

    // This can easily be threaded as the input array is independent from the output array
    for (auto x = 1; x < m_size_x - 1; ++x) {
        for (auto y = 1; y < m_size_y - 1; ++y) {
            for (auto z = 1; z < m_size_z - 1; ++z) {
                auto vx = mp_velocity->SourceX().element(x, y, z);
                auto vy = mp_velocity->SourceY().element(x, y, z);
                auto vz = mp_velocity->SourceZ().element(x, y, z);
                if (!FMath::IsNearlyZero(vx) || !FMath::IsNearlyZero(vy) || !FMath::IsNearlyZero(vz)) {
                    // Find the floating point location of the forward advection
                    auto x1 = x + vx * force;
                    auto y1 = y + vy * force;
                    auto z1 = z + vz * force;

                    // Check for and correct boundary collisions
                    Collide(x, y, z, x1, y1, z1);

                    // Find the nearest top-left integer grid point of the advection
                    auto x1A = FMath::FloorToInt(x1);
                    auto y1A = FMath::FloorToInt(y1);
                    auto z1A = FMath::FloorToInt(z1);

                    // Store the fractional parts
                    auto fx1 = x1 - x1A;
                    auto fy1 = y1 - y1A;
                    auto fz1 = z1 - z1A;

                    // The floating point location after forward advection (x1,y1,z1) will land within an 8 point cube (A,B,C,D,E,F,G,H).
                    // Distribute the value of the source point among the destination grid points using bilinear interoplation.
                    // Subtract the total value given to the destination grid points from the source point.

                    // Pull source value from the unmodified p_in
                    auto source_value = p_in.element(x, y, z);

                    // Bilinear interpolation
                    auto A = (1.0f - fz1) * (1.0f - fy1) * (1.0f - fx1) * source_value;
                    auto B = (1.0f - fz1) * (1.0f - fy1) * fx1 * source_value;
                    auto C = (1.0f - fz1) * fy1 * (1.0f - fx1) * source_value;
                    auto D = (1.0f - fz1) * fy1 * fx1 * source_value;
                    auto E = fz1 * (1.0f - fy1) * (1.0f - fx1) * source_value;
                    auto F = fz1 * (1.0f - fy1) * fx1 * source_value;
                    auto G = fz1 * fy1 * (1.0f - fx1) * source_value;
                    auto H = fz1 * fy1 * fx1 * source_value;

                    // Add A,B,C,D,E,F,G,H to the eight destination cells
                    p_out.element(x1A, y1A, z1A) += A;
                    p_out.element(x1A + 1, y1A, z1A) += B;
                    p_out.element(x1A, y1A + 1, z1A) += C;
                    p_out.element(x1A + 1, y1A + 1, z1A) += D;
                    p_out.element(x1A, y1A, z1A + 1) += E;
                    p_out.element(x1A + 1, y1A, z1A + 1) += F;
                    p_out.element(x1A, y1A + 1, z1A + 1) += G;
                    p_out.element(x1A + 1, y1A + 1, z1A + 1) += H;

                    // Subtract A-H from source for mass conservation
                    p_out.element(x, y, z) -= A + B + C + D + E + F + G + H;
                }
            }
        }
    }
}

void FluidSimulation3D::ReverseAdvection(const Fluid3D& p_in, Fluid3D& p_out, float scale) const
{
    auto force = m_dt * scale; // distance to advect
    int32 x1A, y1A, z1A; // x, y, z locations of top-left-back grid point (A) after advection
    float A, // top-left-back grid point value after advection
        B, // top-right-back grid point value after advection
        C, // bottom-left-back grid point value after advection
        D, // bottom-right-back grid point value after advection
        E, // top-left-front grid point value after advection
        F, // top-right-front grid point value after advection
        G, // bottom-left-front grid point value after advection
        H; // bottom-right-front grid point value after advection

    // Copy source to destination as reverse advection results in adding/subtracing not moving
    p_out = p_in;

    if (FMath::IsNearlyZero(force)) {
        return;
    }

    // we need to zero out the fractions
    // The new X coordinate after advection stored in x,y,z where x,y,z,z is the original source point
    TArray3D<int32> FromSource_xA(m_size_x, m_size_y, m_size_z, -1.f);
    // The new Y coordinate after advection stored in x,y,z where x,y,z is the original source point
    TArray3D<int32> FromSource_yA(m_size_x, m_size_y, m_size_z, -1.f);
    // The new Z coordinate after advection stored in x,y,z where x,y,z is the original source point
    TArray3D<int32> FromSource_zA(m_size_x, m_size_y, m_size_z, -1.f);
    // The value of A after advection stored in x,y,z where x,y,z is the original source point
    TArray3D<float> FromSource_A(m_size_x, m_size_y, m_size_z);
    // The value of B after advection stored in x,y,z where x,y,z is the original source point
    TArray3D<float> FromSource_B(m_size_x, m_size_y, m_size_z);
    // The value of C after advection stored in x,y,z where x,y,z is the original source point
    TArray3D<float> FromSource_C(m_size_x, m_size_y, m_size_z);
    // The value of D after advection stored in x,y,z where x,y,z is the original source point
    TArray3D<float> FromSource_D(m_size_x, m_size_y, m_size_z);
    // The value of E after advection stored in x,y,z where x,y,z is the original source point
    TArray3D<float> FromSource_E(m_size_x, m_size_y, m_size_z);
    // The value of F after advection stored in x,y,z where x,y,z is the original source point
    TArray3D<float> FromSource_F(m_size_x, m_size_y, m_size_z);
    // The value of G after advection stored in x,y,z where x,y,z is the original source point
    TArray3D<float> FromSource_G(m_size_x, m_size_y, m_size_z);
    // The value of H after advection stored in x,y,z where x,y,z is the original source point
    TArray3D<float> FromSource_H(m_size_x, m_size_y, m_size_z);
    // The total accumulated value after advection stored in x,y,z where x,y,z is the destination point
    TArray3D<float> TotalDestValue(m_size_x, m_size_y, m_size_z);

    // This can easily be threaded as the input array is independent from the output array
    for (auto x = 1; x < m_size_x - 1; ++x) {
        for (auto y = 1; y < m_size_y - 1; ++y) {
            for (auto z = 1; z < m_size_z - 1; ++z) {
                auto vx = mp_velocity->SourceX().element(x, y, z);
                auto vy = mp_velocity->SourceY().element(x, y, z);
                auto vz = mp_velocity->SourceZ().element(x, y, z);
                if (!FMath::IsNearlyZero(vx) || !FMath::IsNearlyZero(vy) || !FMath::IsNearlyZero(vz)) {
                    // Find the floating point location of the advection
                    auto x1 = x + vx * force;
                    auto y1 = y + vy * force;
                    auto z1 = z + vz * force;

                    // Check for and correct boundary collisions
                    Collide(x, y, z, x1, y1, z1);

                    // Find the nearest top-left integer grid point of the advection
                    x1A = FMath::FloorToInt(x1);
                    y1A = FMath::FloorToInt(y1);
                    z1A = FMath::FloorToInt(z1);

                    // Store the fractional parts
                    auto fx1 = x1 - x1A;
                    auto fy1 = y1 - y1A;
                    auto fz1 = z1 - z1A;

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
                    B = (1.0f - fz1) * (1.0f - fy1) * fx1;
                    C = (1.0f - fz1) * fy1 * (1.0f - fx1);
                    D = (1.0f - fz1) * fy1 * fx1;
                    E = fz1 * (1.0f - fy1) * (1.0f - fx1);
                    F = fz1 * (1.0f - fy1) * fx1;
                    G = fz1 * fy1 * (1.0f - fx1);
                    H = fz1 * fy1 * fx1;

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

    for (auto y = 1; y < m_size_y - 1; ++y) {
        for (auto x = 1; x < m_size_x - 1; ++x) {
            for (auto z = 1; z < m_size_z - 1; ++z) {
                if (FromSource_xA.element(x, y, z) != -1.f) {
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
                    auto A_Total = TotalDestValue.element(x1A, y1A, z1A);
                    auto B_Total = TotalDestValue.element(x1A + 1, y1A, z1A);
                    auto C_Total = TotalDestValue.element(x1A, y1A + 1, z1A);
                    auto D_Total = TotalDestValue.element(x1A + 1, y1A + 1, z1A);
                    auto E_Total = TotalDestValue.element(x1A, y1A, z1A + 1);
                    auto F_Total = TotalDestValue.element(x1A + 1, y1A, z1A + 1);
                    auto G_Total = TotalDestValue.element(x1A, y1A + 1, z1A + 1);
                    auto H_Total = TotalDestValue.element(x1A + 1, y1A + 1, z1A + 1);

                    // If less then 1.0 in total then no scaling is neccessary
                    if (A_Total < 1.0f)
                        A_Total = 1.0f;
                    if (B_Total < 1.0f)
                        B_Total = 1.0f;
                    if (C_Total < 1.0f)
                        C_Total = 1.0f;
                    if (D_Total < 1.0f)
                        D_Total = 1.0f;
                    if (E_Total < 1.0f)
                        E_Total = 1.0f;
                    if (F_Total < 1.0f)
                        F_Total = 1.0f;
                    if (G_Total < 1.0f)
                        G_Total = 1.0f;
                    if (H_Total < 1.0f)
                        H_Total = 1.0f;

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
                    p_out.element(x, y, z) += A * p_in.element(x1A, y1A, z1A) + B * p_in.element(x1A + 1, y1A, z1A) + C * p_in.element(x1A, y1A + 1, z1A) + D * p_in.element(x1A + 1, y1A + 1, z1A) + E * p_in.element(x1A, y1A, z1A + 1) + F * p_in.element(x1A + 1, y1A, z1A + 1) + G * p_in.element(x1A, y1A + 1, z1A + 1) + H * p_in.element(x1A + 1, y1A + 1, z1A + 1);

                    // Subtract the values added to the destination from the source for mass conservation
                    p_out.element(x1A, y1A, z1A) -= A * p_in.element(x1A, y1A, z1A);
                    p_out.element(x1A + 1, y1A, z1A) -= B * p_in.element(x1A + 1, y1A, z1A);
                    p_out.element(x1A, y1A + 1, z1A) -= C * p_in.element(x1A, y1A + 1, z1A);
                    p_out.element(x1A + 1, y1A + 1, z1A) -= D * p_in.element(x1A + 1, y1A + 1, z1A);
                    p_out.element(x1A, y1A, z1A + 1) -= E * p_in.element(x1A, y1A, z1A + 1);
                    p_out.element(x1A + 1, y1A, z1A + 1) -= F * p_in.element(x1A + 1, y1A, z1A + 1);
                    p_out.element(x1A, y1A + 1, z1A + 1) -= G * p_in.element(x1A, y1A + 1, z1A + 1);
                    p_out.element(x1A + 1, y1A + 1, z1A + 1) -= H * p_in.element(x1A + 1, y1A + 1, z1A + 1);
                }
            }
        }
    }
}

inline float FluidSimulation3D::TransferPresure(const Fluid3D& p_in, int32 x, int32 y, int32 z, float force) const
{
    // Take care of boundries
    if (IsBlocked(x, y, z, EFlowDirection::Self)) {
        return 0.f;
    }
    auto d = 0.f;
    auto c = 0.f;
    if (!IsBlocked(x, y, z, EFlowDirection::X_Plus)) {
        c += p_in.element(x + 1, y, z);
        ++d;
    }
    if (!IsBlocked(x, y, z, EFlowDirection::X_Minus)) {
        c += p_in.element(x - 1, y, z);
        ++d;
    }
    if (!IsBlocked(x, y, z, EFlowDirection::Y_Plus)) {
        c += p_in.element(x, y + 1, z);
        ++d;
    }
    if (!IsBlocked(x, y, z, EFlowDirection::Y_Minus)) {
        c += p_in.element(x, y - 1, z);
        ++d;
    }
    if (!IsBlocked(x, y, z, EFlowDirection::Z_Plus)) {
        c += p_in.element(x, y, z + 1);
        ++d;
    }
    if (!IsBlocked(x, y, z, EFlowDirection::Z_Minus)) {
        c += p_in.element(x, y, z - 1);
        ++d;
    }
    return p_in.element(x, y, z) + force * (c - d * p_in.element(x, y, z));
}

void FluidSimulation3D::DiffusionStable(const Fluid3D& p_in, Fluid3D& p_out, float scale) const
{
    auto force = m_dt * scale;

    if (FMath::IsNegativeFloat(force) || FMath::IsNearlyZero(force))
        return;

    for (auto x = 0; x < m_size_x; ++x) {
        for (auto y = 0; y < m_size_y; ++y) {
            for (auto z = 0; z < m_size_z; ++z) {
                p_out.element(x, y, z) = TransferPresure(p_in, x, y, z, force);
            }
        }
    }
}

// Signed advection is mass conserving, but allows signed quantities
// so could be used for velocity, since it's faster.
void FluidSimulation3D::ReverseSignedAdvection(VelPkg3D& v, const float scale) const
{
    // negate advection scale, since it's reverse advection
    auto force = -m_dt * scale;

    if (FMath::IsNearlyZero(scale)) {
        return;
    }
    // First copy the scalar values over, since we are adding/subtracting in values, not moving things
    auto velOutX = v.DestinationX();
    auto velOutY = v.DestinationY();
    auto velOutZ = v.DestinationZ();

    for (auto x = 1; x < m_size_x - 1; ++x) {
        for (auto y = 1; y < m_size_y - 1; ++y) {
            for (auto z = 1; z < m_size_z - 1; ++z) {
                auto vx = mp_velocity->SourceX().element(x, y, z);
                auto vy = mp_velocity->SourceY().element(x, y, z);
                auto vz = mp_velocity->SourceZ().element(x, y, z);
                if (!FMath::IsNearlyZero(vx) || !FMath::IsNearlyZero(vy) || !FMath::IsNearlyZero(vz)) {
                    // Find the floating point location of the advection
                    // x, y, z locations after advection
                    auto x1 = x + vx * force;
                    auto y1 = y + vy * force;
                    auto z1 = z + vz * force;

                    auto bCollide = Collide(x, y, z, x1, y1, z1);

                    // Find the nearest top-left integer grid point of the advection
                    auto x1A = FMath::FloorToInt(x1);
                    auto y1A = FMath::FloorToInt(y1);
                    auto z1A = FMath::FloorToInt(z1);

                    // Store the fractional parts
                    auto fx1 = x1 - x1A;
                    auto fy1 = y1 - y1A;
                    auto fz1 = z1 - z1A;

                    // Get amounts from (in) source cells for X velocity
                    auto A_X = (1.0f - fx1) * (1.0f - fy1) * (1.0f - fz1) * v.DestinationX().element(x1A, y1A, z1A);
                    auto B_X = fx1 * (1.0f - fy1) * (1.0f - fz1) * v.DestinationX().element(x1A + 1, y1A, z1A);
                    auto C_X = (1.0f - fx1) * fy1 * (1.0f - fz1) * v.DestinationX().element(x1A, y1A + 1, z1A);
                    auto D_X = fx1 * fy1 * (1.0f - fz1) * v.DestinationX().element(x1A + 1, y1A + 1, z1A);
                    auto E_X = (1.0f - fx1) * (1.0f - fy1) * fz1 * v.DestinationX().element(x1A, y1A, z1A + 1);
                    auto F_X = fx1 * (1.0f - fy1) * fz1 * v.DestinationX().element(x1A + 1, y1A, z1A + 1);
                    auto G_X = (1.0f - fx1) * fy1 * fz1 * v.DestinationX().element(x1A, y1A + 1, z1A + 1);
                    auto H_X = fx1 * fy1 * fz1 * v.DestinationX().element(x1A + 1, y1A + 1, z1A + 1);

                    // Get amounts from (in) source cells for Y velocity
                    auto A_Y = (1.0f - fx1) * (1.0f - fy1) * (1.0f - fz1) * v.DestinationY().element(x1A, y1A, z1A);
                    auto B_Y = fx1 * (1.0f - fy1) * (1.0f - fz1) * v.DestinationY().element(x1A + 1, y1A, z1A);
                    auto C_Y = (1.0f - fx1) * fy1 * (1.0f - fz1) * v.DestinationY().element(x1A, y1A + 1, z1A);
                    auto D_Y = fx1 * fy1 * (1.0f - fz1) * v.DestinationY().element(x1A + 1, y1A + 1, z1A);
                    auto E_Y = (1.0f - fx1) * (1.0f - fy1) * fz1 * v.DestinationY().element(x1A, y1A, z1A + 1);
                    auto F_Y = fx1 * (1.0f - fy1) * fz1 * v.DestinationY().element(x1A + 1, y1A, z1A + 1);
                    auto G_Y = (1.0f - fx1) * fy1 * fz1 * v.DestinationY().element(x1A, y1A + 1, z1A + 1);
                    auto H_Y = fx1 * fy1 * fz1 * v.DestinationY().element(x1A + 1, y1A + 1, z1A + 1);

                    // Get amounts from (in) source cells for Z velocity
                    auto A_Z = (1.0f - fx1) * (1.0f - fy1) * (1.0f - fz1) * v.DestinationZ().element(x1A, y1A, z1A);
                    auto B_Z = fx1 * (1.0f - fy1) * (1.0f - fz1) * v.DestinationZ().element(x1A + 1, y1A, z1A);
                    auto C_Z = (1.0f - fx1) * fy1 * (1.0f - fz1) * v.DestinationZ().element(x1A, y1A + 1, z1A);
                    auto D_Z = fx1 * fy1 * (1.0f - fz1) * v.DestinationZ().element(x1A + 1, y1A + 1, z1A);
                    auto E_Z = (1.0f - fx1) * (1.0f - fy1) * fz1 * v.DestinationZ().element(x1A, y1A, z1A + 1);
                    auto F_Z = fx1 * (1.0f - fy1) * fz1 * v.DestinationZ().element(x1A + 1, y1A, z1A + 1);
                    auto G_Z = (1.0f - fx1) * fy1 * fz1 * v.DestinationZ().element(x1A, y1A + 1, z1A + 1);
                    auto H_Z = fx1 * fy1 * fz1 * v.DestinationZ().element(x1A + 1, y1A + 1, z1A + 1);

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

    v.SourceX() = velOutX;
    v.SourceY() = velOutY;
    v.SourceZ() = velOutZ;
}

// Checks if destination point during advection is out of bounds and pulls point in if needed
bool FluidSimulation3D::Collide(int32 this_x, int32 this_y, int32 this_z, float& new_x, float& new_y, float& new_z) const
{
    const auto max_advect = 2.f - KINDA_SMALL_NUMBER; // 1.5 - is center of neighbor cell
    auto bCollide = false;

    auto delta_x = FMath::Clamp<float>(new_x - this_x, -max_advect, max_advect);
    auto delta_y = FMath::Clamp<float>(new_y - this_y, -max_advect, max_advect);
    auto delta_z = FMath::Clamp<float>(new_z - this_z, -max_advect, max_advect);

    new_x = this_x + delta_x;
    new_y = this_y + delta_y;
    new_z = this_z + delta_z;

    if (IsBlocked(this_x, this_y, this_z, EFlowDirection::Self)) {
        new_x = this_x;
        new_y = this_y;
        new_z = this_z;
        bCollide = true;
    }
    if (FMath::Abs(delta_x) > 1.0f) {
        if (delta_x > 0 && IsBlocked(this_x, this_y, this_z, EFlowDirection::X_Plus)) {
            new_x = this_x;
            bCollide = true;
        }
        if (delta_x < 0 && IsBlocked(this_x, this_y, this_z, EFlowDirection::X_Minus)) {
            new_x = this_x;
            bCollide = true;
        }
    }
    if (FMath::Abs(delta_y) > 1.0f) {
        if (delta_y > 0 && IsBlocked(this_x, this_y, this_z, EFlowDirection::Y_Plus) ) {
            new_y = this_y;
            bCollide = true;
        }
        if (delta_y < 0 && IsBlocked(this_x, this_y, this_z, EFlowDirection::Y_Minus)) {
            new_y = this_y;
            bCollide = true;
        }
    }
    if (FMath::Abs(delta_z) > 1.0f) {
        if (delta_z > 0 && IsBlocked(this_x, this_y, this_z, EFlowDirection::Z_Plus)) {
            new_z = this_z;
            bCollide = true;
        }
        if (delta_z < 0 && IsBlocked(this_x, this_y, this_z, EFlowDirection::Z_Minus)) {
            new_z = this_z;
            bCollide = true;
        }
    }

    if (m_bBoundaryCondition) {
        bCollide = false;
    }
    return bCollide;
}

bool FluidSimulation3D::IsBlocked(int32 x, int32 y, int32 z, EFlowDirection dir) const
{
    if (x == 0 || x == m_size_x - 1) {
        return true;
    }
    if (y == 0 || y == m_size_y - 1) {
        return true;
    }
    if (z == 0 || z == m_size_z - 1) {
        return true;
    }

    switch (dir) {
    case EFlowDirection::X_Plus:
        return x + 1 < m_size_x && EnumHasAllFlags(m_solids->element(x, y, z), dir);
    case EFlowDirection::X_Minus:
        return x - 1 >= 0 && EnumHasAllFlags(m_solids->element(x, y, z), dir);
    case EFlowDirection::Y_Plus:
        return y + 1 < m_size_y && EnumHasAllFlags(m_solids->element(x, y, z), dir);
    case EFlowDirection::Y_Minus:
        return y - 1 >= 0 && EnumHasAllFlags(m_solids->element(x, y, z), dir);
    case EFlowDirection::Z_Plus:
        return z + 1 < m_size_z && EnumHasAllFlags(m_solids->element(x, y, z), dir);
    case EFlowDirection::Z_Minus:
        return z - 1 >= 0 && EnumHasAllFlags(m_solids->element(x, y, z), dir);
    case EFlowDirection::Self:
        return EnumHasAnyFlags(m_solids->element(x, y, z), EFlowDirection::Self);
    default:
        UE_LOG(LogFluidSimulation, Verbose, TEXT("Atmos tried to check with undefined direction!"));
        return false;
    }
}
// Apply acceleration due to pressure
void FluidSimulation3D::PressureAcceleration(const float scale) const
{
    auto force = m_dt * scale;

    mp_velocity->DestinationX() = mp_velocity->SourceX();
    mp_velocity->DestinationY() = mp_velocity->SourceY();
    mp_velocity->DestinationZ() = mp_velocity->SourceZ();

    for (auto x = 0; x < m_size_x - 1; ++x) {
        for (auto y = 0; y < m_size_y - 1; ++y) {
            for (auto z = 0; z < m_size_z - 1; ++z) {
                // Pressure differential between points to get an accelleration force.
                auto src_press = mp_pressure->SourceO2().element(x, y, z) + mp_pressure->SourceN2().element(x, y, z) + mp_pressure->SourceCO2().element(x, y, z) + mp_pressure->SourceToxin().element(x, y, z);

                auto dest_x = mp_pressure->SourceO2().element(x + 1, y, z) + mp_pressure->SourceN2().element(x + 1, y, z) + mp_pressure->SourceCO2().element(x + 1, y, z) + mp_pressure->SourceToxin().element(x + 1, y, z);

                auto dest_y = mp_pressure->SourceO2().element(x, y + 1, z) + mp_pressure->SourceN2().element(x, y + 1, z) + mp_pressure->SourceCO2().element(x, y + 1, z) + mp_pressure->SourceToxin().element(x, y + 1, z);

                auto dest_z = mp_pressure->SourceO2().element(x, y, z + 1) + mp_pressure->SourceN2().element(x, y, z + 1) + mp_pressure->SourceCO2().element(x, y, z + 1) + mp_pressure->SourceToxin().element(x, y, z + 1);

                auto force_x = dest_x - src_press;
                auto force_y = dest_y - src_press;
                auto force_z = dest_z - src_press;

                // Use the acceleration force to move the velocity field in the appropriate direction.
                // Ex. If an area of high pressure exists the acceleration force will turn the velocity field
                // away from this area
                mp_velocity->DestinationX().element(x, y, z) += force * force_x;
                mp_velocity->DestinationX().element(x + 1, y, z) -= force * force_x;

                mp_velocity->DestinationY().element(x, y, z) += force * force_y;
                mp_velocity->DestinationY().element(x, y + 1, z) -= force * force_y;

                mp_velocity->DestinationZ().element(x, y, z) += force * force_z;
                mp_velocity->DestinationZ().element(x, y, z + 1) -= force * force_z;
            }
        }
    }

    mp_velocity->SwapLocationsX();
    mp_velocity->SwapLocationsY();
    mp_velocity->SwapLocationsZ();
}

// Apply a natural deceleration to forces applied to the grids
void FluidSimulation3D::ExponentialDecay(Fluid3D& p_in_and_out, const float decay) const
{
    for (auto x = 0; x < m_size_x; ++x) {
        for (auto y = 0; y < m_size_y; ++y) {
            for (auto z = 0; z < m_size_z; ++z) {
                p_in_and_out.element(x, y, z) = p_in_and_out.element(x, y, z) * FMath::Pow(1 - decay, m_dt);
            }
        }
    }
}

// Apply vorticities to the simulation
void FluidSimulation3D::VorticityConfinement(const float scale) const
{
    mp_velocity->DestinationX().Set(0.0f);
    mp_velocity->DestinationY().Set(0.0f);
    mp_velocity->DestinationZ().Set(0.0f);

    for (auto i = 1; i < m_size_x - 1; ++i) {
        for (auto j = 1; j < m_size_y - 1; ++j) {
            for (auto k = 1; k < m_size_z - 1; ++k) {
                m_curl->element(i, j, k) = FMath::Abs(Curl(i, j, k));
            }
        }
    }

    for (auto x = 1; x < m_size_x - 1; ++x) {
        for (auto y = 1; y < m_size_y - 1; ++y) {
            for (auto z = 1; z < m_size_z - 1; ++z) {
                // Get curl gradient across cells
                auto lr_curl = (m_curl->element(x + 1, y, z) - m_curl->element(x - 1, y, z)) * 0.5f;
                auto ud_curl = (m_curl->element(x, y + 1, z) - m_curl->element(x, y - 1, z)) * 0.5f;
                auto bf_curl = (m_curl->element(x, y, z + 1) - m_curl->element(x, y, z - 1)) * 0.5f;

                // Normalize the derivitive curl vector
                auto length = FMath::Sqrt(lr_curl * lr_curl + ud_curl * ud_curl + bf_curl * bf_curl) + 0.000001f;
                lr_curl /= length;
                ud_curl /= length;
                bf_curl /= length;

                auto magnitude = Curl(x, y, z);

                mp_velocity->DestinationX().element(x, y, z) = -ud_curl * magnitude;
                mp_velocity->DestinationY().element(x, y, z) = lr_curl * magnitude;
                mp_velocity->DestinationZ().element(x, y, z) = bf_curl * magnitude;
            }
        }
    }

    mp_velocity->SourceX() += mp_velocity->DestinationX() * scale;
    mp_velocity->SourceY() += mp_velocity->DestinationY() * scale;
    mp_velocity->SourceZ() += mp_velocity->DestinationZ() * scale;
}

// Calculate the curl at position (x,y,z) in the fluid grid. Physically this represents the vortex strength at the
// cell. Computed as follows: w = (del x U) where U is the velocity vector at (i, j).
float FluidSimulation3D::Curl(const int32 x, const int32 y, const int32 z) const
{
    // difference in XV of cells above and below
    // positive number is a counter-clockwise rotation
    auto x_curl = (mp_velocity->SourceX().element(x, y + 1, z) - mp_velocity->SourceX().element(x, y - 1, z)) * 0.5f;

    // difference in YV of cells left and right
    auto y_curl = (mp_velocity->SourceY().element(x + 1, y, z) - mp_velocity->SourceY().element(x - 1, y, z)) * 0.5f;

    // difference in ZV of cells front and back
    auto z_curl = (mp_velocity->SourceZ().element(x, y, z + 1) - mp_velocity->SourceY().element(x, y, z - 1)) * 0.5f;

    return x_curl - y_curl - z_curl;
}

// Reset the simulation's grids to defaults, does not affect individual parameters
void FluidSimulation3D::Reset() const
{
    mp_pressure->Reset(0.0f);
    mp_velocity->Reset(0.0f);
    m_solids->Set(EFlowDirection::None);
}
