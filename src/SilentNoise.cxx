#include "WireCellGen/SilentNoise.h" // holly noise
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Units.h"
#include "WireCellIface/SimpleFrame.h"

#include <memory>

WIRECELL_FACTORY(SilentNoise, WireCell::Gen::SilentNoise, WireCell::IFrameSource, WireCell::IConfigurable);



using namespace WireCell;

Gen::SilentNoise::SilentNoise()
    : m_count(0)
{
}

Gen::SilentNoise::~SilentNoise()
{
}

void Gen::SilentNoise::configure(const WireCell::Configuration& )
{
    
}

WireCell::Configuration Gen::SilentNoise::default_configuration() const
{
    return WireCell::Configuration(); // don't say I never gave you anything.
}


bool Gen::SilentNoise::operator()(output_pointer& out)
{
    ++m_count;
    ITrace::vector empty;
    out = std::make_shared<SimpleFrame>(m_count, m_count*5.0*units::ms, empty);
    return true;
}
