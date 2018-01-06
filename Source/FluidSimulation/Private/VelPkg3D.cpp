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

#include "VelPkg3D.h"

VelPkg3D::VelPkg3D() {}

VelPkg3D::VelPkg3D(int32 xSize, int32 ySize, int32 zSize)
  : m_data{{xSize, ySize, zSize}, {xSize, ySize, zSize}, {xSize, ySize, zSize}}
{
}

void VelPkg3D::swap()
{
    for(int i = 0; i < m_data.Num(); ++i)
    {
        m_data[i].swap();
    }
}

void VelPkg3D::reset(float value)
{
    for(int i = 0; i < m_data.Num(); ++i)
    {
        m_data[i].reset(value);
    }
}
