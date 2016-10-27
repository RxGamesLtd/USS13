#include "Public/FluidSimulation.h"
#include "Public/AtmoPkg3D.h"

AtmoPkg3D::AtmoPkg3D(int32 x, int32 y, int32 z) : m_X(x), m_Y(y), m_Z(z)
{
	mp_prop = MakeShareable(new FluidProperties());
	mp_prop->diffusion = 0.0f;
	mp_prop->advection = 0.0f;
	mp_prop->force = 0.0f;
	mp_prop->decay = 0.0f;
	mp_sourceO2 = MakeShareable(new Fluid3D(m_X, m_Y, m_Z));
	mp_destO2 = MakeShareable(new Fluid3D(m_X, m_Y, m_Z));
	mp_sourceN2 = MakeShareable(new Fluid3D(m_X, m_Y, m_Z));
	mp_destN2 = MakeShareable(new Fluid3D(m_X, m_Y, m_Z));
	mp_sourceCO2 = MakeShareable(new Fluid3D(m_X, m_Y, m_Z));
	mp_destCO2 = MakeShareable(new Fluid3D(m_X, m_Y, m_Z));
	mp_sourceToxin = MakeShareable(new Fluid3D(m_X, m_Y, m_Z));
	mp_destToxin = MakeShareable(new Fluid3D(m_X, m_Y, m_Z));
}

AtmoPkg3D& AtmoPkg3D::operator=(const AtmoPkg3D& right)
{
	if (this != &right)
	{
		mp_sourceO2 = MakeShareable(new Fluid3D(right.m_X, right.m_Y, right.m_Z));
		mp_destO2 = MakeShareable(new Fluid3D(right.m_X, right.m_Y, right.m_Z));
		mp_sourceN2 = MakeShareable(new Fluid3D(right.m_X, right.m_Y, right.m_Z));
		mp_destN2 = MakeShareable(new Fluid3D(right.m_X, right.m_Y, right.m_Z));
		mp_sourceCO2 = MakeShareable(new Fluid3D(right.m_X, right.m_Y, right.m_Z));
		mp_destCO2 = MakeShareable(new Fluid3D(right.m_X, right.m_Y, right.m_Z));
		mp_sourceToxin = MakeShareable(new Fluid3D(right.m_X, right.m_Y, right.m_Z));
		mp_destToxin = MakeShareable(new Fluid3D(right.m_X, right.m_Y, right.m_Z));
		mp_prop = MakeShareable(new FluidProperties());
		mp_prop->diffusion = right.mp_prop->diffusion;
		mp_prop->advection = right.mp_prop->advection;
		mp_prop->force = right.mp_prop->force;
		mp_prop->decay = right.mp_prop->decay;
	}
	return *this;
}

void AtmoPkg3D::SwapLocations()
{
	auto temp = mp_sourceO2;
	mp_sourceO2 = mp_destO2;
	mp_destO2 = temp;

	temp = mp_sourceN2;
	mp_sourceN2 = mp_destN2;
	mp_destN2 = temp;

	temp = mp_sourceCO2;
	mp_sourceCO2 = mp_destCO2;
	mp_destCO2 = temp;

	temp = mp_sourceToxin;
	mp_sourceToxin = mp_destToxin;
	mp_destToxin = temp;
}

void AtmoPkg3D::Reset(float v) const
{
	mp_sourceO2->Set(v);
	mp_destO2->Set(v);

	mp_sourceN2->Set(v);
	mp_destN2->Set(v);

	mp_sourceCO2->Set(v);
	mp_destCO2->Set(v);

	mp_sourceToxin->Set(v);
	mp_destToxin->Set(v);
}

TSharedPtr<Fluid3D, ESPMode::ThreadSafe> AtmoPkg3D::SourceO2() const
{
	return mp_sourceO2;
}

TSharedPtr<Fluid3D, ESPMode::ThreadSafe> AtmoPkg3D::DestinationO2() const
{
	return mp_destO2;
}

TSharedPtr<Fluid3D, ESPMode::ThreadSafe> AtmoPkg3D::SourceN2() const
{
	return mp_sourceN2;
}

TSharedPtr<Fluid3D, ESPMode::ThreadSafe> AtmoPkg3D::DestinationN2() const
{
	return mp_destN2;
}

TSharedPtr<Fluid3D, ESPMode::ThreadSafe> AtmoPkg3D::SourceCO2() const
{
	return mp_sourceCO2;
}

TSharedPtr<Fluid3D, ESPMode::ThreadSafe> AtmoPkg3D::DestinationCO2() const
{
	return mp_destCO2;
}

TSharedPtr<Fluid3D, ESPMode::ThreadSafe> AtmoPkg3D::SourceToxin() const
{
	return mp_sourceToxin;
}

TSharedPtr<Fluid3D, ESPMode::ThreadSafe> AtmoPkg3D::DestinationToxin() const
{
	return mp_destToxin;
}

TSharedPtr<FluidProperties, ESPMode::ThreadSafe> AtmoPkg3D::Properties() const
{
	return mp_prop;
}
