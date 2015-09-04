#include "WireCellGen/Tiling.h"
#include "WireCellGen/TilingGraph.h"

#include "WireCellUtil/NamedFactory.h"

#include <vector>
#include <iostream>

using namespace WireCell;

WIRECELL_NAMEDFACTORY(Tiling);
WIRECELL_NAMEDFACTORY_ASSOCIATE(Tiling, ITiling);


Tiling::Tiling()
{
}

Tiling::~Tiling()
{
}

void Tiling::reset()
{
    m_output.clear();
}
void Tiling::flush()
{
}

bool Tiling::insert(const input_type& cell_vector)
{
    auto summary = ICellSummary::pointer(new TilingGraph(cell_vector));
    m_output.push_back(summary);
    return true;
}

bool Tiling::extract(output_type& cell_summary) 
{
    if (m_output.empty()) {
	return false;
    }
    cell_summary = m_output.front();
    m_output.pop_front();
    return true;
}
