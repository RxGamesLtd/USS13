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

#include "Array3D.h"

// Adds fluid specific functions to class Array3D
class FLUIDSIMULATIONMODULE_API Fluid3D : public TArray3D<float>
{
public:
    // Default constructor for zero sized array
    Fluid3D() = default;

    // Constructor - Sets size of array
    Fluid3D(int32 x, int32 y, int32 z);

    // When a point is advected it will land in Add fractions of value to the 4 neighboring grid
    // points of the floating point coordinates
    void distributeFloatingPoint(float x, float y, float z, float value);
};
