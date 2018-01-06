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
#include "Delegate.h"
#include "Platform.h"

template <typename T>
class TArray3D
{
    using ValueType = T;

public:
    // Default Constructor - do nothing
    TArray3D() : m_x(0), m_y(0), m_z(0), m_size(0) {}

    // Destruct array
    virtual ~TArray3D() = default;

    // Constructor
    TArray3D(int32 x, int32 y, int32 z) : m_x(x), m_y(y), m_z(z), m_size(x * y * z) { m_array.SetNum(m_size); }

    // Constructor
    TArray3D(int32 x, int32 y, int32 z, ValueType initialValue) : TArray3D(x, y, z) { set(initialValue); }

    // Array bracket operator. Returns reference to element at give index.
    FORCEINLINE const ValueType& operator[](int32 index) const { return m_array[index]; }

    // Array bracket operator. Returns reference to element at give index.
    FORCEINLINE ValueType& operator[](int32 index) { return m_array[index]; }

    // Multiplication - Single value
    FORCEINLINE TArray3D operator*(ValueType value)
    {
        TArray3D<ValueType> result(*this);
        result *= value;
        return result;
    }

    // Assignment by Multiplication  - Single value
    FORCEINLINE void operator*=(ValueType value)
    {
        auto count = m_size;
        while(count--)
        {
            m_array[count] = m_array[count] * value;
        }
    }

    // Multiplication - arrIn values
    FORCEINLINE TArray3D operator*(const TArray3D& arrIn)
    {
        TArray3D<ValueType> result(*this);
        result *= arrIn;
        return result;
    }

    // Assignment by Multiplication
    FORCEINLINE void operator*=(const TArray3D& arrIn)
    {
        auto count = m_size;
        while(count--)
        {
            m_array[count] = m_array[count] * arrIn[count];
        }
    }

    // Division - Single value
    FORCEINLINE TArray3D operator/(ValueType value)
    {
        TArray3D<ValueType> result(*this);
        result /= value;
        return result;
    }

    // Assignment by Division  - Single value
    FORCEINLINE void operator/=(ValueType value)
    {
        auto count = m_size;
        while(count--)
        {
            m_array[count] = m_array[count] / value;
        }
    }

    // Division - arrIn values
    FORCEINLINE TArray3D operator/(const TArray3D& arrIn)
    {
        TArray3D<ValueType> result(*this);
        result /= arrIn;
        return result;
    }

    // Assignment by Division
    FORCEINLINE void operator/=(const TArray3D& arrIn)
    {
        auto count = m_size;
        while(count--)
        {
            m_array[count] = m_array[count] / arrIn[count];
        }
    }

    // Addition - Single value
    FORCEINLINE TArray3D operator+(ValueType value)
    {
        TArray3D<ValueType> result(*this);
        result += value;
        return result;
    }

    // Assignment by Addition - Single value
    FORCEINLINE void operator+=(ValueType value)
    {
        auto count = m_size;
        while(count--)
        {
            m_array[count] = m_array[count] + value;
        }
    }

    // Addition - arrIn values
    FORCEINLINE TArray3D operator+(const TArray3D& arrIn)
    {
        TArray3D<ValueType> result(*this);
        result += arrIn;
        return result;
    }

    // Assignment by Addition - arrIn values
    FORCEINLINE void operator+=(const TArray3D& arrIn)
    {
        auto count = m_size;
        while(count--)
        {
            m_array[count] = m_array[count] + arrIn[count];
        }
    }

    // Subtraction - Single value
    FORCEINLINE TArray3D operator-(ValueType value)
    {
        TArray3D<ValueType> result(*this);
        result -= value;
        return result;
    }

    // Assignment by Subtraction - Single value
    FORCEINLINE void operator-=(ValueType value)
    {
        auto count = m_size;
        while(count--)
        {
            m_array[count] = m_array[count] - value;
        }
    }

    // Subtraction - arrIn values
    FORCEINLINE TArray3D operator-(TArray3D& arrIn)
    {
        TArray3D<ValueType> result(*this);
        result -= arrIn;
        return result;
    }

    // Assignment by Subtraction - arrIn values
    FORCEINLINE void operator-=(TArray3D& arrIn)
    {
        auto count = m_size;
        while(count--)
        {
            m_array[count] = m_array[count] - arrIn[count];
        }
    }

    // Returns the index in the 1D array from 3D coordinates
    FORCEINLINE int32 index(int32 x, int32 y, int32 z) const { return x + m_x * (y + m_y * z); }

    // Returns the value in the array from the 3D coordinates
    FORCEINLINE const ValueType& element(int32 x, int32 y, int32 z) const { return m_array[index(x, y, z)]; }

    FORCEINLINE ValueType& element(int32 x, int32 y, int32 z) { return m_array[index(x, y, z)]; }

    FORCEINLINE int32 getX() const { return m_x; }

    FORCEINLINE void setX(int32 x)
    {
        m_x = x;
        m_size = m_x * m_y * m_z;
        m_array.SetNum(m_size);
    }

    FORCEINLINE int32 getY() const { return m_y; }

    FORCEINLINE void setY(int32 y)
    {
        m_y = y;
        m_size = m_x * m_y * m_z;
        m_array.SetNum(m_size);
    }

    FORCEINLINE int32 getZ() const { return m_z; }

    FORCEINLINE void setZ(int32 z)
    {
        m_z = z;
        m_size = m_x * m_y * m_z;
        m_array.SetNum(m_size);
    }

    FORCEINLINE int32 size() const { return m_size; }

    // Set entire array to a single value
    FORCEINLINE void set(ValueType initialValue)
    {
        auto count = m_size;
        while(count--)
        {
            m_array[count] = initialValue;
        }
    }

    // Set value to a delegate return value
    FORCEINLINE void set(TBaseDelegate<ValueType, int32, int32, int32> func)
    {
        if(!func.IsBound())
            return;
        auto i = 0;
        for(auto x = 0; x < m_x; ++x)
        {
            for(auto y = 0; y < m_y; ++y)
            {
                for(auto z = 0; z < m_z; ++z)
                {
                    m_array[i] = func.Execute(x, y, z);
                    ++i;
                }
            }
        }
    }

protected:
    TArray<ValueType, FDefaultAllocator> m_array; // internal array

    int32 m_x; // X dimension of array
    int32 m_y; // Y dimension of array
    int32 m_z; // Z dimension of array
    int32 m_size; // Total size of array
};
