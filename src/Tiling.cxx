#include "WireCellGen/Tiling.h"
#include "WireCellUtil/NamedFactory.h"

#include "TilingGraph.h"

#include <vector>
#include <iostream>

using namespace WireCell;

WIRECELL_NAMEDFACTORY(Tiling);
WIRECELL_NAMEDFACTORY_ASSOCIATE(Tiling, ICellSink);
WIRECELL_NAMEDFACTORY_ASSOCIATE(Tiling, ITiling);


Tiling::Tiling()
    : m_graph(0)
{
}

Tiling::~Tiling()
{
    if (m_graph) {
    	delete m_graph;
    	m_graph = 0;
    }
}


void Tiling::sink(const ICell::iterator_range& cells)
{
    if (m_graph) {
	delete m_graph;
	m_graph = 0;
    }

    if (cells.begin() == cells.end()) {
	std::cerr << "Tiling: no cells set" << std::endl;
	return;
    }

    m_graph = new TilingGraph;
    for (auto cp : cells) {
	m_graph->record(cp);
    }
}



IWireVector Tiling::wires(ICell::pointer cell) const 
{
    // freebie
    return cell->wires();
}
ICellVector Tiling::cells(IWire::pointer wire) const 
{
    if (!m_graph) {
	ICellVector c;
	return c;
    }
    return m_graph->cells(wire);
}
ICell::pointer Tiling::cell(const IWireVector& wires) const 
{
    if (!m_graph) {
	return 0;
    }
    return m_graph->cell(wires);
}
ICellVector Tiling::neighbors(ICell::pointer cell) const 
{
    ICellVector v;
    return v;
}

