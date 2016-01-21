#include "WireCellGen/WireSource.h"
#include "WireCellUtil/NamedFactory.h"

using namespace WireCell;

WIRECELL_NAMEDFACTORY_BEGIN(WireSource)
WIRECELL_NAMEDFACTORY_INTERFACE(WireSource, IWireSource);
WIRECELL_NAMEDFACTORY_END(WireSource)

WireSource::WireSource()
    : m_params(new WireParams)
    , m_wiregen()
{
}

WireSource::~WireSource()
{

}

Configuration WireSource::default_configuration() const
{
    return m_params->default_configuration();
}
void WireSource::configure(const Configuration& cfg)
{
    m_params->configure(cfg);
}

bool WireSource::operator()(output_pointer& wires)
{
    return m_wiregen(m_params, wires);
}
