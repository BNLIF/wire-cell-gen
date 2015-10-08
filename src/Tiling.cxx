#include "WireCellGen/Tiling.h"
#include "WireCellGen/TilingGraph.h"

#include "WireCellUtil/NamedFactory.h"

#include <vector>
#include <iostream>

using namespace WireCell;

WIRECELL_NAMEDFACTORY(Tiling) {
    WIRECELL_NAMEDFACTORY_INTERFACE(Tiling, ITiling);
}


Tiling::Tiling()
{
}

Tiling::~Tiling()
{
}

bool Tiling::insert(const input_type& cell_vector)
{
    if (!cell_vector) {
	m_output = nullptr;
	return true;
    }
    m_output = output_type(new TilingGraph(*cell_vector));
    return true;
}

