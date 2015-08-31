#include "WireCellGen/Tiling.h"
#include "WireCellGen/TilingGraph.h"

#include "WireCellUtil/NamedFactory.h"

#include <vector>
#include <iostream>

using namespace WireCell;

WIRECELL_NAMEDFACTORY(Tiling);
WIRECELL_NAMEDFACTORY_ASSOCIATE(Tiling, ITiling);


Tiling::Tiling()
    : m_summary(nullptr)
{
}

Tiling::~Tiling()
{
}

bool Tiling::sink(const ICellVector& cells)
{
    m_summary = ICellSummary::pointer(new TilingGraph(cells));
    return true;
}

