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

#include "Platform.h"
#include "Array.h"

template <typename T>
class TArray3D : public TArray<T>
{
public:

	// Constructor
	TArray3D(int32 x, int32 y, int32 z)
		: TArray<T>(),
		  m_X(x), m_Y(y), m_Z(z), m_size(x * y * z)
	{
		TArray<T>::SetNum(m_size);
	}

	// Constructor
	TArray3D(int32 x, int32 y, int32 z, T initialValue) : TArray3D<T>(x, y, z)
	{
		Set(initialValue);
	}

	// Copy Constructor
	TArray3D(const TArray3D& arrIn)
		: TArray<T>(arrIn),
		  m_X(arrIn.m_X), m_Y(arrIn.m_Y), m_Z(arrIn.m_Z), m_size(arrIn.m_X * arrIn.m_Y * arrIn.m_Z)
	{
	}

	// Multiplication - Single value
	FORCEINLINE TArray3D operator*(T value)
	{
		TArray3D result(*this);
		result *= value;
		return result;
	}

	// Assignment by Multiplication  - Single value
	FORCEINLINE void operator*=(T value)
	{
		auto count = m_size;
		while (count--)
		{
			TArray<T>::operator[](count) = TArray<T>::operator[](count) * value;
		}
	}

	// Multiplication - arrIn values
	FORCEINLINE TArray3D operator*(const TArray3D& arrIn)
	{
		TArray3D result(*this);
		result *= arrIn;
		return result;
	}

	// Assignment by Multiplication
	FORCEINLINE void operator*=(const TArray3D& arrIn)
	{
		auto count = m_size;
		while (count--)
		{
			TArray<T>::operator[](count) = TArray<T>::operator[](count) * arrIn[count];
		}
	}

	// Division - Single value
	FORCEINLINE TArray3D operator/(T value)
	{
		TArray3D result(*this);
		result /= value;
		return result;
	}

	// Assignment by Division  - Single value
	FORCEINLINE void operator/=(T value)
	{
		auto count = m_size;
		while (count--)
		{
			TArray<T>::operator[](count) = TArray<T>::operator[](count) / value;
		}
	}

	// Division - arrIn values
	FORCEINLINE TArray3D operator/(const TArray3D& arrIn)
	{
		TArray3D result(*this);
		result /= arrIn;
		return result;
	}

	// Assignment by Division
	FORCEINLINE void operator/=(const TArray3D& arrIn)
	{
		auto count = m_size;
		while (count--)
		{
			TArray<T>::operator[](count) = TArray<T>::operator[](count) / arrIn[count];
		}
	}

	// Addition - Single value
	FORCEINLINE TArray3D operator+(T value)
	{
		TArray3D result(*this);
		result += value;
		return result;
	}

	// Assignment by Addition - Single value
	// ReSharper disable once CppHidingFunction
	FORCEINLINE void operator+=(T value)
	{
		auto count = m_size;
		while (count--)
		{
			TArray<T>::operator[](count) = TArray<T>::operator[](count) + value;
		}
	}

	// Addition - arrIn values
	FORCEINLINE TArray3D operator+(const TArray3D& arrIn)
	{
		TArray3D result(*this);
		result += arrIn;
		return result;
	}

	// Assignment by Addition - arrIn values
	FORCEINLINE void operator+=(const TArray3D& arrIn)
	{
		auto count = m_size;
		while (count--)
		{
			TArray<T>::operator[](count) = TArray<T>::operator[](count) + arrIn[count];
		}
	}

	// Subtraction - Single value
	FORCEINLINE TArray3D operator-(T value)
	{
		TArray3D result(*this);
		result -= value;
		return result;
	}

	// Assignment by Subtraction - Single value
	FORCEINLINE void operator-=(T value)
	{
		auto count = m_size;
		while (count--)
		{
			TArray<T>::operator[](count) = TArray<T>::operator[](count) - value;
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
		while (count--)
		{
			TArray<T>::operator[](count) = TArray<T>::operator[](count) - arrIn[count];
		}
	}

	// Assignment Operator
	virtual FORCEINLINE const TArray3D& operator=(const TArray3D& right)
	{
		//avoid self assignment
		if (this != &right)
		{
			if (m_size != right.m_size)
			{
				m_X = right.m_X;
				m_Y = right.m_Y;
				m_Z = right.m_Z;
			}
		}
		TArray<T>::operator=(right);
		return *this;
	}

	// Returns the index in the 1D array from 3D coordinates
	FORCEINLINE int64 index(int32 x, int32 y, int32 z) const
	{
		return x + m_X * (y + m_Y * z);
	}

	// Returns the value in the array from the 3D coordinates
	FORCEINLINE const T& element(int32 x, int32 y, int32 z) const
	{
		return TArray<T>::operator[](index(x, y, z));
	}

	FORCEINLINE T& element(int32 x, int32 y, int32 z)
	{
		return TArray<T>::operator[](index(x, y, z));
	}

	int32 GetX() const
	{
		return m_X;
	}

	void SetX(int32 setX)
	{
		m_X = setX;
		m_size = m_X * m_Y * m_Z;
		TArray<T>::SetNum(m_size);
	}

	int32 GetY() const
	{
		return m_Y;
	}

	void SetY(int32 setY)
	{
		m_Y = setY;
		m_size = m_X * m_Y * m_Z;
		TArray<T>::SetNum(m_size);
	}

	int32 GetZ() const
	{
		return m_Z;
	}


	void SetZ(int32 setZ)
	{
		m_Z = setZ;
		m_size = m_X * m_Y * m_Z;
		TArray<T>::SetNum(m_size);
	}

	int32 GetSize() const
	{
		return m_size;
	}

	// Set entire array to a single value
	void Set(T initialValue)
	{
		auto count = m_size;
		while (count--)
		{
			TArray<T>::operator[](count) = initialValue;
		}
	}

	template <typename UserClass>
	void Set(UserClass* t, void (UserClass::*fp)(T& arr, int32 i, int32 j, int32 k, int64 index) const)
	{
		int32 i = 0;
		int32 x, y, z;
		for (x = 0; x < m_X; ++x)
		{
			for (y = 0; y < m_Y; ++y)
			{
				for (z = 0; z < m_Z; ++z)
				{
					(t ->* fp)(TArray<T>::operator[](i), x, y, z, i);
					++i;
				}
			}
		}
	}

protected:

	int32 m_X; // X dimension of array  
	int32 m_Y; // Y dimension of array
	int32 m_Z; // Z dimension of array
	int64 m_size; // Total size of array
};
