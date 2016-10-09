#pragma once

#include "Fluid3D.h"
#include "FluidProperties.h"
#include "Platform.h"
#include "SharedPointer.h"

class AtmoPkg3D
{
public:

	// Constructor - Initilizes source and destination FLuid3D objects for atmo in X, Y, Z directions
	AtmoPkg3D(int32 xSize, int32 ySize, int32 zSize);

	// Assignment Operator
	AtmoPkg3D& operator=(const AtmoPkg3D& right);

	// Swap the source and destination objects 
	void SwapLocations();

	// Reset the source and destination objects to specified value
	void Reset(float v) const;

	// Accessors
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> SourceO2() const;
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> DestinationO2() const;
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> SourceN2() const;
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> DestinationN2() const;
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> SourceCO2() const;
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> DestinationCO2() const;
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> SourceToxin() const;
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> DestinationToxin() const;
	TSharedPtr<FluidProperties, ESPMode::ThreadSafe> Properties() const;

private:
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> mp_sourceO2; // source  for velocity in X direction
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> mp_destO2; // destination for velocity in X direction
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> mp_sourceN2; // source for velocity in Y direction
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> mp_destN2; // destination for velocity in Y direction
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> mp_sourceCO2; // source for velocity in Z direction
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> mp_destCO2; // destination for velocity in Z direction
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> mp_sourceToxin; // source for velocity in Z direction
	TSharedPtr<Fluid3D, ESPMode::ThreadSafe> mp_destToxin; // destination for velocity in Z direction
	TSharedPtr<FluidProperties, ESPMode::ThreadSafe> mp_prop;
	int32 m_X;
	int32 m_Y;
	int32 m_Z;
};
