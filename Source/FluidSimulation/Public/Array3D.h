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

#pragma once

#include "Array.h"
#include "Delegate.h"
#include "FluidSimulation.h"
#include "Platform.h"

template <typename T>
class TArray3D {
public:
    // Default Constructor - do nothing
    TArray3D()
        : m_X(0)
        , m_Y(0)
        , m_Z(0)
        , m_size(0)
    {
    }

    // Destruct array
    virtual ~TArray3D() {}

    // Constructor
    TArray3D(int32 x, int32 y, int32 z)
        : m_X(x)
        , m_Y(y)
        , m_Z(z)
        , m_size(x * y * z)
    {
        m_array.SetNum(m_size);
    }

    // Constructor
    TArray3D(int32 x, int32 y, int32 z, T initialValue)
        : TArray3D<T>(x, y, z)
    {
        Set(initialValue);
    }

    // Copy Constructor
    TArray3D(const TArray3D& arrIn)
        : m_array(arrIn.m_array)
        , m_X(arrIn.m_X)
        , m_Y(arrIn.m_Y)
        , m_Z(arrIn.m_Z)
        , m_size(arrIn.m_size)
    {
    }

    // Assignment Operator
    FORCEINLINE const TArray3D& operator=(const TArray3D& right)
    {
        //avoid self assignment
        if (this != &right) {
            if (m_size != right.m_size) {
                m_X = right.m_X;
                m_Y = right.m_Y;
                m_Z = right.m_Z;
            }
            m_array = right.m_array;
        }
        return *this;
    }

    //Array bracket operator. Returns reference to element at give index.
    FORCEINLINE const T& operator[](int32 Index) const
    {
        return m_array[Index];
    }

    //Array bracket operator. Returns reference to element at give index.
    FORCEINLINE T& operator[](int32 Index)
    {
        return m_array[Index];
    }

    // Multiplication - Single value
    FORCEINLINE TArray3D operator*(T value)
    {
        TArray3D<T> result(*this);
        result *= value;
        return result;
    }

    // Assignment by Multiplication  - Single value
    FORCEINLINE void operator*=(T value)
    {
        auto count = m_size;
        while (count--) {
            m_array[count] = m_array[count] * value;
        }
    }

    // Multiplication - arrIn values
    FORCEINLINE TArray3D operator*(const TArray3D& arrIn)
    {
        TArray3D<T> result(*this);
        result *= arrIn;
        return result;
    }

    // Assignment by Multiplication
    FORCEINLINE void operator*=(const TArray3D& arrIn)
    {
        auto count = m_size;
        while (count--) {
            m_array[count] = m_array[count] * arrIn[count];
        }
    }

    // Division - Single value
    FORCEINLINE TArray3D operator/(T value)
    {
        TArray3D<T> result(*this);
        result /= value;
        return result;
    }

    // Assignment by Division  - Single value
    FORCEINLINE void operator/=(T value)
    {
        auto count = m_size;
        while (count--) {
            m_array[count] = m_array[count] / value;
        }
    }

    // Division - arrIn values
    FORCEINLINE TArray3D operator/(const TArray3D& arrIn)
    {
        TArray3D<T> result(*this);
        result /= arrIn;
        return result;
    }

    // Assignment by Division
    FORCEINLINE void operator/=(const TArray3D& arrIn)
    {
        auto count = m_size;
        while (count--) {
            m_array[count] = m_array[count] / arrIn[count];
        }
    }

    // Addition - Single value
    FORCEINLINE TArray3D operator+(T value)
    {
        TArray3D<T> result(*this);
        result += value;
        return result;
    }

    // Assignment by Addition - Single value
    FORCEINLINE void operator+=(T value)
    {
        auto count = m_size;
        while (count--) {
            m_array[count] = m_array[count] + value;
        }
    }

    // Addition - arrIn values
    FORCEINLINE TArray3D operator+(const TArray3D& arrIn)
    {
        TArray3D<T> result(*this);
        result += arrIn;
        return result;
    }

    // Assignment by Addition - arrIn values
    FORCEINLINE void operator+=(const TArray3D& arrIn)
    {
        auto count = m_size;
        while (count--) {
            m_array[count] = m_array[count] + arrIn[count];
        }
    }

    // Subtraction - Single value
    FORCEINLINE TArray3D operator-(T value)
    {
        TArray3D<T> result(*this);
        result -= value;
        return result;
    }

    // Assignment by Subtraction - Single value
    FORCEINLINE void operator-=(T value)
    {
        auto count = m_size;
        while (count--) {
            m_array[count] = m_array[count] - value;
        }
    }

    // Subtraction - arrIn values
    FORCEINLINE TArray3D operator-(TArray3D& arrIn)
    {
        TArray3D<T> result(*this);
        result -= arrIn;
        return result;
    }

    // Assignment by Subtraction - arrIn values
    FORCEINLINE void operator-=(TArray3D& arrIn)
    {
        auto count = m_size;
        while (count--) {
            m_array[count] = m_array[count] - arrIn[count];
        }
    }

    // Returns the index in the 1D array from 3D coordinates
    FORCEINLINE int32 index(int32 x, int32 y, int32 z) const
    {
        return x + m_X * (y + m_Y * z);
    }

    // Returns the value in the array from the 3D coordinates
    FORCEINLINE const T& element(int32 x, int32 y, int32 z) const
    {
        return m_array[index(x, y, z)];
    }

    FORCEINLINE T& element(int32 x, int32 y, int32 z)
    {
        return m_array[index(x, y, z)];
    }

    FORCEINLINE int32 GetX() const
    {
        return m_X;
    }

    FORCEINLINE void SetX(int32 setX)
    {
        m_X = setX;
        m_size = m_X * m_Y * m_Z;
        m_array.SetNum(m_size);
    }

    FORCEINLINE int32 GetY() const
    {
        return m_Y;
    }

    FORCEINLINE void SetY(int32 setY)
    {
        m_Y = setY;
        m_size = m_X * m_Y * m_Z;
        m_array.SetNum(m_size);
    }

    FORCEINLINE int32 GetZ() const
    {
        return m_Z;
    }

    FORCEINLINE void SetZ(int32 setZ)
    {
        m_Z = setZ;
        m_size = m_X * m_Y * m_Z;
        m_array.SetNum(m_size);
    }

    FORCEINLINE int32 GetSize() const
    {
        return m_size;
    }

    // Set entire array to a single value
    FORCEINLINE void Set(T initialValue)
    {
        auto count = m_size;
        while (count--) {
            m_array[count] = initialValue;
        }
    }

    // Set value to a delegate return value
    FORCEINLINE void Set(TBaseDelegate<T, int32, int32, int32> func)
    {
        if (!func.IsBound())
            return;
        auto i = 0;
        for (auto x = 0; x < m_X; ++x) {
            for (auto y = 0; y < m_Y; ++y) {
                for (auto z = 0; z < m_Z; ++z) {
                    m_array[i] = func.Execute(x, y, z);
                    ++i;
                }
            }
        }
    }

protected:
    TArray<T, FDefaultAllocator> m_array; // internal array

    int32 m_X; // X dimension of array
    int32 m_Y; // Y dimension of array
    int32 m_Z; // Z dimension of array
    int32 m_size; // Total size of array
};
