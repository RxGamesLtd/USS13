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

// VelocityPackage3D is a container class for the Source and Destination Fluid3D objects of velocity in the
// x, y, and z directions.
class FLUIDSIMULATION_API VelPkg3D
{
public:
    VelPkg3D();
    // Constructor - Initilizes source and destination FLuid3D objects for velocity in X, Y, Z directions
    VelPkg3D(int32 xSize, int32 ySize, int32 zSize);

    // Swap the source and destination objects for velocity
    void swap();

    // Reset the source and destination objects to specified value
    void reset(float value);

    // Accessors
    const Fluid3D& sourceX() const { return m_data[0].source(); }
    const Fluid3D& sourceY() const { return m_data[1].source(); }
    const Fluid3D& sourceZ() const { return m_data[2].source(); }

    Fluid3D& destinationX() { return m_data[0].destination(); }
    Fluid3D& destinationY() { return m_data[1].destination(); }
    Fluid3D& destinationZ() { return m_data[2].destination(); }

    FluidProperties& properties() { return m_prop; }

private:
    TArray<FluidPkg3D, TFixedAllocator<3>> m_data; // 3 components for each direction
    FluidProperties m_prop;
};
