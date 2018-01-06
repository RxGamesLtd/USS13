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

#include "Fluid3D.h"

Fluid3D::Fluid3D(int32 x, int32 y, int32 z) : TArray3D(x, y, z){};

// Distribute a value to the 8 grid points surrounding the floating point coordinates
// x,y,z must be 1 less than their associated max values (dimensions of array)
void Fluid3D::distributeFloatingPoint(float x, float y, float z, float value)
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
    this->element(ix, iy, iz + 1) += (1.0f - fx) * (1.0f - fy) * (fz)*value;
    this->element(ix + 1, iy + 1, iz) += (fx) * (fy) * (1.0f - fz) * value;
    this->element(ix + 1, iy, iz + 1) += (fx) * (1.0f - fy) * (fz)*value;
    this->element(ix, iy + 1, iz + 1) += (1.0f - fx) * (fy) * (fz)*value;
    this->element(ix + 1, iy + 1, iz + 1) += (fx) * (fy) * (fz)*value;
}
