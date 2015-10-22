#include "WireCellGen/Tiling.h"
#include "WireCellGen/TilingGraph.h"

#include "WireCellUtil/NamedFactory.h"

#include <vector>
#include <iostream>

using namespace WireCell;

WIRECELL_NAMEDFACTORY_BEGIN(Tiling) 
WIRECELL_NAMEDFACTORY_INTERFACE(Tiling, ITiling);
WIRECELL_NAMEDFACTORY_END(Tiling) 


Tiling::Tiling()
{
}

Tiling::~Tiling()
{
}

bool Tiling::insert(const input_pointer& cell_vector)
{
    if (!cell_vector) {
	m_output = nullptr;
	return true;
    }
    m_output = output_pointer(new TilingGraph(*cell_vector));
    return true;
}

