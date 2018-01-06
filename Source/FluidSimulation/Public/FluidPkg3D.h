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

#include "Array.h"
#include "Fluid3D.h"
#include "FluidProperties.h"

// Class consists of a 'source' and 'destination' Fluid3D object and defines
// the properties they share
class FluidPkg3D
{
public:
    FluidPkg3D();
    // Constructor - Initilizes source and destination FLuid3D objects
    FluidPkg3D(int32 xSize, int32 ySize, int32 zSize);

    // Swap the source and destination objects
    void swap();

    // Reset the source and destination objects to specified value
    void reset(float value);

    // Accessors
    const Fluid3D& source() const { return m_data[m_sourceBuffer]; }
    Fluid3D& destination() { return m_data[(m_sourceBuffer + 1) % 2]; }
    FluidProperties& properties() { return m_prop; }

private:
    TArray<Fluid3D, TFixedAllocator<2>> m_data; // double buffering
    int32 m_sourceBuffer;

    FluidProperties m_prop; // defines properties of fluid
};
