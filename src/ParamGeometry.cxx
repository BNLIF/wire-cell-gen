#include "WireCellGen/ParamGeometry.h"
#include "WireCellIface/IWire.h"
#include "WireCellIface/ICell.h"
#include "WireCellIface/IWireSummary.h"
#include "WireCellIface/ITiling.h"
#include "WireCellIface/IWireParameters.h"
#include "WireCellIface/IWireGenerator.h"
#include "WireCellUtil/NamedFactory.h"

using namespace WireCell;

WIRECELL_NAMEDFACTORY(ParamGeometry);
WIRECELL_NAMEDFACTORY_ASSOCIATE(ParamGeometry, IConfigurable);
WIRECELL_NAMEDFACTORY_ASSOCIATE(ParamGeometry, IGeometry);

Configuration ParamGeometry::default_configuration() const
{
    return configuration_loads("", "json"); // fixme
}

void ParamGeometry::configure(const Configuration& cfg)
{
} // fixme

ParamGeometry::ParamGeometry()
{
    auto wp_wps = WireCell::Factory::lookup<IWireParameters>("WireParams");
    auto pw_gen = WireCell::Factory::lookup<IWireGenerator>("ParamWires");
    pw_gen->generate(wp_wps);
    m_wires = WireCell::Factory::lookup<IWireSequence>("ParamWires");
    IWireSink::pointer bc_sink = WireCell::Factory::lookup<IWireSink>("BoundCells");
    bc_sink->sink(m_wires->wires_range());
    m_cells = WireCell::Factory::lookup<ICellSequence>("BoundCells");
    
    auto wire_sink = WireCell::Factory::lookup<IWireSink>("WireSummary");
    wire_sink->sink(m_wires->wires_range());
    m_wiresummary = WireCell::Factory::lookup<IWireSummary>("WireSummary");
    
    auto tiling = WireCell::Factory::lookup<ICellSink>("Tiling");
    tiling->sink(m_cells->cells_range());
    m_tiling = WireCell::Factory::lookup<ITiling>("Tiling");
}
ParamGeometry::~ParamGeometry()
{
}
