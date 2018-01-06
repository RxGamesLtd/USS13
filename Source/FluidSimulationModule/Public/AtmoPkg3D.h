// The MIT License (MIT)
// Copyright (c) 2018 RxCompile
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions
// of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
// OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "Fluid3D.h"
#include "FluidPkg3D.h"
#include "FluidProperties.h"

#include "EngineMinimal.h"

UENUM()
namespace EGasType {
enum Type
{
    O2 = 0,
    N2,
    CO2,
    Toxin,
    GasTypeCount // should be last
};
} // namespace EGasType

class FLUIDSIMULATIONMODULE_API AtmoPkg3D
{
public:
    // Constructor - Initilizes source and destination FLuid3D objects for atmo in X, Y, Z directions
    AtmoPkg3D(int32 x, int32 y, int32 z);

    // Swap the source and destination objects
    void swap();

    // Reset the source and destination objects to specified value
    void reset(float value);

    // Accessors
    const Fluid3D& sourceO2() const { return m_data[EGasType::O2].source(); }
    Fluid3D& destinationO2() { return m_data[EGasType::O2].destination(); }
    const Fluid3D& sourceN2() const { return m_data[EGasType::N2].source(); }
    Fluid3D& destinationN2() { return m_data[EGasType::N2].destination(); }
    const Fluid3D& sourceCO2() const { return m_data[EGasType::CO2].source(); }
    Fluid3D& destinationCO2() { return m_data[EGasType::CO2].destination(); }
    const Fluid3D& sourceToxin() const { return m_data[EGasType::Toxin].source(); }
    Fluid3D& destinationToxin() { return m_data[EGasType::Toxin].destination(); }

    const FluidProperties& properties() const { return m_prop; }
    FluidProperties& properties() { return m_prop; }

private:
    TArray<FluidPkg3D, TFixedAllocator<EGasType::GasTypeCount>> m_data;
    FluidProperties m_prop;
};
