//-------------------------------------------------------------------------------------
//
// Copyright 2009 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works
// of this software for any purpose and without fee, provided, that the above
// copyright notice and this statement appear in all copies.  Intel makes no
// representations about the suitability of this software for any purpose.  THIS
// SOFTWARE IS PROVIDED "AS IS." INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES,
// EXPRESS OR IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER
// INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE, INCLUDING LIABILITY FOR
// INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not assume
// any responsibility for any errors which may appear in this software nor any
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

DECLARE_CYCLE_STAT(TEXT("Fluid simulation update"), STAT_AtmosUpdatesCount, STATGROUP_AtmosStats)
DECLARE_CYCLE_STAT(TEXT("Fluid simulation update: diffusion"), STAT_AtmosDiffusionUpdatesCount, STATGROUP_AtmosStats)
DECLARE_CYCLE_STAT(TEXT("Fluid simulation update: forces"), STAT_AtmosForcesUpdatesCount, STATGROUP_AtmosStats)
DECLARE_CYCLE_STAT(TEXT("Fluid simulation update: advection"), STAT_AtmosAdvectionUpdatesCount, STATGROUP_AtmosStats)
DECLARE_CYCLE_STAT(TEXT("Fluid simulation stable diffusion"), STAT_AtmosStableDiffusion, STATGROUP_AtmosStats)

FluidSimulation3D::FluidSimulation3D(int32 xSize, int32 ySize, int32 zSize, float dt)
  : m_solids(xSize - 1, ySize - 1, zSize - 1)
  , m_curl(xSize, ySize, zSize)
  , m_velocity(xSize, ySize, zSize)
  , m_pressure(xSize, ySize, zSize)
  , m_diffusionIter(1)
  , m_vorticity(0.0)
  , m_pressureAccel(0.0)
  , m_dt(dt)
  , m_sizeX(xSize)
  , m_sizeY(ySize)
  , m_sizeZ(zSize)
{
    reset();
}

// Update is called every frame or as specified in the max desired updates per
// second GUI slider
void FluidSimulation3D::update()
{
    SCOPE_CYCLE_COUNTER(STAT_AtmosUpdatesCount)
    updateDiffusion();
    updateForces();
    updateAdvection();
}

// Apply diffusion across the grids
void FluidSimulation3D::updateDiffusion()
{
    SCOPE_CYCLE_COUNTER(STAT_AtmosDiffusionUpdatesCount)
    // Skip diffusion if disabled
    // Diffusion of Velocity
    if(!FMath::IsNearlyZero(m_velocity.properties().diffusion))
    {
        const auto scaledVelocity = m_velocity.properties().diffusion / static_cast<float>(m_diffusionIter);

        for(auto i = 0; i < m_diffusionIter; ++i)
        {
            diffusionStable(m_velocity.sourceX(), m_velocity.destinationX(), scaledVelocity);
            diffusionStable(m_velocity.sourceY(), m_velocity.destinationY(), scaledVelocity);
            diffusionStable(m_velocity.sourceZ(), m_velocity.destinationZ(), scaledVelocity);
            m_velocity.swap();
        }
    }

    // Diffusion of Pressure
    if(!FMath::IsNearlyZero(m_pressure.properties().diffusion))
    {
        const auto scaledDiffusion = m_pressure.properties().diffusion / static_cast<float>(m_diffusionIter);
        for(auto i = 0; i < m_diffusionIter; ++i)
        {
            diffusionStable(m_pressure.sourceO2(), m_pressure.destinationO2(), scaledDiffusion);
            diffusionStable(m_pressure.sourceCO2(), m_pressure.destinationCO2(), scaledDiffusion);
            diffusionStable(m_pressure.sourceN2(), m_pressure.destinationN2(), scaledDiffusion);
            diffusionStable(m_pressure.sourceToxin(), m_pressure.destinationToxin(), scaledDiffusion);
            m_pressure.swap();
        }
    }
}

// Apply forces across the grids
void FluidSimulation3D::updateForces()
{
    SCOPE_CYCLE_COUNTER(STAT_AtmosForcesUpdatesCount)
    // Apply dampening force on velocity due to viscosity
    if(!FMath::IsNearlyZero(m_velocity.properties().decay))
    {
        exponentialDecay(m_velocity.destinationX(), m_velocity.properties().decay);
        exponentialDecay(m_velocity.destinationY(), m_velocity.properties().decay);
        exponentialDecay(m_velocity.destinationZ(), m_velocity.properties().decay);
        m_velocity.swap();
    }

    // Apply equilibrium force on pressure for mass conservation
    if(!FMath::IsNearlyZero(m_pressureAccel))
    {
        pressureAcceleration(m_pressureAccel);
    }

    // Apply curl force on vorticies to prevent artificial dampening
    if(!FMath::IsNearlyZero(m_vorticity))
    {
        vorticityConfinement(m_vorticity);
    }
}

// Apply advection across the grids
void FluidSimulation3D::updateAdvection()
{
    SCOPE_CYCLE_COUNTER(STAT_AtmosAdvectionUpdatesCount)
    const auto avgDimension = (m_sizeX + m_sizeY + m_sizeZ) / 3.0f;
    const auto stdDimension = 100.0f;

    // Change advection scale depending on grid size. Smaller grids means larger
    // cells, so scale should be smaller. Average dimension size of std_dimension
    // value (100) equals an advection_scale of 1
    const auto advectionScale = avgDimension / stdDimension;

    // Advection order makes significant differences
    // Advecting pressure first leads to self-maintaining waves and ripple
    // artifacts Advecting velocity first naturally dissipates the waves

    // Advect Velocity
    forwardAdvection(
      m_velocity.sourceX(), m_velocity.destinationX(), m_velocity.properties().advection * advectionScale);
    forwardAdvection(
      m_velocity.sourceY(), m_velocity.destinationY(), m_velocity.properties().advection * advectionScale);
    forwardAdvection(
      m_velocity.sourceZ(), m_velocity.destinationZ(), m_velocity.properties().advection * advectionScale);

    reverseSignedAdvection(m_velocity, m_velocity.properties().advection * advectionScale);

    /*SetBoundary(mp_velocity.sourceX());
    SetBoundary(mp_velocity.sourceY());
    SetBoundary(mp_velocity.sourceZ());*/

    // Advect Pressure. Represents compressible fluid
    forwardAdvection(
      m_pressure.sourceO2(), m_pressure.destinationO2(), m_pressure.properties().advection * advectionScale);
    forwardAdvection(
      m_pressure.sourceN2(), m_pressure.destinationN2(), m_pressure.properties().advection * advectionScale);
    forwardAdvection(
      m_pressure.sourceCO2(), m_pressure.destinationCO2(), m_pressure.properties().advection * advectionScale);
    forwardAdvection(
      m_pressure.sourceToxin(), m_pressure.destinationToxin(), m_pressure.properties().advection * advectionScale);
    m_pressure.swap();
    reverseAdvection(
      m_pressure.sourceO2(), m_pressure.destinationO2(), m_pressure.properties().advection * advectionScale);
    reverseAdvection(
      m_pressure.sourceN2(), m_pressure.destinationN2(), m_pressure.properties().advection * advectionScale);
    reverseAdvection(
      m_pressure.sourceCO2(), m_pressure.destinationCO2(), m_pressure.properties().advection * advectionScale);
    reverseAdvection(
      m_pressure.sourceToxin(), m_pressure.destinationToxin(), m_pressure.properties().advection * advectionScale);
    m_pressure.swap();
}

void FluidSimulation3D::forwardAdvection(const Fluid3D& in, Fluid3D& out, float scale) const
{
    const auto force = m_dt * scale; // distance to advect

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

    // Copy source to destination as forward advection results in
    // adding/subtracing not moving
    out = in;

    if(FMath::IsNearlyZero(force))
    {
        return;
    }

    // This can easily be threaded as the input array is independent from the
    // output array
    for(auto x = 1; x < m_sizeX - 1; ++x)
    {
        for(auto y = 1; y < m_sizeY - 1; ++y)
        {
            for(auto z = 1; z < m_sizeZ - 1; ++z)
            {
                const auto vx = m_velocity.sourceX().element(x, y, z);
                const auto vy = m_velocity.sourceY().element(x, y, z);
                const auto vz = m_velocity.sourceZ().element(x, y, z);
                if(!FMath::IsNearlyZero(vx) || !FMath::IsNearlyZero(vy) || !FMath::IsNearlyZero(vz))
                {
                    // Find the floating point location of the forward advection
                    auto x1 = x + vx * force;
                    auto y1 = y + vy * force;
                    auto z1 = z + vz * force;

                    // Check for and correct boundary collisions
                    collide(x, y, z, x1, y1, z1);

                    // Find the nearest top-left integer grid point of the advection
                    const auto x1A = FMath::FloorToInt(x1);
                    const auto y1A = FMath::FloorToInt(y1);
                    const auto z1A = FMath::FloorToInt(z1);

                    // Store the fractional parts
                    const auto fx1 = x1 - x1A;
                    const auto fy1 = y1 - y1A;
                    const auto fz1 = z1 - z1A;

                    // The floating point location after forward advection (x1,y1,z1) will
                    // land within an 8 point cube (A,B,C,D,E,F,G,H). Distribute the value
                    // of the source point among the destination grid points using
                    // bilinear interoplation. Subtract the total value given to the
                    // destination grid points from the source point.

                    // Pull source value from the unmodified p_in
                    const auto sourceValue = in.element(x, y, z);

                    // Bilinear interpolation
                    auto A = (1.0f - fz1) * (1.0f - fy1) * (1.0f - fx1) * sourceValue;
                    auto B = (1.0f - fz1) * (1.0f - fy1) * fx1 * sourceValue;
                    auto C = (1.0f - fz1) * fy1 * (1.0f - fx1) * sourceValue;
                    auto D = (1.0f - fz1) * fy1 * fx1 * sourceValue;
                    auto E = fz1 * (1.0f - fy1) * (1.0f - fx1) * sourceValue;
                    auto F = fz1 * (1.0f - fy1) * fx1 * sourceValue;
                    auto G = fz1 * fy1 * (1.0f - fx1) * sourceValue;
                    auto H = fz1 * fy1 * fx1 * sourceValue;

                    // Add A,B,C,D,E,F,G,H to the eight destination cells
                    out.element(x1A, y1A, z1A) += A;
                    out.element(x1A + 1, y1A, z1A) += B;
                    out.element(x1A, y1A + 1, z1A) += C;
                    out.element(x1A + 1, y1A + 1, z1A) += D;
                    out.element(x1A, y1A, z1A + 1) += E;
                    out.element(x1A + 1, y1A, z1A + 1) += F;
                    out.element(x1A, y1A + 1, z1A + 1) += G;
                    out.element(x1A + 1, y1A + 1, z1A + 1) += H;

                    // Subtract A-H from source for mass conservation
                    out.element(x, y, z) -= A + B + C + D + E + F + G + H;
                }
            }
        }
    }
}

void FluidSimulation3D::reverseAdvection(const Fluid3D& in, Fluid3D& out, float scale) const
{
    const auto force = m_dt * scale; // distance to advect
    int32 x1A, y1A, z1A; // x, y, z locations of top-left-back grid point (A) after advection
    float A, // top-left-back grid point value after advection
      B, // top-right-back grid point value after advection
      C, // bottom-left-back grid point value after advection
      D, // bottom-right-back grid point value after advection
      E, // top-left-front grid point value after advection
      F, // top-right-front grid point value after advection
      G, // bottom-left-front grid point value after advection
      H; // bottom-right-front grid point value after advection

    // Copy source to destination as reverse advection results in
    // adding/subtracing not moving
    out = in;

    if(FMath::IsNearlyZero(force))
    {
        return;
    }

    // we need to zero out the fractions
    // The new X coordinate after advection stored in x,y,z where x,y,z,z is the
    // original source point
    TArray3D<int32> FromSource_xA(m_sizeX, m_sizeY, m_sizeZ, -1.f);
    // The new Y coordinate after advection stored in x,y,z where x,y,z is the
    // original source point
    TArray3D<int32> FromSource_yA(m_sizeX, m_sizeY, m_sizeZ, -1.f);
    // The new Z coordinate after advection stored in x,y,z where x,y,z is the
    // original source point
    TArray3D<int32> FromSource_zA(m_sizeX, m_sizeY, m_sizeZ, -1.f);
    // The value of A after advection stored in x,y,z where x,y,z is the original
    // source point
    TArray3D<float> FromSource_A(m_sizeX, m_sizeY, m_sizeZ);
    // The value of B after advection stored in x,y,z where x,y,z is the original
    // source point
    TArray3D<float> FromSource_B(m_sizeX, m_sizeY, m_sizeZ);
    // The value of C after advection stored in x,y,z where x,y,z is the original
    // source point
    TArray3D<float> FromSource_C(m_sizeX, m_sizeY, m_sizeZ);
    // The value of D after advection stored in x,y,z where x,y,z is the original
    // source point
    TArray3D<float> FromSource_D(m_sizeX, m_sizeY, m_sizeZ);
    // The value of E after advection stored in x,y,z where x,y,z is the original
    // source point
    TArray3D<float> FromSource_E(m_sizeX, m_sizeY, m_sizeZ);
    // The value of F after advection stored in x,y,z where x,y,z is the original
    // source point
    TArray3D<float> FromSource_F(m_sizeX, m_sizeY, m_sizeZ);
    // The value of G after advection stored in x,y,z where x,y,z is the original
    // source point
    TArray3D<float> FromSource_G(m_sizeX, m_sizeY, m_sizeZ);
    // The value of H after advection stored in x,y,z where x,y,z is the original
    // source point
    TArray3D<float> FromSource_H(m_sizeX, m_sizeY, m_sizeZ);
    // The total accumulated value after advection stored in x,y,z where x,y,z is
    // the destination point
    TArray3D<float> TotalDestValue(m_sizeX, m_sizeY, m_sizeZ);

    // This can easily be threaded as the input array is independent from the
    // output array
    for(auto x = 1; x < m_sizeX - 1; ++x)
    {
        for(auto y = 1; y < m_sizeY - 1; ++y)
        {
            for(auto z = 1; z < m_sizeZ - 1; ++z)
            {
                auto vx = m_velocity.sourceX().element(x, y, z);
                auto vy = m_velocity.sourceY().element(x, y, z);
                auto vz = m_velocity.sourceZ().element(x, y, z);
                if(!FMath::IsNearlyZero(vx) || !FMath::IsNearlyZero(vy) || !FMath::IsNearlyZero(vz))
                {
                    // Find the floating point location of the advection
                    auto x1 = x + vx * force;
                    auto y1 = y + vy * force;
                    auto z1 = z + vz * force;

                    // Check for and correct boundary collisions
                    collide(x, y, z, x1, y1, z1);

                    // Find the nearest top-left integer grid point of the advection
                    x1A = FMath::FloorToInt(x1);
                    y1A = FMath::FloorToInt(y1);
                    z1A = FMath::FloorToInt(z1);

                    // Store the fractional parts
                    const auto fx1 = x1 - x1A;
                    const auto fy1 = y1 - y1A;
                    const auto fz1 = z1 - z1A;

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
                    By adding the source value into the destination, we handle the problem
                    of multiple destinations but by subtracting it from the source we
                    gloss over the problem of multiple sources. Suppose multiple
                    destinations have the same (partial) source cells, then what happens
                    is the first dest that is processed will get all of that source cell
                    (or all of the fraction it needs).  Subsequent dest cells will get a
                    reduced fraction.  In extreme cases this will lead to holes forming
                    based on the update order.

                    Solution:  Maintain an array for dest cells, and source cells.
                    For dest cells, store the eight source cells and the eight fractions
                    For source cells, store the number of dest cells that source from
                    here, and the total fraction E.G.  Dest cells A, B, C all source from
                    cell D (and explicit others XYZ, which we don't need to store) So,
                    dest cells store A->D(0.1)XYZ..., B->D(0.5)XYZ.... C->D(0.7)XYZ...
                    Source Cell D is updated with A, B then C
                    Update A:   Dests = 1, Tot = 0.1
                    Update B:   Dests = 2, Tot = 0.6
                    Update C:   Dests = 3, Tot = 1.3

                    How much should go to each of A, B and C? They are asking for a total
                    of 1.3, so should they get it all, or should they just get 0.4333 in
                    total? Ad Hoc answer: if total <=1 then they get what they ask for if
                    total >1 then is is divided between them proportionally. If there were
                    two at 1.0, they would get 0.5 each If there were two at 0.5, they
                    would get 0.5 each If there were two at 0.1, they would get 0.1 each
                    If there were one at 0.6 and one at 0.8, they would get 0.6/1.4 and
                    0.8/1.4  (0.429 and 0.571) each

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

                    // Store the coordinates of destination point A for this source point
                    // (x,y,z)
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

    for(auto y = 1; y < m_sizeY - 1; ++y)
    {
        for(auto x = 1; x < m_sizeX - 1; ++x)
        {
            for(auto z = 1; z < m_sizeZ - 1; ++z)
            {
                if(FromSource_xA.element(x, y, z) != -1.f)
                {
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
                    if(A_Total < 1.0f)
                        A_Total = 1.0f;
                    if(B_Total < 1.0f)
                        B_Total = 1.0f;
                    if(C_Total < 1.0f)
                        C_Total = 1.0f;
                    if(D_Total < 1.0f)
                        D_Total = 1.0f;
                    if(E_Total < 1.0f)
                        E_Total = 1.0f;
                    if(F_Total < 1.0f)
                        F_Total = 1.0f;
                    if(G_Total < 1.0f)
                        G_Total = 1.0f;
                    if(H_Total < 1.0f)
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
                    // So we are taking fractions from p_in, but not altering those values
                    // as they are used again by later cells if the field were mass
                    // conserving, then we could simply move the value but if we try that
                    // we lose mass
                    out.element(x, y, z) += A * in.element(x1A, y1A, z1A) + B * in.element(x1A + 1, y1A, z1A) +
                                            C * in.element(x1A, y1A + 1, z1A) + D * in.element(x1A + 1, y1A + 1, z1A) +
                                            E * in.element(x1A, y1A, z1A + 1) + F * in.element(x1A + 1, y1A, z1A + 1) +
                                            G * in.element(x1A, y1A + 1, z1A + 1) +
                                            H * in.element(x1A + 1, y1A + 1, z1A + 1);

                    // Subtract the values added to the destination from the source for
                    // mass conservation
                    out.element(x1A, y1A, z1A) -= A * in.element(x1A, y1A, z1A);
                    out.element(x1A + 1, y1A, z1A) -= B * in.element(x1A + 1, y1A, z1A);
                    out.element(x1A, y1A + 1, z1A) -= C * in.element(x1A, y1A + 1, z1A);
                    out.element(x1A + 1, y1A + 1, z1A) -= D * in.element(x1A + 1, y1A + 1, z1A);
                    out.element(x1A, y1A, z1A + 1) -= E * in.element(x1A, y1A, z1A + 1);
                    out.element(x1A + 1, y1A, z1A + 1) -= F * in.element(x1A + 1, y1A, z1A + 1);
                    out.element(x1A, y1A + 1, z1A + 1) -= G * in.element(x1A, y1A + 1, z1A + 1);
                    out.element(x1A + 1, y1A + 1, z1A + 1) -= H * in.element(x1A + 1, y1A + 1, z1A + 1);
                }
            }
        }
    }
}

inline float FluidSimulation3D::transferPressure(const Fluid3D& in, int32 x, int32 y, int32 z, float force) const
{
    QUICK_SCOPE_CYCLE_COUNTER(TransferPressure);
    // Take care of boundries
    if(isBlocked(x, y, z, EFlowDirection::Self))
    {
        return 0.f;
    }
    auto d = 0.f;
    auto c = 0.f;
    if(!isBlocked(x, y, z, EFlowDirection::XPlus))
    {
        c += in.element(x + 1, y, z);
        ++d;
    }
    if(!isBlocked(x, y, z, EFlowDirection::XMinus))
    {
        c += in.element(x - 1, y, z);
        ++d;
    }
    if(!isBlocked(x, y, z, EFlowDirection::YPlus))
    {
        c += in.element(x, y + 1, z);
        ++d;
    }
    if(!isBlocked(x, y, z, EFlowDirection::YMinus))
    {
        c += in.element(x, y - 1, z);
        ++d;
    }
    if(!isBlocked(x, y, z, EFlowDirection::ZPlus))
    {
        c += in.element(x, y, z + 1);
        ++d;
    }
    if(!isBlocked(x, y, z, EFlowDirection::ZMinus))
    {
        c += in.element(x, y, z - 1);
        ++d;
    }
    return in.element(x, y, z) + force * (c - d * in.element(x, y, z));
}

void FluidSimulation3D::diffusionStable(const Fluid3D& in, Fluid3D& out, float scale) const
{
    SCOPE_CYCLE_COUNTER(STAT_AtmosStableDiffusion)
    const auto force = m_dt * scale;

    if(FMath::IsNegativeFloat(force) || FMath::IsNearlyZero(force))
        return;

    for(auto x = 0; x < m_sizeX; ++x)
    {
        for(auto y = 0; y < m_sizeY; ++y)
        {
            for(auto z = 0; z < m_sizeZ; ++z)
            {
                out.element(x, y, z) = transferPressure(in, x, y, z, force);
            }
        }
    }
}

// Signed advection is mass conserving, but allows signed quantities
// so could be used for velocity, since it's faster.
void FluidSimulation3D::reverseSignedAdvection(VelPkg3D& v, const float scale) const
{
    // negate advection scale, since it's reverse advection
    const auto force = -m_dt * scale;

    if(FMath::IsNearlyZero(scale))
    {
        return;
    }
    // First copy the scalar values over, since we are adding/subtracting in
    // values, not moving things
    auto velOutX = v.destinationX();
    auto velOutY = v.destinationY();
    auto velOutZ = v.destinationZ();

    for(auto x = 1; x < m_sizeX - 1; ++x)
    {
        for(auto y = 1; y < m_sizeY - 1; ++y)
        {
            for(auto z = 1; z < m_sizeZ - 1; ++z)
            {
                auto vx = m_velocity.sourceX().element(x, y, z);
                auto vy = m_velocity.sourceY().element(x, y, z);
                auto vz = m_velocity.sourceZ().element(x, y, z);
                if(!FMath::IsNearlyZero(vx) || !FMath::IsNearlyZero(vy) || !FMath::IsNearlyZero(vz))
                {
                    // Find the floating point location of the advection
                    // x, y, z locations after advection
                    auto x1 = x + vx * force;
                    auto y1 = y + vy * force;
                    auto z1 = z + vz * force;

                    const auto bCollide = collide(x, y, z, x1, y1, z1);

                    // Find the nearest top-left integer grid point of the advection
                    auto x1A = FMath::FloorToInt(x1);
                    auto y1A = FMath::FloorToInt(y1);
                    auto z1A = FMath::FloorToInt(z1);

                    // Store the fractional parts
                    auto fx1 = x1 - x1A;
                    auto fy1 = y1 - y1A;
                    auto fz1 = z1 - z1A;

                    // Get amounts from (in) source cells for X velocity
                    auto A_X = (1.0f - fx1) * (1.0f - fy1) * (1.0f - fz1) * v.destinationX().element(x1A, y1A, z1A);
                    auto B_X = fx1 * (1.0f - fy1) * (1.0f - fz1) * v.destinationX().element(x1A + 1, y1A, z1A);
                    auto C_X = (1.0f - fx1) * fy1 * (1.0f - fz1) * v.destinationX().element(x1A, y1A + 1, z1A);
                    auto D_X = fx1 * fy1 * (1.0f - fz1) * v.destinationX().element(x1A + 1, y1A + 1, z1A);
                    auto E_X = (1.0f - fx1) * (1.0f - fy1) * fz1 * v.destinationX().element(x1A, y1A, z1A + 1);
                    auto F_X = fx1 * (1.0f - fy1) * fz1 * v.destinationX().element(x1A + 1, y1A, z1A + 1);
                    auto G_X = (1.0f - fx1) * fy1 * fz1 * v.destinationX().element(x1A, y1A + 1, z1A + 1);
                    auto H_X = fx1 * fy1 * fz1 * v.destinationX().element(x1A + 1, y1A + 1, z1A + 1);

                    // Get amounts from (in) source cells for Y velocity
                    auto A_Y = (1.0f - fx1) * (1.0f - fy1) * (1.0f - fz1) * v.destinationY().element(x1A, y1A, z1A);
                    auto B_Y = fx1 * (1.0f - fy1) * (1.0f - fz1) * v.destinationY().element(x1A + 1, y1A, z1A);
                    auto C_Y = (1.0f - fx1) * fy1 * (1.0f - fz1) * v.destinationY().element(x1A, y1A + 1, z1A);
                    auto D_Y = fx1 * fy1 * (1.0f - fz1) * v.destinationY().element(x1A + 1, y1A + 1, z1A);
                    auto E_Y = (1.0f - fx1) * (1.0f - fy1) * fz1 * v.destinationY().element(x1A, y1A, z1A + 1);
                    auto F_Y = fx1 * (1.0f - fy1) * fz1 * v.destinationY().element(x1A + 1, y1A, z1A + 1);
                    auto G_Y = (1.0f - fx1) * fy1 * fz1 * v.destinationY().element(x1A, y1A + 1, z1A + 1);
                    auto H_Y = fx1 * fy1 * fz1 * v.destinationY().element(x1A + 1, y1A + 1, z1A + 1);

                    // Get amounts from (in) source cells for Z velocity
                    auto A_Z = (1.0f - fx1) * (1.0f - fy1) * (1.0f - fz1) * v.destinationZ().element(x1A, y1A, z1A);
                    auto B_Z = fx1 * (1.0f - fy1) * (1.0f - fz1) * v.destinationZ().element(x1A + 1, y1A, z1A);
                    auto C_Z = (1.0f - fx1) * fy1 * (1.0f - fz1) * v.destinationZ().element(x1A, y1A + 1, z1A);
                    auto D_Z = fx1 * fy1 * (1.0f - fz1) * v.destinationZ().element(x1A + 1, y1A + 1, z1A);
                    auto E_Z = (1.0f - fx1) * (1.0f - fy1) * fz1 * v.destinationZ().element(x1A, y1A, z1A + 1);
                    auto F_Z = fx1 * (1.0f - fy1) * fz1 * v.destinationZ().element(x1A + 1, y1A, z1A + 1);
                    auto G_Z = (1.0f - fx1) * fy1 * fz1 * v.destinationZ().element(x1A, y1A + 1, z1A + 1);
                    auto H_Z = fx1 * fy1 * fz1 * v.destinationZ().element(x1A + 1, y1A + 1, z1A + 1);

                    // X Velocity
                    // add to (out) source cell
                    if(!bCollide)
                    {
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
                    if(!bCollide)
                    {
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
                    if(!bCollide)
                    {
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
    v.destinationX() = velOutX;
    v.destinationY() = velOutY;
    v.destinationZ() = velOutZ;
    v.swap();
}

// Checks if destination point during advection is out of bounds and pulls point
// in if needed
bool FluidSimulation3D::collide(int32 thisX, int32 thisY, int32 thisZ, float& newX, float& newY, float& newZ) const
{
    const auto maxAdvect = 1.5f - KINDA_SMALL_NUMBER; // 1.5 - is center of neighbor cell
    auto bCollide = false;

    const auto deltaX = FMath::Clamp<float>(newX - thisX, -maxAdvect, maxAdvect);
    const auto deltaY = FMath::Clamp<float>(newY - thisY, -maxAdvect, maxAdvect);
    const auto deltaZ = FMath::Clamp<float>(newZ - thisZ, -maxAdvect, maxAdvect);

    newX = thisX + deltaX;
    newY = thisY + deltaY;
    newZ = thisZ + deltaZ;

    if(newX < 1 || newX >= m_sizeX - 1)
    {
        newX = thisX;
        bCollide = true;
    }
    if(newY < 1 || newY >= m_sizeY - 1)
    {
        newY = thisY;
        bCollide = true;
    }
    if(newZ < 1 || newZ >= m_sizeZ - 1)
    {
        newZ = thisZ;
        bCollide = true;
    }

    if(isBlocked(thisX, thisY, thisZ, EFlowDirection::Self))
    {
        newX = thisX;
        newY = thisY;
        newZ = thisZ;
        bCollide = true;
    }
    if(FMath::Abs(deltaX) > 1.0f)
    {
        if(deltaX > 0 && isBlocked(thisX, thisY, thisZ, EFlowDirection::XPlus))
        {
            newX = thisX;
            bCollide = true;
        }
        if(deltaX < 0 && isBlocked(thisX, thisY, thisZ, EFlowDirection::XMinus))
        {
            newX = thisX;
            bCollide = true;
        }
    }
    if(FMath::Abs(deltaY) > 1.0f)
    {
        if(deltaY > 0 && isBlocked(thisX, thisY, thisZ, EFlowDirection::YPlus))
        {
            newY = thisY;
            bCollide = true;
        }
        if(deltaY < 0 && isBlocked(thisX, thisY, thisZ, EFlowDirection::YMinus))
        {
            newY = thisY;
            bCollide = true;
        }
    }
    if(FMath::Abs(deltaZ) > 1.0f)
    {
        if(deltaZ > 0 && isBlocked(thisX, thisY, thisZ, EFlowDirection::ZPlus))
        {
            newZ = thisZ;
            bCollide = true;
        }
        if(deltaZ < 0 && isBlocked(thisX, thisY, thisZ, EFlowDirection::ZMinus))
        {
            newZ = thisZ;
            bCollide = true;
        }
    }

    return bCollide;
}

bool FluidSimulation3D::isBlocked(int32 x, int32 y, int32 z, EFlowDirection dir) const
{
    if(x == 0 || x == m_sizeX - 1)
    {
        return true;
    }
    if(y == 0 || y == m_sizeY - 1)
    {
        return true;
    }
    if(z == 0 || z == m_sizeZ - 1)
    {
        return true;
    }

    switch(dir)
    {
    case EFlowDirection::XPlus: return x + 1 < m_sizeX && EnumHasAllFlags(m_solids.element(x, y, z), dir);
    case EFlowDirection::XMinus: return x - 1 >= 0 && EnumHasAllFlags(m_solids.element(x, y, z), dir);
    case EFlowDirection::YPlus: return y + 1 < m_sizeY && EnumHasAllFlags(m_solids.element(x, y, z), dir);
    case EFlowDirection::YMinus: return y - 1 >= 0 && EnumHasAllFlags(m_solids.element(x, y, z), dir);
    case EFlowDirection::ZPlus: return z + 1 < m_sizeZ && EnumHasAllFlags(m_solids.element(x, y, z), dir);
    case EFlowDirection::ZMinus: return z - 1 >= 0 && EnumHasAllFlags(m_solids.element(x, y, z), dir);
    case EFlowDirection::Self: return EnumHasAnyFlags(m_solids.element(x, y, z), EFlowDirection::Self);
    default: UE_LOG(LogFluidSimulation, Verbose, TEXT("Atmos tried to check with undefined direction!")); return false;
    }
}
// Apply acceleration due to pressure
void FluidSimulation3D::pressureAcceleration(const float scale)
{
    const auto force = m_dt * scale;

    m_velocity.destinationX() = m_velocity.sourceX();
    m_velocity.destinationY() = m_velocity.sourceY();
    m_velocity.destinationZ() = m_velocity.sourceZ();

    for(auto x = 0; x < m_sizeX - 1; ++x)
    {
        for(auto y = 0; y < m_sizeY - 1; ++y)
        {
            for(auto z = 0; z < m_sizeZ - 1; ++z)
            {
                // Pressure differential between points to get an accelleration force.
                const auto srcPress = m_pressure.sourceO2().element(x, y, z) + m_pressure.sourceN2().element(x, y, z) +
                                      m_pressure.sourceCO2().element(x, y, z) +
                                      m_pressure.sourceToxin().element(x, y, z);

                const auto destX =
                  m_pressure.sourceO2().element(x + 1, y, z) + m_pressure.sourceN2().element(x + 1, y, z) +
                  m_pressure.sourceCO2().element(x + 1, y, z) + m_pressure.sourceToxin().element(x + 1, y, z);

                const auto destY =
                  m_pressure.sourceO2().element(x, y + 1, z) + m_pressure.sourceN2().element(x, y + 1, z) +
                  m_pressure.sourceCO2().element(x, y + 1, z) + m_pressure.sourceToxin().element(x, y + 1, z);

                const auto destZ =
                  m_pressure.sourceO2().element(x, y, z + 1) + m_pressure.sourceN2().element(x, y, z + 1) +
                  m_pressure.sourceCO2().element(x, y, z + 1) + m_pressure.sourceToxin().element(x, y, z + 1);

                const auto forceX = destX - srcPress;
                const auto forceY = destY - srcPress;
                const auto forceZ = destZ - srcPress;

                // Use the acceleration force to move the velocity field in the
                // appropriate direction. Ex. If an area of high pressure exists the
                // acceleration force will turn the velocity field away from this area
                m_velocity.destinationX().element(x, y, z) += force * forceX;
                m_velocity.destinationX().element(x + 1, y, z) -= force * forceX;

                m_velocity.destinationY().element(x, y, z) += force * forceY;
                m_velocity.destinationY().element(x, y + 1, z) -= force * forceY;

                m_velocity.destinationZ().element(x, y, z) += force * forceZ;
                m_velocity.destinationZ().element(x, y, z + 1) -= force * forceZ;
            }
        }
    }

    m_velocity.swap();
}

// Apply a natural deceleration to forces applied to the grids
void FluidSimulation3D::exponentialDecay(Fluid3D& data, float decay) const
{
    for(auto x = 0; x < m_sizeX; ++x)
    {
        for(auto y = 0; y < m_sizeY; ++y)
        {
            for(auto z = 0; z < m_sizeZ; ++z)
            {
                data.element(x, y, z) = data.element(x, y, z) * FMath::Pow(1 - decay, m_dt);
            }
        }
    }
}

// Apply vorticities to the simulation
void FluidSimulation3D::vorticityConfinement(const float scale)
{
    m_velocity.destinationX().set(0.0f);
    m_velocity.destinationY().set(0.0f);
    m_velocity.destinationZ().set(0.0f);

    for(auto i = 1; i < m_sizeX - 1; ++i)
    {
        for(auto j = 1; j < m_sizeY - 1; ++j)
        {
            for(auto k = 1; k < m_sizeZ - 1; ++k)
            {
                m_curl.element(i, j, k) = FMath::Abs(curl(i, j, k));
            }
        }
    }

    for(auto x = 1; x < m_sizeX - 1; ++x)
    {
        for(auto y = 1; y < m_sizeY - 1; ++y)
        {
            for(auto z = 1; z < m_sizeZ - 1; ++z)
            {
                // Get curl gradient across cells
                auto lrCurl = (m_curl.element(x + 1, y, z) - m_curl.element(x - 1, y, z)) * 0.5f;
                auto udCurl = (m_curl.element(x, y + 1, z) - m_curl.element(x, y - 1, z)) * 0.5f;
                auto bfCurl = (m_curl.element(x, y, z + 1) - m_curl.element(x, y, z - 1)) * 0.5f;

                // Normalize the derivitive curl vector
                const auto length = FMath::Sqrt(lrCurl * lrCurl + udCurl * udCurl + bfCurl * bfCurl) + 0.000001f;
                lrCurl /= length;
                udCurl /= length;
                bfCurl /= length;

                const auto magnitude = curl(x, y, z);

                m_velocity.destinationX().element(x, y, z) = -udCurl * magnitude;
                m_velocity.destinationY().element(x, y, z) = lrCurl * magnitude;
                m_velocity.destinationZ().element(x, y, z) = bfCurl * magnitude;
            }
        }
    }
    m_velocity.destinationX() *= scale;
    m_velocity.destinationY() *= scale;
    m_velocity.destinationZ() *= scale;
    m_velocity.destinationX() += m_velocity.sourceX();
    m_velocity.destinationY() += m_velocity.sourceY();
    m_velocity.destinationZ() += m_velocity.sourceZ();
    m_velocity.swap();
}

// Calculate the curl at position (x,y,z) in the fluid grid. Physically this
// represents the vortex strength at the cell. Computed as follows: w = (del x
// U) where U is the velocity vector at (i, j).
float FluidSimulation3D::curl(const int32 x, const int32 y, const int32 z) const
{
    // difference in XV of cells above and below
    // positive number is a counter-clockwise rotation
    const auto xCurl = (m_velocity.sourceX().element(x, y + 1, z) - m_velocity.sourceX().element(x, y - 1, z)) * 0.5f;

    // difference in YV of cells left and right
    const auto yCurl = (m_velocity.sourceY().element(x + 1, y, z) - m_velocity.sourceY().element(x - 1, y, z)) * 0.5f;

    // difference in ZV of cells front and back
    const auto zCurl = (m_velocity.sourceZ().element(x, y, z + 1) - m_velocity.sourceY().element(x, y, z - 1)) * 0.5f;

    return xCurl - yCurl - zCurl;
}

// Reset the simulation's grids to defaults, does not affect individual
// parameters
void FluidSimulation3D::reset()
{
    m_pressure.reset(0.0f);
    m_velocity.reset(0.0f);
    m_solids.set(EFlowDirection::None);
}
